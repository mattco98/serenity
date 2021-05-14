/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/PathClipping.h>

namespace Gfx {

static constexpr float EPSILON = 0.0001f;

template<typename T>
int operator<=>(const Point<T>& a, const Point<T>& b)
{
    if (a.x() == b.x()) {
        if (a.y() == b.y())
            return 0;
        auto diff = b.y() - a.y();
        if (fabsf(diff) < EPSILON)
            return 0;
        return diff;
    }
    auto diff = b.x() - a.x();
    if (fabsf(diff) < EPSILON)
        return 0;
    return diff;
}

struct IntersectionResult {
    enum Type {
        Coincident,
        Intersects,
        DoesNotIntersect,
    };

    Type type;
    FloatPoint point;
};

// All three points should be collinear
bool is_point_between(const FloatPoint& point, const FloatPoint& a, const FloatPoint& b);
bool is_point_between(const FloatPoint& point, const FloatPoint& a, const FloatPoint& b)
{
    VERIFY(a < b);
    if (a.x() == b.x()) {
        // This is a vertical line
        return point.y() >= a.y() && point.y() <= b.y();
    }

    return point.x() >= a.x() && point.x() <= b.x();
}

float line_slope(const FloatPoint& a, const FloatPoint& b);
float line_slope(const FloatPoint& a, const FloatPoint& b)
{
    return (b.y() - a.y()) / (b.x() - a.x());
}

bool points_are_equivalent(const FloatPoint& a, const FloatPoint& b);
bool points_are_equivalent(const FloatPoint& a, const FloatPoint& b)
{
    return (a <=> b) == 0;
}

bool points_are_collinear(const FloatPoint& a, const FloatPoint& b, const FloatPoint& c);
bool points_are_collinear(const FloatPoint& a, const FloatPoint& b, const FloatPoint& c)
{
    if (points_are_equivalent(a, b) || points_are_equivalent(b, c) || points_are_equivalent(a, c))
        return true;
    return fabsf(line_slope(a, b) - line_slope(b, c)) < EPSILON;
}

bool point_above_or_on_line(const FloatPoint& point, const FloatPoint& a, const FloatPoint& b);
bool point_above_or_on_line(const FloatPoint& point, const FloatPoint& a, const FloatPoint& b)
{
    auto slope = line_slope(a, b);
    auto intersecting_y = (point.x() - a.x()) * slope + a.y();
    return point.y() <= intersecting_y;
}

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line_segment
IntersectionResult line_intersection(const FloatPoint& a1, const FloatPoint& a2, const FloatPoint& b1, const FloatPoint& b2);
IntersectionResult line_intersection(const FloatPoint& a1, const FloatPoint& a2, const FloatPoint& b1, const FloatPoint& b2)
{
    auto get_nonintersection_result = [&] {
        auto slope1 = line_slope(a1, a2);
        auto slope2 = line_slope(b1, b2);

        if (fabsf(slope1 - slope2) < EPSILON) {
            if (fabsf(a1.x() - b1.x()) < EPSILON && abs(a1.y() - b1.y()) < EPSILON)
                return IntersectionResult { IntersectionResult::Coincident, {} };
        }

        return IntersectionResult { IntersectionResult::DoesNotIntersect, {} };
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

    auto t_numerator = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
    auto u_numerator = (x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3);

    // We can test if the intersection point is on the line before we do the division
    // The following must be true:
    //     0 <= t <= 1, 0 <= u <= 1
    //
    // Simplification:
    //     t = t_numerator / denominator
    //     u = u_numerator / denominator
    //  =>
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

ALWAYS_INLINE IntersectionResult line_intersection(const PathClipping::Segment& a, const PathClipping::Segment& b)
{
    return line_intersection(a.start, a.end, b.start, b.end);
}

struct PathClipping::Event : public RefCounted<PathClipping::Event> {
    bool is_start;
    Segment segment;
    RefPtr<Event> other_event;

    Event(bool is_start, const Segment& segment, const RefPtr<Event>& other_event)
        : is_start(is_start)
        , segment(segment)
        , other_event(other_event)
    {
    }

    const FloatPoint& point() const { return is_start ? segment.start : segment.end; }
    const FloatPoint& other_point() const { return is_start ? segment.end : segment.start; }

    int operator<=>(const Event& event) const
    {
        auto comp = point() <=> event.point();
        if (comp != 0) {
            // Different target points makes this easy
            return comp;
        }

        if (other_point() == event.other_point()) {
            // Both ends of both events are the same, so it doesn't matter
            return 0;
        }

        if (is_start != event.is_start) {
            // Prefer sorting end events first
            return is_start ? 1 : -1;
        }

        // Determine which event line is above the other

        auto line_start = point();
        auto line_end = other_point();
        auto other_point = event.other_point();

        if (!is_start)
            swap(line_start, line_end);

        auto slope = (line_end.y() - line_start.y()) / (line_end.x() - line_start.x());
        auto projected_y = line_start.y() + slope * (other_point.x() - line_start.x());

        // FIXME: Wrong order?
        return projected_y > other_point.y() ? 1 : -1;
    }
};

Vector<PathClipping::Segment> PathClipping::clip(ClipType clip_type, Path& a, Path& b)
{
    a.close_all_subpaths();
    b.close_all_subpaths();

    return PathClipping(clip_type, a, b).clip();
}

PathClipping::PathClipping(ClipType clip_type, Path& a, Path& b)
    : m_clip_type(clip_type)
    , m_a(a)
    , m_b(b)
{
    dbgln("Path1: {}", a);
    dbgln("Path2: {}", b);
}

Vector<PathClipping::Segment> PathClipping::clip()
{
    Vector<Segment> segments;

    dbgln("[clip] adding regions...");

    add_region(m_a);
    add_region(m_b);

    dbgln("[clip] regions added");
    dbgln();

    dbgln("[clip] events:");
    size_t i = 0;
    for (auto& event : m_event_queue)
        dbgln("[clip]   event{} = {}", i++, *event);
    dbgln();

    auto do_event_intersections = [&](auto& event, auto& other) {
        if (intersect_events(event, other)) {
            // event is the same as other
            m_event_queue.remove(event);
            m_event_queue.remove(event->other_event);
            // TODO: Update event_before's annotations
            return true;
        }
        return false;
    };

    while (!m_event_queue.is_empty()) {
        auto& event = m_event_queue.first();

        dbgln();
        dbgln("status stack:");
        for (auto& event1 : m_status_stack)
            dbgln("  {}", *event1);
        dbgln();

        dbgln("[clip] processing event {}", *event);

        if (event->is_start) {
            auto find_result = find_transition(event);

            RefPtr<Event> event_before;
            RefPtr<Event> event_after;

            if (!find_result.is_end()) {
                auto prev = find_result.prev();
                auto next = find_result;
                if (prev) {
                    dbgln("[clip]   event has a previous event");
                    event_before = *prev;
                }
                if (next) {
                    dbgln("[clip]   event has a next event");
                    event_after = *next;
                }
            }

            if (event_before && do_event_intersections(event, event_before)) {
                dbgln("[clip]   intersected with previous event");
                continue;
            }

            if (event_after && do_event_intersections(event, event_after)) {
                dbgln("[clip]   intersected with next event");
                continue;
            }

            // In the case of intersection, events will have been added to the
            // event queue. They may need to be processed before this event.
            if (m_event_queue.first() != event) {
                // An event has been inserted before the current event being processed
                continue;
            }

            // FIXME: Calculate fill annotations

            dbgln("[clip]   inserting event");
            m_status_stack.insert_before(find_result, event);

            // FIXME: Set status on event->other_event?
        } else {
            auto existing_status = m_status_stack.find(event->other_event);
            if (existing_status.prev() && existing_status.next()) {
                do_event_intersections(*existing_status.prev(), *existing_status.next());
            }

            dbgln("[clip]   removing event");
            m_status_stack.remove(existing_status);

            segments.append(event->segment);
        }

        m_event_queue.take_first();
    }

    return segments;
}

void PathClipping::add_region(Path& path)
{
    auto split_lines = path.split_lines();
    dbgln("[add_region] adding path...");

    for (auto& split_line : split_lines) {
        auto is_forward = split_line.from <=> split_line.to;
        dbgln("[add_region]   split_line={} is_forward={}", split_line, is_forward);
        if (is_forward == 0)
            continue;

        auto from = split_line.from;
        auto to = split_line.to;
        if (is_forward < 0)
            swap(from, to);

        add_segment({ from, to });
    }

    dbgln("[add_region] path added");
}

void PathClipping::add_segment(const Segment& segment)
{
    auto start = adopt_ref(*new Event(true, segment, {}));
    auto end = adopt_ref(*new Event(false, segment, start));
    start->other_event = end;

    add_event(start);
    add_event(end);
}

DoublyLinkedList<RefPtr<PathClipping::Event>>::Iterator PathClipping::find_transition(RefPtr<Event> event)
{
    auto a1 = event->segment.start;
    auto a2 = event->segment.end;
    dbgln("[find_transition] finding transition (a1={} a2={})", a1, a2);

    size_t offset = 0;

    auto r = m_status_stack.find_if([&](const RefPtr<Event>& other_event) {
        offset++;
        auto b1 = other_event->segment.start;
        auto b2 = other_event->segment.end;
        dbgln("[find_transition]   candidate event (b1={} b2={})", b1, b2);

        if (points_are_collinear(a1, b1, b2)) {
            dbgln("[find_transition]     a1 is collinear with b1->b2");
            if (points_are_collinear(a2, b1, b2)) {
                dbgln("[find_transition]     a2 is collinear with b1->b2");
                return true;
            }
            if (point_above_or_on_line(a2, b1, b2)) {
                dbgln("[find_transition]     a2 is above/on b1->b2");
            } else {
                dbgln("[find_transition]     a2 is NOT above/on b1->b2");
            }
            return point_above_or_on_line(a2, b1, b2);
        }
        if (point_above_or_on_line(a1, b1, b2)) {
            dbgln("[find_transition]     a1 is above/on b1->b2");
        } else {
            dbgln("[find_transition]     a1 is NOT above/on b1->b2");
        }
        return point_above_or_on_line(a1, b1, b2);
    });

    dbgln("[find_transition]   transition found at offset {} (end={})", offset == 0 ? offset : offset - 1, r.is_end());

    return r;
}

bool PathClipping::intersect_events(RefPtr<Event>& a, RefPtr<Event>& b)
{
    dbgln("[intersect_event] intersecting {} and {}", *a, *b);
    auto share_points = points_are_equivalent(a->point(), b->point()) ||
        points_are_equivalent(a->point(), b->other_point()) ||
        points_are_equivalent(a->other_point(), b->point()) ||
        points_are_equivalent(a->other_point(), b->other_point());

    if (share_points) {
        // The segments are joined at one end, so we just ignore this and process
        // them as two normal segments
        dbgln("[intersect_event]   segments are joined at ends");
        return false;
    }

    auto intersection_result = line_intersection(a->segment.start, a->segment.end, b->segment.start, b->segment.end);
    dbgln("[intersect_event]   intersection_result={}", intersection_result);

    if (intersection_result.type == IntersectionResult::Intersects) {
        auto split_point = intersection_result.point;

        // Use <=> to take some epsilon into account
        auto split_a = (split_point <=> a->point()) != 0 && (split_point <=> a->other_point()) != 0;
        auto split_b = (split_point <=> b->point()) != 0 && (split_point <=> b->other_point()) != 0;

        dbgln("[intersect_event]   split_a={} split_b={}", split_a, split_b);

        if (split_a)
            split_event(a, split_point);

        if (split_b)
            split_event(b, split_point);
    } else if (intersection_result.type == IntersectionResult::Coincident) {
        auto a1_eq_b1 = (a->point() <=> b->point()) == 0;
        auto a2_eq_b2 = (a->other_point() <=> b->other_point()) == 0;

        dbgln("[intersect_event]   a1_eq_b1={} a2_eq_b2={}", a1_eq_b1, a2_eq_b2);

        if (a1_eq_b1 && a2_eq_b2) {
            // The segments are equal
            return true;
        }

        auto a1_between = !a1_eq_b1 && is_point_between(a->point(), b->point(), b->other_point());
        auto a2_between = !a2_eq_b2 && is_point_between(a->other_point(), b->point(), b->other_point());

        dbgln("[intersect_event]   a1_between={} a2_between={}", a1_between, a2_between);

        if (a1_eq_b1) {
            if (a2_between) {
                // event1: (a1)---(a2)
                // event2: (b1)---------(b2)
                split_event(b, a->other_point());
            } else {
                // event1: (a1)-----------(a2)
                // event2: (b1)---(b2)
                split_event(a, b->other_point());
            }

            // During the split, one of the segments is the same as event a, so
            // we return true to indicate this event is redundant
            return true;
        } else if (a1_between) {
            if (!a2_eq_b2) {
                // Make a2 equal to b2
                if (a2_between) {
                    // event1:        (a1)-----(a2)
                    // event2: (b1)-------------------(b2)
                    split_event(b, a->other_point());
                } else {
                    // event1:      (a1)---------(a2)
                    // event2: (b1)--------(b2)
                    split_event(a, b->other_point());
                }

                // event1:      (a1)---(a2)
                // event2: (b1)--------(b2)
                split_event(b, a->point());
            }
        }
    }

    return false;
}

void PathClipping::split_event(RefPtr<Event>& event, const FloatPoint& point_to_split_at)
{
    dbgln("[split_event] splitting {} at point {}", *event, point_to_split_at);
    // from:
    //      (start)---------(s)----(end)
    // to:
    //     (start1)---------(x)----(end2)
    //
    // s = point_to_split_at, x = end1 and start2
    //
    // Note: point_to_split_at must lie on the event segment line. This is the
    //       responsibility of the caller

    VERIFY(event->is_start);

    dbgln("[split_event]   removing {}", *event->other_event);
    m_event_queue.remove(m_event_queue.find(event->other_event));
    auto end = event->segment.end;
    event->segment.end = point_to_split_at;
    auto first_segment_end = adopt_ref(*new Event { false, event->segment, event });
    add_event(first_segment_end);

    Segment second_segment { point_to_split_at, end };
    auto second_segment_start = adopt_ref(*new Event { true, second_segment, {} });
    auto second_segment_end = adopt_ref(*new Event { false, second_segment, second_segment_start });
    second_segment_start->other_event = second_segment_end;

    add_event(second_segment_start);
    add_event(second_segment_end);
}

void PathClipping::add_event(const RefPtr<Event>& event)
{
    size_t offset = 0;
    auto insertion_location = m_event_queue.find_if([&](const RefPtr<Event>& a) {
        offset++;
        return *event >= *a;
    });

    dbgln("[add_event] adding event {} at offset {} (size={})", *event, offset, m_event_queue.size_slow());

    m_event_queue.insert_before(insertion_location, event);
}

}

namespace AK {

template<>
struct Formatter<Gfx::PathClipping::Event> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::PathClipping::Event& event)
    {
        Formatter<StringView>::format(builder, String::formatted("{{ segment={} is_start={} }}", event.segment, event.is_start));
    }
};

template<>
struct Formatter<Gfx::IntersectionResult> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::IntersectionResult& result)
    {
        const char* type;
        switch (result.type) {
        case Gfx::IntersectionResult::Coincident:
            type = "Coincident";
            break;
        case Gfx::IntersectionResult::Intersects:
            type = "Intersects";
            break;
        case Gfx::IntersectionResult::DoesNotIntersect:
            type = "DoesNotIntersect";
            break;
        }

        Formatter<StringView>::format(builder, String::formatted("{{ type={} point={} }}", type, result.point));
    }
};

}
