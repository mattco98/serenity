/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Path.h>
#include <LibGfx/PathClipping.h>

namespace Gfx {

#define CLIP_DEBUG 1

static constexpr float EPSILON = 0.0001f;

struct IntersectionResult {
    enum Type {
        Coincident,
        Intersects,
        DoesNotIntersect,
    };

    Type type;
    FloatPoint point {};
};

static bool equivalent(float a, float b)
{
    return fabsf(a - b) < EPSILON;
}

static bool equivalent(FloatPoint const& a, FloatPoint const& b)
{
    return equivalent(a.x(), b.x()) && equivalent(a.y(), b.y());
}

template<typename T>
static int operator<=>(Point<T> const& a, Point<T> const& b)
{
    if (equivalent(a.x(), b.x())) {
        if (equivalent(a.y(), b.y()))
            return 0;
        return a.y() < b.y() ? -1 : 1;
    }
    return a.x() < b.x() ? -1 : 1;
}

static bool points_are_collinear(FloatPoint const& a, FloatPoint const& b, FloatPoint const& c);

// All three points should be collinear
static bool is_point_between(FloatPoint const& point, FloatPoint const& a, FloatPoint const& b)
{
    VERIFY(a < b);

#if CLIP_DEBUG
    VERIFY(points_are_collinear(a, b, point));
#endif

    if (equivalent(a.x(), b.x())) {
        // This is a vertical line
        return point.y() >= a.y() && point.y() <= b.y();
    }

    return point.x() >= a.x() && point.x() <= b.x();
}

static float line_slope(FloatPoint const& a, FloatPoint const& b)
{
    return (b.y() - a.y()) / (b.x() - a.x());
}

static bool points_are_collinear(FloatPoint const& a, FloatPoint const& b, FloatPoint const& c)
{
    if (equivalent(a, b) || equivalent(b, c) || equivalent(a, c))
        return true;
    return equivalent(line_slope(a, b), line_slope(b, c));
}

static bool point_above_or_on_line(FloatPoint const& point, FloatPoint const& a, FloatPoint const& b)
{
    if (equivalent(a.x(), b.x()))
        return point.x() >= a.x();

    auto slope = line_slope(a, b);
    auto intersecting_y = (point.x() - a.x()) * slope + a.y();
    return point.y() <= intersecting_y;
}

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line_segment
static IntersectionResult line_intersection(FloatPoint const& a1, FloatPoint const& a2, FloatPoint const& b1, FloatPoint const& b2)
{
    auto get_nonintersection_result = [&] {
        auto slope1 = line_slope(a1, a2);
        auto slope2 = line_slope(b1, b2);

        if (isinf(slope1)) {
            if (isinf(slope2) && equivalent(a1.x(), b1.x()))
                return IntersectionResult { IntersectionResult::Coincident };
            return IntersectionResult { IntersectionResult::DoesNotIntersect };
        }

        // Note that it doesn't really matter which three points we pass into points_are_collinear,
        // since all four will be collinear if the lines are coincident
        if (equivalent(slope1, slope2) && points_are_collinear(a1, a2, b1))
            return IntersectionResult { IntersectionResult::Coincident };

        // FIXME: Old code, remove?
        // if (fabsf(slope1 - slope2) < EPSILON) {
        //     if (fabsf(a1.x() - b1.x()) < EPSILON && fabsf(a1.y() - b1.y()) < EPSILON)
        //         return IntersectionResult { IntersectionResult::Coincident };
        // }

        return IntersectionResult { IntersectionResult::DoesNotIntersect };
    };

    auto x1 = a1.x();
    auto x2 = a2.x();
    auto x3 = b1.x();
    auto x4 = b2.x();
    auto y1 = a1.y();
    auto y2 = a2.y();
    auto y3 = b1.y();
    auto y4 = b2.y();

    auto denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    if (fabsf(denominator) < EPSILON) {
        // Guaranteed to be parallel or coincident
        return get_nonintersection_result();
    }

    auto t_numerator = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
    auto u_numerator = (x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3);

    // We can test if the intersection point is on the line before we do the division
    //
    // The following must be true:
    //     0 <= t <= 1, 0 <= u <= 1
    // And:
    //     t = t_numerator / denominator
    //     u = u_numerator / denominator
    // Therefore:
    //     0 <= t_numerator <= denominator
    //     0 <= u_numerator <= denominator

    if (denominator < 0.0f) {
        // Invert the signs so we don't multiply away the negative sign
        denominator *= -1.0f;
        t_numerator *= -1.0f;
        u_numerator *= -1.0f;
    }

    if (t_numerator < 0.0f || t_numerator > denominator)
        return get_nonintersection_result();
    if (u_numerator < 0.0f || u_numerator > denominator)
        return get_nonintersection_result();

    auto t = t_numerator / denominator;

    auto x = x1 + t * (x2 - x1);
    auto y = y1 + t * (y2 - y1);

    return IntersectionResult { IntersectionResult::Intersects, { x, y } };
}

class PathClipping::Event : RefCounted<PathClipping::Event> {
public:
    static Array<RefPtr<Event>, 2> make_event_pair(PathClipping::Segment const& segment)
    {
        auto start_event = adopt_ref(*new Event(true, segment));
        auto end_event = adopt_ref(*new Event(false, segment));
        start_event->m_other_event = end_event;
        end_event->m_other_event = start_event;
        return { start_event, end_event };
    }
    Event(bool is_start, Segment const& segment)
        : m_is_start(is_start)
        , m_segment(segment)
    {
    }

    auto const& point() const { return m_is_start ? m_segment.start : m_segment.end; }
    auto const& other_point() const { return m_is_start ? m_segment.end : m_segment.start; }

    bool is_start() const { return m_is_start; }

    auto const& segment() const { return m_segment; }
    auto& segment() { return m_segment; }
    void set_segment(Segment const& segment) { m_segment = segment; }

    auto const& other_event() const { return m_other_event; }
    void set_other_event(RefPtr<Event> const& event) { m_other_event = event; }


    int operator<=>(Event const& other) const
    {
        auto start_comparison = point() <=> other.point();
        if (start_comparison != 0)
            return start_comparison;

        if (equivalent(point(), other.point()))
            return 0;

        if (m_is_start != other.m_is_start)
            return m_is_start ? 1 : -1;

        return point_above_or_on_line(other_point(), other.point(), other.other_point()) ? 1 : -1;
    }

private:
    bool m_is_start;
    Segment m_segment;
    RefPtr<Event> m_other_event;
};

PathClipping::Polygon PathClipping::make_polygon(Path& path)
{
    path.close();

    PathClipping clipping;

    for (auto const& split_line : path.split_lines())
    {
        auto point_comparison = split_line.from <=> split_line.to;
        if (point_comparison == 0)
            continue;

        if (point_comparison > 0)
            swap(split_line.from, split_line.to);

        Segment segment { split_line.from, split_line.to };
        auto [start_event, end_event] = Event::make_event_pair(segment);
        clipping.add_event(start_event);
        clipping.add_event(end_event);
    }

    TODO();
}

void PathClipping::process_events()
{
    auto remove_duplicate_segment = [&](RefPtr<Event>& to_remove, RefPtr<Event>& other) {
        m_event_queue.remove(to_remove);
        m_event_queue.remove(to_remove->other_event());

        // FIXME: Is this necessary?
        m_status.remove(to_remove);
        m_status.remove(to_remove->other_event());

        // ev = to_remove
        // eve = other

        // Update annotations to the surviving event
        bool toggle;
        if (to_remove->segment().self.below == IsInside::Unknown) {
            toggle = true;
        } else {
            toggle = to_remove->segment().self.above != to_remove->segment().self.below;
        }

        if (toggle)
            other->segment().self.above = other->segment().self.above == IsInside::Yes ? IsInside::No : IsInside::Yes;
    };

    auto do_intersection = [&](RefPtr<Event>& a, RefPtr<Event>& b) -> bool {
        auto const& a1 = a->segment().start;
        auto const& a2 = a->segment().end;
        auto const& b1 = b->segment().start;
        auto const& b2 = b->segment().end;

        auto result = line_intersection(a1, a2, b1, b2);
        switch (result.type) {
        case IntersectionResult::Coincident:{
            if (equivalent(a1, b2) || equivalent(a2, b1))
                return false;

            bool a1_eq_b1 = equivalent(a1, b1);
            bool a2_eq_b2 = equivalent(a2, b2);

            if (a1_eq_b1 && a2_eq_b2) {
                remove_duplicate_segment(a, b);
                return true;
            }

            bool a1_between = !a1_eq_b1 && is_point_between(a1, b1, b2);
            bool a2_between = !a2_eq_b2 && is_point_between(a2, b1, b2);

            if (a1_eq_b1) {
                if (a2_between) {
                    // (a1)----(a2)
                    // (b1)-----------(b2)
                    split_event(b, a2);

                    // (a1)----(a2)
                    // (b1)----(  )---(b2)
                } else {
                    // (a1)-----------(a2)
                    // (b1)----(b2)
                    split_event(a, b2);

                    // (a1)----(  )---(a2)
                    // (b1)----(b2)
                }

                // In both of the above cases, the segment beginning with (a1) is
                // always duplicated, so we remove that segment
                remove_duplicate_segment(a, b);

                return true;
            }

            if (a1_between) {
                if (!a2_eq_b2) {
                    if (a2_between) {
                        //       (a1)----(a2)
                        // (b1)----------------(b2)
                        split_event(b, a2);

                        //       (a1)----(a2)
                        // (b1)----------(  )--(b2)
                        split_event(b, a1);

                        //       (a1)----(a2)
                        // (b1)--(  )----(  )--(b2)
                    } else {
                        //         (a1)-----------(a2)
                        // (b1)-----------(b2)
                        split_event(a, b1);

                        //         (a1)---(  )----(a2)
                        // (b1)-----------(b2)
                        split_event(b, a1);

                        //         (a1)---(  )----(a2)
                        // (b1)----(  )---(b2)
                    }

                    // In both of the above cases, the segment beginning with (a1) is
                    // always duplicated, so we remove that segment
                    remove_duplicate_segment(a, b);

                    return true;
                }
            }

            return false;
        }
        case IntersectionResult::DoesNotIntersect:
            return false;
        case IntersectionResult::Intersects: {
            bool intersected = false;
            if (!equivalent(result.point, a1) && !equivalent(result.point, a2)) {
                intersected = true;
                split_event(a, result.point);
            }
            if (!equivalent(result.point, b1) && !equivalent(result.point, b2)) {
                intersected = true;
                split_event(b, result.point);
            }

            return intersected;
        }
        default:
            VERIFY_NOT_REACHED();
        }
    };

    Vector<Segment> segments;

    while (!m_event_queue.is_empty()) {
        auto& event = m_event_queue.first();

        if (event->is_start()) {
            auto [above, below] = find_transition(event);

            bool intersected = false;

            if (above)
                intersected = do_intersection(event, *above);
            if (below && !intersected)
                do_intersection(event, *below);

            if (event != m_event_queue.first())
                continue;

            bool toggle;
            if (event->segment().self.below == IsInside::Unknown) {
                toggle = true;
            } else {
                toggle = event->segment().self.above != event->segment().self.below;
            }

            if (below) {
                event->segment().self.below = (*below)->segment().self.above;
            } else {
                event->segment().self.below = m_primary_poly_inverted ? IsInside::Yes : IsInside::No;
            }

            if (toggle) {
                event->segment().self.above = event->segment().self.below == IsInside::Yes ? IsInside::No : IsInside::Yes;
            } else {
                event->segment().self.above = event->segment().self.below;
            }

            m_status.insert_before(below, event);
        } else {

        }
    }
}

void PathClipping::add_event(RefPtr<Event> const& event)
{
    auto it = m_event_queue.find_if([&](auto& curr_event) {
        return *curr_event > *event;
    });

    m_event_queue.insert_before(it, event);
}

/*

function statusFindTransition(event){
  return status_root.findTransition(function(curr_event){
    var comp = statusCompare(event, curr_event.ev);
    return comp > 0; // top to bottom
  });
}

function statusCompare(event, curr_event){
    var a1 = event.seg.start;
    var a2 = event.seg.end;
    var b1 = curr_event.seg.start;
    var b2 = curr_event.seg.end;

    if (pointsCollinear(a1, b1, b2)){
        if (pointsCollinear(a2, b1, b2))
            return 1; // doesn't matter, as long as it's consistent
        return pointAboveOrOnLine(a2, b1, b2) ? 1 : -1;
    }
    return pointAboveOrOnLine(a1, b1, b2) ? 1 : -1;
}
 */

PathClipping::Transition PathClipping::find_transition(RefPtr<Event> const& event)
{
    auto it = m_status.find_if([&](RefPtr<Event> const& curr_event) {
        if (points_are_collinear(event->segment().start, curr_event->segment().start, curr_event->segment().end)) {
            if (points_are_collinear(event->segment().end, curr_event->segment().start, curr_event->segment().end))
                return 1;
            return point_above_or_on_line(event->segment().end, curr_event->segment().start, curr_event->segment().end) ? 1 : -1;
        }
        return point_above_or_on_line(event->segment().start, curr_event->segment().start, curr_event->segment().end) ? 1 : -1;
    });

    return { it.prev(), it };
}

void PathClipping::split_event(RefPtr<Event>& event, Gfx::FloatPoint const& point)
{
    // (a)-------------(d)
    // into...
    // (a)----(b/c)----(d)

    auto a = event;
    auto d = event->other_event();

    Segment first_segment { a->point(), point };
    Segment second_segment { point, d->point() };

    auto b = adopt_ref(*new Event(false, first_segment));
    auto c = adopt_ref(*new Event(true, second_segment));

    // Pair a and b
    a->set_other_event(b);
    a->set_segment(first_segment);
    b->set_other_event(a);

    // Pair c and d
    d->set_other_event(c);
    d->set_segment(second_segment);
    c->set_other_event(d);

    // Add the new events. The old events are already in the event queue
    add_event(b);
    add_event(c);
}

}
