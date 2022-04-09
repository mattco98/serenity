/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/DoublyLinkedList.h>
#include <AK/String.h>
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

// Boolean segment tables

enum class State : u8 {
    Discard,
    FillAbove,
    FillBelow,
};

// Turn off clang-format here so we can have 4 entries per row, otherwise
// these tables take up a lot of space
// clang-format off
constexpr static State union_states[] = {
    State::Discard,   State::FillBelow, State::FillAbove, State::Discard,
    State::FillBelow, State::FillBelow, State::Discard,   State::Discard,
    State::FillAbove, State::Discard,   State::FillAbove, State::Discard,
    State::Discard,   State::Discard,   State::Discard,   State::Discard,
};

constexpr static State intersect_states[] = {
    State::Discard, State::Discard,   State::Discard,   State::Discard,
    State::Discard, State::FillBelow, State::Discard,   State::FillBelow,
    State::Discard, State::Discard,   State::FillAbove, State::FillAbove,
    State::Discard, State::FillBelow, State::FillAbove, State::Discard,
};

constexpr static State difference_states[] = {
    State::Discard,   State::Discard,   State::Discard,   State::Discard,
    State::FillBelow, State::Discard,   State::FillBelow, State::Discard,
    State::FillAbove, State::FillAbove, State::Discard,   State::Discard,
    State::Discard,   State::FillAbove, State::FillBelow, State::Discard,
};

constexpr static State difference_reversed_states[] = {
    State::Discard, State::FillBelow, State::FillAbove, State::Discard,
    State::Discard, State::Discard,   State::FillAbove, State::FillAbove,
    State::Discard, State::FillBelow, State::Discard,   State::FillBelow,
    State::Discard, State::Discard,   State::Discard,   State::Discard,
};

constexpr static State xor_states[] = {
    State::Discard,   State::FillBelow, State::FillAbove, State::Discard,
    State::FillBelow, State::Discard,   State::Discard,   State::FillAbove,
    State::FillAbove, State::Discard,   State::Discard,   State::FillBelow,
    State::Discard,   State::FillAbove, State::FillBelow, State::Discard,
};
// clang-format on

static size_t segment_state_index(PathClipping::Segment const& segment)
{
    size_t index = 0;
    if (segment.self.above == IsInside::Yes)
        index += 8;
    if (segment.self.below == IsInside::Yes)
        index += 4;
    if (segment.other.above == IsInside::Yes)
        index += 2;
    if (segment.other.below == IsInside::Yes)
        index += 1;
    return index;
}

static State const* table_for_clip_type(PathClipping::ClipType clip_type)
{
    switch (clip_type) {
    case PathClipping::ClipType::Intersection:
        return intersect_states;
    case PathClipping::ClipType::Union:
        return union_states;
    case PathClipping::ClipType::Difference:
        return difference_states;
    case PathClipping::ClipType::DifferenceReversed:
        return difference_reversed_states;
    case PathClipping::ClipType::Xor:
        return xor_states;
    }

    VERIFY_NOT_REACHED();
}

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

struct PathClipping::Event : public RefCounted<PathClipping::Event> {
    bool is_start;
    bool is_primary;
    PathClipping::Segment segment;
    RefPtr<Event> other_event;

    Event(bool is_start, bool is_primary, PathClipping::Segment const& segment, RefPtr<Event> const& other_event)
        : is_start(is_start)
        , is_primary(is_primary)
        , segment(segment)
        , other_event(other_event)
    {
    }

    FloatPoint const& point() const { return is_start ? segment.start : segment.end; }
    FloatPoint const& other_point() const { return is_start ? segment.end : segment.start; }

    void update_other_segment() { other_event->segment = segment; }

    // Determines the ordering of this event in the status queue relative to another event,
    // i.e., which event should be processed first.
    int operator<=>(Event const& event) const
    {
        auto comp = point() <=> event.point();
        if (comp != 0) {
            // Different target points makes this easy, whichever event has the left-mose
            // starting point should be processed first.
            return comp;
        }

        if (other_point() == event.other_point()) {
            // Both ends of both events are the same, so it doesn't matter. These events
            // will eventually be combined to produce a single segment.
            return 0;
        }

        if (is_start != event.is_start) {
            // Prefer sorting end events first
            return is_start ? 1 : -1;
        }

        // Determine which event line is above the event
        return point_above_or_on_line(
                   other_point(),
                   event.is_start ? event.point() : event.other_point(),
                   event.is_start ? event.other_point() : event.point())
            ? 1
            : -1;
    }
};

Vector<Path> PathClipping::clip(Path& a, Path& b, PathClipping::ClipType clip_type)
{
    if (clip_type == PathClipping::ClipType::DifferenceReversed)
        return clip(b, a, PathClipping::ClipType::Difference);

    a.close_all_subpaths();
    b.close_all_subpaths();

    auto poly_a = convert_to_polygon(a, true);
    auto poly_b = convert_to_polygon(b, false);
    auto combined = combine(poly_a, poly_b);
    return select_segments(combined, clip_type);
}

PathClipping::Polygon PathClipping::convert_to_polygon(Path& path, bool is_primary)
{
    PathClipping processor(false);

    auto split_lines = path.split_lines();

    for (auto& split_line : split_lines) {
        auto is_forward = split_line.from <=> split_line.to;
        if (is_forward == 0)
            continue;

        auto from = split_line.from;
        auto to = split_line.to;
        if (is_forward > 0)
            swap(from, to);

        processor.add_segment({ from, to }, is_primary);
    }

    return processor.create_polygon();
}
PathClipping::Polygon PathClipping::combine(Polygon const& a, Polygon const& b)
{
    PathClipping processor(true);

    for (auto& segment : a)
        processor.add_segment(segment, true);
    for (auto& segment : b)
        processor.add_segment(segment, false);

    return processor.create_polygon();
}

PathClipping::Polygon PathClipping::clip_polygon(Polygon const& input_polygon, ClipType clip_type)
{
    Polygon output_polygon;
    auto clip_table = table_for_clip_type(clip_type);

    for (auto& segment : input_polygon) {
        auto table_index = segment_state_index(segment);
        auto state = clip_table[table_index];

        if (state == State::Discard)
            continue;

        output_polygon.append(Segment {
            segment.start,
            segment.end,
            Annotation {
                state == State::FillAbove ? IsInside::Yes : IsInside::No,
                state == State::FillBelow ? IsInside::Yes : IsInside::No,
            } });
    }

    return output_polygon;
}

Vector<Path> PathClipping::convert_to_path(Polygon& polygon)
{
    Vector<Vector<FloatPoint>> chains;
    Vector<Path> paths;

    auto reverse_chain = [&](size_t chain_index) {
        chains[chain_index] = chains[chain_index].reversed();
    };

    auto merge_chains = [&](size_t first_chain_index, size_t second_chain_index) {
        auto& first_chain = chains[first_chain_index];
        auto& second_chain = chains[second_chain_index];
        first_chain.extend(move(second_chain));
        chains.remove(second_chain_index);
    };

    auto finalize_chain = [&](size_t chain_index) {
        auto chain = chains[chain_index];
        chains.remove(chain_index);
        if (chain.size() <= 3) {
            // This chain has no area. A chain has to have at least 4 points to contain
            // area, since chains always end with the same point they start with. We can
            // just ignore this chain
            return;
        }
        Path path;
        path.move_to(chain[0]);
        for (size_t i = 1; i < chain.size(); i++)
            path.line_to(chain[i]);
        paths.append(move(path));
    };

    struct Match {
        size_t index;
        bool matches_start_of_chain;
        bool matches_start_of_segment;
    };

    for (auto& segment : polygon) {
        for (auto& chain : chains) {
            StringBuilder builder;
            builder.appendff("{}", chain[0]);
            for (size_t i = 1; i < chain.size(); i++)
                builder.appendff(" --> {}", chain[i]);
        }

        Optional<Match> maybe_first_match;
        Optional<Match> maybe_second_match;

        auto set_match = [&](size_t index, bool matches_start_of_chain, bool matches_start_of_segment) {
            Match match { index, matches_start_of_chain, matches_start_of_segment };

            if (!maybe_first_match.has_value()) {
                maybe_first_match = match;
                return false;
            }

            maybe_second_match = match;
            return true;
        };

        for (size_t i = 0; i < chains.size(); i++) {
            auto& chain = chains[i];
            auto& head = chain.first();
            auto& tail = chain.last();

            if (equivalent(head, segment.start)) {
                if (set_match(i, true, true))
                    break;
            } else if (equivalent(head, segment.end)) {
                if (set_match(i, true, false))
                    break;
            } else if (equivalent(tail, segment.start)) {
                if (set_match(i, false, true))
                    break;
            } else if (equivalent(tail, segment.end)) {
                if (set_match(i, false, false))
                    break;
            }
        }

        if (!maybe_first_match.has_value()) {
            // No matches, make a new chain
            Vector<FloatPoint> new_chain;
            new_chain.append(segment.start);
            new_chain.append(segment.end);
            chains.append(move(new_chain));
            continue;
        }

        if (!maybe_second_match.has_value()) {
            // One match

            auto& match = maybe_first_match.value();
            auto& chain = chains[match.index];
            auto& point_to_append = match.matches_start_of_segment ? segment.end : segment.start;
            auto opposite_point = match.matches_start_of_chain ? chain.last() : chain.first();

            if (match.matches_start_of_chain) {
                chain.prepend(point_to_append);
            } else {
                chain.append(point_to_append);
            }

            if (equivalent(point_to_append, opposite_point)) {
                // this chain is closing
                finalize_chain(match.index);
            }

            continue;
        }

        // Two matches, we have to join two chains

        auto& first_match = maybe_first_match.value();
        auto& second_match = maybe_second_match.value();

        auto first_match_index = first_match.index;
        auto second_match_index = second_match.index;

        auto reverse_first_chain = chains[first_match_index].size() < chains[second_match_index].size();

        if (first_match.matches_start_of_chain) {
            if (second_match.matches_start_of_chain) {
                if (reverse_first_chain) {
                    reverse_chain(first_match_index);
                    merge_chains(first_match_index, second_match_index);
                } else {
                    reverse_chain(second_match_index);
                    merge_chains(second_match_index, first_match_index);
                }
            } else {
                merge_chains(second_match_index, first_match_index);
            }
        } else {
            if (second_match.matches_start_of_chain) {
                merge_chains(first_match_index, second_match_index);
            } else {
                if (reverse_first_chain) {
                    reverse_chain(first_match_index);
                    merge_chains(second_match_index, first_match_index);
                } else {
                    reverse_chain(second_match_index);
                    merge_chains(first_match_index, second_match_index);
                }
            }
        }
    }

    VERIFY(chains.is_empty());
    return paths;
}

Vector<Path> PathClipping::select_segments(Polygon const& input_polygon, ClipType clip_type)
{
    auto output_polygon = clip_polygon(input_polygon, clip_type);
    return convert_to_path(output_polygon);
}

PathClipping::PathClipping(bool is_combining_phase)
    : m_is_combining_phase(is_combining_phase)
{
}

PathClipping::Polygon PathClipping::create_polygon()
{
    Polygon polygon;

    auto do_event_intersections = [&](RefPtr<Event>& event, RefPtr<Event>& other) {
        auto result = intersect_events(event, other);
        if (result) {
            // event is the same as other
            if (m_is_combining_phase) {
                result->segment.other.above = event->segment.self.above;
                result->segment.other.below = event->segment.self.below;
                result->update_other_segment();
            } else {
                bool toggle;
                if (event->segment.self.below == IsInside::Unknown) {
                    toggle = true;
                } else {
                    toggle = event->segment.self.above != event->segment.self.below;
                }

                if (toggle) {
                    auto fill_above = result->segment.self.above;
                    VERIFY(fill_above != IsInside::Unknown);
                    result->segment.self.above = fill_above == IsInside::Yes ? IsInside::No : IsInside::Yes;
                    result->update_other_segment();
                }
            }

            dbgln("removing {}", event);
            m_event_queue.remove(event);
            m_event_queue.remove(event->other_event);

            return true;
        }

        return false;
    };

    while (!m_event_queue.is_empty()) {
        auto& event = m_event_queue.first();

        if (event->is_start) {
            auto find_result = find_transition(event);

            RefPtr<Event> event_above;
            RefPtr<Event> event_below;

            if (!find_result.is_end()) {
                auto prev = find_result.prev();
                auto next = find_result;
                if (prev)
                    event_above = *prev;
                if (next)
                    event_below = *next;
            } else if (!m_status_stack.is_empty()) {
                event_above = m_status_stack.last();
            }

            auto intersected = false;
            if (event_above)
                intersected = do_event_intersections(event, event_above);

            if (!intersected && event_below)
                do_event_intersections(event, event_below);

            // In the case of intersection, events will have been added to the
            // event queue. They may need to be processed before this event.
            if (m_event_queue.first() != event) {
                // An event has been inserted before the current event being processed
                continue;
            }

            if (m_is_combining_phase) {
                if (event->segment.other.above == IsInside::Unknown) {
                    IsInside inside;
                    if (!event_below) {
                        inside = IsInside::No;
                    } else if (event->is_primary == event_below->is_primary) {
                        inside = event_below->segment.other.above;
                        VERIFY(inside != IsInside::Unknown);
                    } else {
                        inside = event_below->segment.self.above;
                        VERIFY(inside != IsInside::Unknown);
                    }

                    event->segment.other.above = inside;
                    event->segment.other.below = inside;
                    event->update_other_segment();
                }
            } else {
                bool toggle;
                if (event->segment.self.below == IsInside::Unknown) {
                    toggle = true;
                } else {
                    toggle = event->segment.self.above != event->segment.self.below;
                }

                if (!event_below) {
                    event->segment.self.below = IsInside::No;
                } else {
                    event->segment.self.below = event_below->segment.self.above;
                }

                if (toggle) {
                    auto fill_below = event->segment.self.below;
                    VERIFY(fill_below != IsInside::Unknown);
                    event->segment.self.above = fill_below == IsInside::Yes ? IsInside::No : IsInside::Yes;
                } else {
                    event->segment.self.above = event->segment.self.below;
                }

                event->update_other_segment();
            }

            m_status_stack.insert_before(find_result, event);
        } else {
            auto existing_status = m_status_stack.find(event->other_event);
            if (existing_status.prev() && existing_status.next()) {
                do_event_intersections(*existing_status.prev(), *existing_status.next());
            }

            m_status_stack.remove(existing_status);

            if (m_is_combining_phase && !event->is_primary) {
                // Swap fill info for secondary polygon
                swap(event->segment.self.above, event->segment.other.above);
                swap(event->segment.self.below, event->segment.other.below);
                event->update_other_segment();
            }

            polygon.append(event->segment);
        }

        (void)m_event_queue.take_first();
    }

    return polygon;
}

void PathClipping::add_segment(PathClipping::Segment const& segment, bool is_primary)
{
    auto start = adopt_ref(*new Event(true, is_primary, segment, {}));
    auto end = adopt_ref(*new Event(false, is_primary, segment, start));
    start->other_event = end;

    add_event(start);
    add_event(end);
}

DoublyLinkedList<RefPtr<PathClipping::Event>>::Iterator PathClipping::find_transition(RefPtr<Event> event)
{
    auto a1 = event->segment.start;
    auto a2 = event->segment.end;

    auto r = m_status_stack.find_if([&](RefPtr<Event> const& other_event) {
        auto b1 = other_event->segment.start;
        auto b2 = other_event->segment.end;

        if (points_are_collinear(a1, b1, b2)) {
            if (points_are_collinear(a2, b1, b2))
                return true;
            return point_above_or_on_line(a2, b1, b2);
        }
        return point_above_or_on_line(a1, b1, b2);
    });

    return r;
}

RefPtr<PathClipping::Event> PathClipping::intersect_events(RefPtr<Event>& a, RefPtr<Event>& b)
{
    auto share_point = equivalent(a->point(), b->point());
    auto share_other_point = equivalent(a->other_point(), b->other_point());

    if (share_point && share_other_point)
        return b;

    if (share_point || share_other_point || equivalent(a->point(), b->other_point()) || equivalent(a->other_point(), b->point())) {
        // The segments are joined at one end, so we just ignore this and process
        // them as two normal segments
        return {};
    }

    auto intersection_result = line_intersection(a->segment.start, a->segment.end, b->segment.start, b->segment.end);

    if (intersection_result.type == IntersectionResult::Intersects) {
        auto split_point = intersection_result.point;

        auto split_a = !equivalent(split_point, a->point()) && !equivalent(split_point, a->other_point());
        auto split_b = !equivalent(split_point, b->point()) && !equivalent(split_point, b->other_point());

        if (split_a)
            split_event(a, split_point);

        if (split_b)
            split_event(b, split_point);
    } else if (intersection_result.type == IntersectionResult::Coincident) {
        auto a1_eq_b1 = equivalent(a->point(), b->point());
        auto a2_eq_b2 = equivalent(a->other_point(), b->other_point());

        if (a1_eq_b1 && a2_eq_b2) {
            // The segments are equal
            return b;
        }

        auto a1_between = !a1_eq_b1 && is_point_between(a->point(), b->point(), b->other_point());
        auto a2_between = !a2_eq_b2 && is_point_between(a->other_point(), b->point(), b->other_point());

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
            return b;
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

    return {};
}

void PathClipping::split_event(RefPtr<Event>& event, FloatPoint const& point_to_split_at)
{
    // from:
    //      (start)----------------(end)
    // to:
    //     (start1)---------(x)----(end2)
    //
    // where (x) is point_to_split_at
    //
    // Note: point_to_split_at must lie on the event segment line. This is the
    //       responsibility of the caller
    //
    // Note: We _must_ mutate event here instead of removing it and adding a new
    //       event. This is because the event can currently be in the status queue,
    //       and we don't want to have to bother with detecting whether that is the
    //       case or not.

    VERIFY(event->is_start);

    m_event_queue.remove(event->other_event);

    PathClipping::Segment new_segment {
        point_to_split_at,
        event->segment.end,
        Annotation { event->segment.self.above, event->segment.self.below }
    };
    event->segment.end = point_to_split_at;
    auto first_segment_end = adopt_ref(*new Event { !event->is_start, event->is_primary, event->segment, event });
    event->other_event = first_segment_end;
    add_event(first_segment_end);

    add_segment(new_segment, event->is_primary);
}

void PathClipping::add_event(RefPtr<Event> const& event)
{
    auto insertion_location = m_event_queue.find_if([&](RefPtr<Event> const& a) {
        return *event < *a;
    });

    m_event_queue.insert_before(insertion_location, event);
}

}

namespace AK {

template<>
struct Formatter<Gfx::PathClipping::Segment> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Gfx::PathClipping::Segment const& segment)
    {
        String self = "???";
        String other = "???";

        if (segment.self.above != Gfx::IsInside::Unknown || segment.self.below != Gfx::IsInside::Unknown)
            self = String::formatted("{} {}", segment.self.above, segment.self.below);
        if (segment.other.above != Gfx::IsInside::Unknown || segment.other.below != Gfx::IsInside::Unknown)
            other = String::formatted("{} {}", segment.other.above, segment.other.below);

        auto str = String::formatted("{{ [{}, {}] self={} other={} }}", segment.start, segment.end, self, other);
        TRY(Formatter<StringView>::format(builder, str));

        return {};
    }
};

template<>
struct Formatter<Gfx::IntersectionResult> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Gfx::IntersectionResult const& result)
    {
        char const* type;
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

        TRY(Formatter<StringView>::format(builder, String::formatted("{{ type={} point={} }}", type, result.point)));
        return {};
    }
};

template<>
struct Formatter<Gfx::PathClipping::ClipType> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Gfx::PathClipping::ClipType const& result)
    {
        switch (result) {
        case Gfx::PathClipping::ClipType::Intersection:
            TRY(Formatter<StringView>::format(builder, "Intersection"));
            break;
        case Gfx::PathClipping::ClipType::Union:
            TRY(Formatter<StringView>::format(builder, "Union"));
            break;
        case Gfx::PathClipping::ClipType::Difference:
            TRY(Formatter<StringView>::format(builder, "Difference"));
            break;
        case Gfx::PathClipping::ClipType::DifferenceReversed:
            TRY(Formatter<StringView>::format(builder, "DifferenceReversed"));
            break;
        case Gfx::PathClipping::ClipType::Xor:
            TRY(Formatter<StringView>::format(builder, "Xor"));
            break;
        }

        return {};
    }
};

template<>
struct Formatter<Gfx::PathClipping::Event> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Gfx::PathClipping::Event const& event)
    {
        TRY(Formatter<StringView>::format(builder, String::formatted("{{ segment={}{}{} }}", event.segment, event.is_start ? ", start" : "", event.is_primary ? ", primary" : "")));
        return {};
    }
};

template<>
struct Formatter<Gfx::IsInside> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Gfx::IsInside state)
    {
        switch (state) {
        case Gfx::IsInside::Yes:
            TRY(Formatter<StringView>::format(builder, "Yes"));
            break;
        case Gfx::IsInside::No:
            TRY(Formatter<StringView>::format(builder, "No"));
            break;
        case Gfx::IsInside::Unknown:
            TRY(Formatter<StringView>::format(builder, "Unknown"));
            break;
        default:
            VERIFY_NOT_REACHED();
        }

        return {};
    }
};

}
