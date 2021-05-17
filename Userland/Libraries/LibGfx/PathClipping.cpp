/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/PathClipping.h>

#define DEBUG_PATH_CLIPPING 0
#define dbg(...) dbgln_if(DEBUG_PATH_CLIPPING, __VA_ARGS__)

namespace Gfx {

static constexpr float EPSILON = 0.0001f;

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

static size_t segment_state_index(const PathClipping::Segment& segment)
{
    size_t index = 0;
    if (segment.self_fill_above == TriState::True)
        index += 8;
    if (segment.self_fill_below == TriState::True)
        index += 4;
    if (segment.other_fill_above == TriState::True)
        index += 2;
    if (segment.other_fill_below == TriState::True)
        index += 1;
    return index;
}

static const State* table_for_clip_type(ClipType clip_type)
{
    switch (clip_type) {
    case ClipType::Intersection:
        return intersect_states;
    case ClipType::Union:
        return union_states;
    case ClipType::Difference:
        return difference_states;
    case ClipType::DifferenceReversed:
        return difference_reversed_states;
    case ClipType::Xor:
        return xor_states;
    }

    VERIFY_NOT_REACHED();
}

static bool equivalent(float a, float b)
{
    return fabsf(a - b) < EPSILON;
}

static bool equivalent(const FloatPoint& a, const FloatPoint& b)
{
    return equivalent(a.x(), b.x()) && equivalent(a.y(), b.y());
}

template<typename T>
static int operator<=>(const Point<T>& a, const Point<T>& b)
{
    if (equivalent(a.x(), b.x())) {
        if (equivalent(a.y(), b.y()))
            return 0;
        return a.y() < b.y() ? -1 : 1;
    }
    return a.x() < b.x() ? -1 : 1;
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
static bool is_point_between(const FloatPoint& point, const FloatPoint& a, const FloatPoint& b)
{
    VERIFY(a < b);
    if (equivalent(a.x(), b.x())) {
        // This is a vertical line
        return point.y() >= a.y() && point.y() <= b.y();
    }

    return point.x() >= a.x() && point.x() <= b.x();
}

static float line_slope(const FloatPoint& a, const FloatPoint& b)
{
    return (b.y() - a.y()) / (b.x() - a.x());
}

static bool points_are_collinear(const FloatPoint& a, const FloatPoint& b, const FloatPoint& c)
{
    if (equivalent(a, b) || equivalent(b, c) || equivalent(a, c))
        return true;
    return fabsf(line_slope(a, b) - line_slope(b, c)) < EPSILON;
}

static bool point_above_or_on_line(const FloatPoint& point, const FloatPoint& a, const FloatPoint& b)
{
    auto slope = line_slope(a, b);
    auto intersecting_y = (point.x() - a.x()) * slope + a.y();
    return point.y() <= intersecting_y;
}

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line_segment
static IntersectionResult line_intersection(const FloatPoint& a1, const FloatPoint& a2, const FloatPoint& b1, const FloatPoint& b2)
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

static int compare_events(bool a_is_start, const FloatPoint& a1, const FloatPoint& a2, bool b_is_start, const FloatPoint& b1, const FloatPoint& b2)
{
    auto comp = a1 <=> b1;
    if (comp != 0) {
        // Different target points makes this easy
        return comp;
    }

    if (a2 == b2) {
        // Both ends of both events are the same, so it doesn't matter
        return 0;
    }

    if (a_is_start != b_is_start) {
        // Prefer sorting end events first
        return a_is_start ? 1 : -1;
    }

    // Determine which event line is above the other
    return point_above_or_on_line(
               a2,
               b_is_start ? b1 : b2,
               b_is_start ? b2 : b1)
        ? 1
        : -1;
}

struct PathClipping::Event : public RefCounted<PathClipping::Event> {
    bool is_start;
    bool is_primary;
    Segment segment;
    RefPtr<Event> other_event;

    Event(bool is_start, bool is_primary, const Segment& segment, const RefPtr<Event>& other_event)
        : is_start(is_start)
        , is_primary(is_primary)
        , segment(segment)
        , other_event(other_event)
    {
    }

    const FloatPoint& point() const { return is_start ? segment.start : segment.end; }
    const FloatPoint& other_point() const { return is_start ? segment.end : segment.start; }

    void update_other_segment() { other_event->segment = segment; }

    // // FIXME: Remove?
    int operator<=>(const Event& event) const
    {
        return compare_events(is_start, point(), other_point(), event.is_start, event.point(), event.other_point());
    }
};

Vector<Path> PathClipping::clip(Path& a, Path& b, ClipType clip_type)
{
    if (clip_type == ClipType::DifferenceReversed)
        return clip(b, a, ClipType::Difference);

    dbg("\033[32;1m[clip]\033[0m STARTING");
    dbg("[clip] a = {}", a);
    dbg("[clip] b = {}", b);

    a.close_all_subpaths();
    b.close_all_subpaths();

    auto poly_a = convert_to_polygon(a, true);
    auto poly_b = convert_to_polygon(b, false);
    auto combined = combine(poly_a, poly_b);
    dbg("\033[32;1m[clip]\033[0m ENDING");
    return select_segments(combined, clip_type);
}

PathClipping::Polygon PathClipping::convert_to_polygon(Path& path, bool is_primary)
{
    PathClipping processor(false);

    auto split_lines = path.split_lines();
    dbg("\033[32;1m[convert_to_polygon]\033[0m STARTING");

    for (auto& split_line : split_lines) {
        auto is_forward = split_line.from <=> split_line.to;
        dbg("[convert_to_polygon]   split_line={} is_forward={}", split_line, is_forward);
        if (is_forward == 0)
            continue;

        auto from = split_line.from;
        auto to = split_line.to;
        if (is_forward > 0)
            swap(from, to);

        processor.add_segment({ from, to }, is_primary);
    }

    dbg("\033[32;1m[convert_to_polygon]\033[0m ENDING");

    return processor.create_polygon();
}

PathClipping::Polygon PathClipping::combine(const Polygon& a, const Polygon& b)
{
    dbg("\033[32;1m[combine]\033[0m STARTING");
    PathClipping processor(true);

    for (auto& segment : a)
        processor.add_segment(segment, true);
    for (auto& segment : b)
        processor.add_segment(segment, false);

    dbg("\033[32;1m[combine]\033[0m ENDING");
    return processor.create_polygon();
}

PathClipping::Polygon PathClipping::clip_polygon(const Polygon& input_polygon, ClipType clip_type)
{
    dbg("[clip_polygon] clipping with type {}", clip_type);
    Polygon output_polygon;
    auto clip_table = table_for_clip_type(clip_type);

    for (auto& segment : input_polygon) {
        auto table_index = segment_state_index(segment);
        auto state = clip_table[table_index];

        dbg("[clip_polygon]   segment {} is of state {}", segment, state);

        if (state == State::Discard)
            continue;

        output_polygon.append(Segment {
            segment.start,
            segment.end,
            state == State::FillAbove ? TriState::True : TriState::False,
            state == State::FillBelow ? TriState::True : TriState::False,
        });
    }

    return output_polygon;
}

Vector<Path> PathClipping::convert_to_path(Polygon& polygon)
{
    dbg("\033[32;1m[convert_to_path]\033[0m Converting...");
    for (auto& segment : polygon)
        dbg("[convert_to_path]   segment {}", segment);

    Vector<Vector<FloatPoint>> chains;
    Vector<Path> paths;

    auto reverse_chain = [&](size_t chain_index) {
        chains[chain_index] = chains[chain_index].reversed();
    };

    auto merge_chains = [&](size_t first_chain_index, size_t second_chain_index) {
        auto& first_chain = chains[first_chain_index];
        auto& second_chain = chains[second_chain_index];
        first_chain.append(move(second_chain));
        chains.remove(second_chain_index);
    };

    auto finalize_chain = [&](size_t chain_index) {
        auto chain = chains[chain_index];
        chains.remove(chain_index);
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
        dbg("[convert_to_path] processing {}", segment);
        dbg("[convert_to_path] chains:");
        for (auto& chain : chains) {
            StringBuilder builder;
            builder.appendff("{}", chain[0]);
            for (size_t i = 1; i < chain.size(); i++)
                builder.appendff(" --> {}", chain[i]);
            dbg("[convert_to_path]   {}", builder.to_string());
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
            dbg("[convert_to_path]   looping over chain {} with head={} tail={}", i, head, tail);

            if (equivalent(head, segment.start)) {
                dbg("[convert_to_path]     head == segment.start");
                if (set_match(i, true, true))
                    break;
            } else if (equivalent(head, segment.end)) {
                dbg("[convert_to_path]     head == segment.end");
                if (set_match(i, true, false))
                    break;
            } else if (equivalent(tail, segment.start)) {
                dbg("[convert_to_path]     tail == segment.start");
                if (set_match(i, false, true))
                    break;
            } else if (equivalent(tail, segment.end)) {
                dbg("[convert_to_path]     tail == segment.end");
                if (set_match(i, false, false))
                    break;
            }
        }

        if (!maybe_first_match.has_value()) {
            // No matches, make a new chain
            dbg("[convert_to_path]   no matches, making a new chain from {} to {}", segment.start, segment.end);
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

            dbg("[convert_to_path]   chain.first={} chain.last={}", chain.first(), chain.last());
            dbg("[convert_to_path]   found one match with chain {} (matches_start_of_chain={} matches_start_of_segment={})", match.index, match.matches_start_of_chain, match.matches_start_of_segment);

            if (match.matches_start_of_chain) {
                dbg("[convert_to_path]     prepending {} to chain", point_to_append);
                chain.prepend(point_to_append);
            } else {
                dbg("[convert_to_path]     appending {} to chain", point_to_append);
                chain.append(point_to_append);
            }

            dbg("[convert_to_path]   point_to_append={} opposite_point={}", point_to_append, opposite_point);
            if (equivalent(point_to_append, opposite_point)) {
                // this chain is closing
                dbg("[convert_to_path]   closing this chain");
                finalize_chain(match.index);
            }

            continue;
        }

        // Two matches, we have to join two chains

        dbg("[convert_to_path]   found two matches");

        auto& first_match = maybe_first_match.value();
        auto& second_match = maybe_second_match.value();

        auto first_match_index = first_match.index;
        auto second_match_index = second_match.index;

        dbg("[convert_to_path]     first_match_index={}", first_match_index);
        dbg("[convert_to_path]     second_match_index={}", second_match_index);

        auto reverse_first_chain = chains[first_match_index].size() < chains[second_match_index].size();

        dbg("[convert_to_path]     reverse_first_chain={}", reverse_first_chain);

        if (first_match.matches_start_of_chain) {
            dbg("[convert_to_path]     1");
            if (second_match.matches_start_of_chain) {
                dbg("[convert_to_path]     2");
                if (reverse_first_chain) {
                    dbg("[convert_to_path]     3");
                    reverse_chain(first_match_index);
                    merge_chains(first_match_index, second_match_index);
                } else {
                    dbg("[convert_to_path]     4");
                    reverse_chain(second_match_index);
                    merge_chains(second_match_index, first_match_index);
                }
            } else {
                dbg("[convert_to_path]     5");
                merge_chains(second_match_index, first_match_index);
            }
        } else {
            dbg("[convert_to_path]     6");
            if (second_match.matches_start_of_chain) {
                dbg("[convert_to_path]     7");
                merge_chains(first_match_index, second_match_index);
            } else {
                if (reverse_first_chain) {
                    dbg("[convert_to_path]     8");
                    reverse_chain(first_match_index);
                    merge_chains(second_match_index, first_match_index);
                } else {
                    dbg("[convert_to_path]     9");
                    reverse_chain(second_match_index);
                    merge_chains(first_match_index, second_match_index);
                }
            }
        }
    }

    dbg("chains size={}", chains.size());
    VERIFY(chains.is_empty());
    return paths;
}

Vector<Path> PathClipping::select_segments(const Polygon& input_polygon, ClipType clip_type)
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

    dbg("\033[32;1m[create_polygon]\033[0m STARTING");
    dbg();
    dbg("[create] events:");
    size_t i = 0;
    for (auto& event : m_event_queue)
        dbg("[create]   event{} = {}", i++, *event);
    dbg();

    auto do_event_intersections = [&](RefPtr<Event>& event, RefPtr<Event>& other) {
        auto result = intersect_events(event, other);
        if (result) {
            dbg("\033[36;1m[create]\033[0m   result from do_event_intersections");
            // event is the same as other
            if (m_is_combining_phase) {
                result->segment.other_fill_above = event->segment.self_fill_above;
                result->segment.other_fill_below = event->segment.self_fill_below;
                dbg("\033[36;1m[create]\033[0m   other_fill_above={} other_fill_below={}", result->segment.other_fill_above, result->segment.other_fill_below);
                result->update_other_segment();
            } else {
                bool toggle;
                if (event->segment.self_fill_below == TriState::Unknown) {
                    dbg("\033[36;1m[create]\033[0m   (1) toggle = true");
                    toggle = true;
                } else {
                    toggle = event->segment.self_fill_above != event->segment.self_fill_below;
                    dbg("\033[36;1m[create]\033[0m   (2) toggle = {}", toggle);
                }

                if (toggle) {
                    auto fill_above = result->segment.self_fill_above;
                    VERIFY(fill_above != TriState::Unknown);
                    result->segment.self_fill_above = fill_above == TriState::True ? TriState::False : TriState::True;
                    dbg("\033[36;1m[create]\033[0m   Toggling self_fill_above from {} to {}", fill_above, result->segment.self_fill_above);
                    result->update_other_segment();
                }
            }

            m_event_queue.remove(event);
            m_event_queue.remove(event->other_event);

            return true;
        }

        return false;
    };

    while (!m_event_queue.is_empty()) {
        auto& event = m_event_queue.first();

        dbg();
        dbg("\033[34;1m[create_polygon]\033[0m status stack: [");
        for (auto& event1 : m_status_stack)
            dbg("\033[34;1m[create_polygon]\033[0m   {}", *event1);
        dbg("\033[34;1m[create_polygon]\033[0m ]");

        dbg("[create] processing event {}", *event);

        if (event->is_start) {
            auto find_result = find_transition(event);

            RefPtr<Event> event_before;
            RefPtr<Event> event_after;

            if (!find_result.is_end()) {
                auto prev = find_result.prev();
                auto next = find_result;
                if (prev) {
                    dbg("[create]   event has a previous event");
                    event_before = *prev;
                }
                if (next) {
                    dbg("[create]   event has a next event");
                    event_after = *next;
                }
            } else if (!m_status_stack.is_empty()) {
                event_before = m_status_stack.last();
            }

            auto intersected = false;
            if (event_before) {
                dbg("[create]   intersecting with previous event");
                intersected = do_event_intersections(event, event_before);
            }

            if (!intersected && event_after) {
                dbg("[create]   intersected with next event");
                do_event_intersections(event, event_after);
            }

            // In the case of intersection, events will have been added to the
            // event queue. They may need to be processed before this event.
            if (m_event_queue.first() != event) {
                // An event has been inserted before the current event being processed
                continue;
            }

            if (m_is_combining_phase) {
                if (event->segment.other_fill_above == TriState::Unknown) {
                    dbg("~~~~~~~~~~ NO OTHER FILL ABOVE INFO");
                    TriState inside;
                    if (!event_after) {
                        dbg("\033[36;1m[create]\033[0m   (1) inside = False");
                        inside = TriState::False;
                    } else if (event->is_primary == event_after->is_primary) {
                        inside = event_after->segment.other_fill_above;
                        VERIFY(inside != TriState::Unknown);
                        dbg("\033[36;1m[create]\033[0m   (2) inside = {}", inside);
                    } else {
                        inside = event_after->segment.self_fill_above;
                        dbg("looking at event_after={}", *event_after);
                        VERIFY(inside != TriState::Unknown);
                        dbg("\033[36;1m[create]\033[0m   (3) inside = {}", inside);
                    }

                    event->segment.other_fill_above = inside;
                    event->segment.other_fill_below = inside;
                    event->update_other_segment();
                }
            } else {
                bool toggle;
                if (event->segment.self_fill_below == TriState::Unknown) {
                    dbg("\033[36;1m[create]\033[0m   toggle = true (self_fill_below == Unknown)");
                    toggle = true;
                } else {
                    toggle = event->segment.self_fill_above != event->segment.self_fill_below;
                    dbg("\033[36;1m[create]\033[0m   toggle = {}", toggle);
                }

                if (!event_after) {
                    dbg("\033[36;1m[create]\033[0m   No event after, setting self_fill_below to false");
                    event->segment.self_fill_below = TriState::False;
                } else {
                    dbg("\033[36;1m[create]\033[0m   Event after, setting self_fill_below to {}", event_after->segment.self_fill_above);
                    event->segment.self_fill_below = event_after->segment.self_fill_above;
                }

                if (toggle) {
                    auto fill_below = event->segment.self_fill_below;
                    VERIFY(fill_below != TriState::Unknown);
                    event->segment.self_fill_above = fill_below == TriState::True ? TriState::False : TriState::True;
                    dbg("\033[36;1m[create]\033[0m   Toggling self_fill_above to {}", event->segment.self_fill_above);
                } else {
                    dbg("\033[36;1m[create]\033[0m   Setting self_fill_above to self_fill_below ({})", event->segment.self_fill_below);
                    event->segment.self_fill_above = event->segment.self_fill_below;
                }

                event->update_other_segment();
            }

            dbg("[create]   inserting event");
            m_status_stack.insert_before(find_result, event);
        } else {
            auto existing_status = m_status_stack.find(event->other_event);
            if (existing_status.prev() && existing_status.next()) {
                do_event_intersections(*existing_status.prev(), *existing_status.next());
            }

            dbg("[create]   removing event");
            m_status_stack.remove(existing_status);

            if (m_is_combining_phase && !event->is_primary) {
                // Swap fill info for secondary polygon
                swap(event->segment.self_fill_above, event->segment.other_fill_above);
                swap(event->segment.self_fill_below, event->segment.other_fill_below);
                event->update_other_segment();
            }

            dbg("=== APPENDING SEGMENT {}", event->segment);
            polygon.append(event->segment);
        }

        m_event_queue.take_first();
    }

    dbg("\033[32;1m[create_polygon]\033[0m ENDING");
    dbg("\033[32;1m[create_polygon]\033[0m SEGMENTS:");
    for (auto& segment : polygon)
        dbg("\033[32;1m[create_polygon]\033[0m   {}", segment);
    dbg();

    return polygon;
}

void PathClipping::add_segment(const Segment& segment, bool is_primary)
{
    auto start = adopt_ref(*new Event(true, is_primary, segment, {}));
    auto end = adopt_ref(*new Event(false, is_primary, segment, start));
    start->other_event = end;

    add_event(start, segment.end);
    add_event(end, segment.start);
}

DoublyLinkedList<RefPtr<PathClipping::Event>>::Iterator PathClipping::find_transition(RefPtr<Event> event)
{
    auto a1 = event->segment.start;
    auto a2 = event->segment.end;
    dbg("[find_transition] finding transition (a1={} a2={})", a1, a2);

    size_t offset = 0;

    auto r = m_status_stack.find_if([&](const RefPtr<Event>& other_event) {
        offset++;
        auto b1 = other_event->segment.start;
        auto b2 = other_event->segment.end;
        dbg("[find_transition]   candidate event (b1={} b2={})", b1, b2);

        if (points_are_collinear(a1, b1, b2)) {
            dbg("[find_transition]     a1 is collinear with b1->b2");
            if (points_are_collinear(a2, b1, b2)) {
                dbg("[find_transition]     a2 is collinear with b1->b2");
                return true;
            }
            if (point_above_or_on_line(a2, b1, b2)) {
                dbg("[find_transition]     a2 is above/on b1->b2");
            } else {
                dbg("[find_transition]     a2 is NOT above/on b1->b2");
            }
            return point_above_or_on_line(a2, b1, b2);
        }
        if (point_above_or_on_line(a1, b1, b2)) {
            dbg("[find_transition]     a1 is above/on b1->b2");
        } else {
            dbg("[find_transition]     a1 is NOT above/on b1->b2");
        }
        return point_above_or_on_line(a1, b1, b2);
    });

    dbg("[find_transition]   transition found at offset {} (end={})", offset == 0 ? offset : offset - 1, r.is_end());

    return r;
}

RefPtr<PathClipping::Event> PathClipping::intersect_events(RefPtr<Event>& a, RefPtr<Event>& b)
{
    dbg("[intersect_event] intersecting {} and {}", *a, *b);
    auto share_points = equivalent(a->point(), b->point())
        || equivalent(a->point(), b->other_point())
        || equivalent(a->other_point(), b->point())
        || equivalent(a->other_point(), b->other_point());

    if (share_points) {
        // The segments are joined at one end, so we just ignore this and process
        // them as two normal segments
        dbg("[intersect_event]   segments are joined at ends");
        return {};
    }

    auto intersection_result = line_intersection(a->segment.start, a->segment.end, b->segment.start, b->segment.end);
    dbg("[intersect_event]   intersection_result={}", intersection_result);

    if (intersection_result.type == IntersectionResult::Intersects) {
        auto split_point = intersection_result.point;

        // Use <=> to take some epsilon into account
        auto split_a = (split_point <=> a->point()) != 0 && (split_point <=> a->other_point()) != 0;
        auto split_b = (split_point <=> b->point()) != 0 && (split_point <=> b->other_point()) != 0;

        dbg("[intersect_event]   split_a={} split_b={}", split_a, split_b);

        if (split_a)
            split_event(a, split_point);

        if (split_b)
            split_event(b, split_point);
    } else if (intersection_result.type == IntersectionResult::Coincident) {
        auto a1_eq_b1 = (a->point() <=> b->point()) == 0;
        auto a2_eq_b2 = (a->other_point() <=> b->other_point()) == 0;

        dbg("[intersect_event]   a1_eq_b1={} a2_eq_b2={}", a1_eq_b1, a2_eq_b2);

        if (a1_eq_b1 && a2_eq_b2) {
            // The segments are equal
            return b;
        }

        auto a1_between = !a1_eq_b1 && is_point_between(a->point(), b->point(), b->other_point());
        auto a2_between = !a2_eq_b2 && is_point_between(a->other_point(), b->point(), b->other_point());

        dbg("[intersect_event]   a1_between={} a2_between={}", a1_between, a2_between);

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

void PathClipping::split_event(RefPtr<Event>& event, const FloatPoint& point_to_split_at)
{
    dbg("[split_event] splitting {} at point {}", *event, point_to_split_at);
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

    dbg("[split_event]   removing {}", *event->other_event);

    Segment new_segment { point_to_split_at, event->segment.end, event->segment.self_fill_above, event->segment.self_fill_below };

    m_event_queue.remove(event->other_event);
    event->segment.end = point_to_split_at;
    auto first_segment_end = adopt_ref(*new Event { !event->is_start, event->is_primary, event->segment, event });
    event->other_event = first_segment_end;
    add_event(first_segment_end, event->point());

    add_segment(new_segment, event->is_primary);
}

void PathClipping::add_event(const RefPtr<Event>& event, const FloatPoint& other_point)
{
    dbg("[add_event] adding {} (other_point={})", *event, other_point);

    auto insertion_location = m_event_queue.find_if([&](const RefPtr<Event>& a) {
        auto r = compare_events(event->is_start, event->point(), other_point, a->is_start, a->point(), a->other_point());
        dbg("[add_event]   comparison to {}: {}", *a, r);
        return r < 0;
    });

    m_event_queue.insert_before(insertion_location, event);
}

}

namespace AK {

template<>
struct Formatter<Gfx::PathClipping::Event> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::PathClipping::Event& event)
    {
        Formatter<StringView>::format(builder, String::formatted("{{ segment={} start={} primary={} }}", event.segment, event.is_start, event.is_primary));
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

template<>
struct Formatter<Gfx::State> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::State& result)
    {
        switch (result) {
        case Gfx::State::Discard:
            Formatter<StringView>::format(builder, "Discard");
            break;
        case Gfx::State::FillAbove:
            Formatter<StringView>::format(builder, "FillAbove");
            break;
        case Gfx::State::FillBelow:
            Formatter<StringView>::format(builder, "FillBelow");
            break;
        }
    }
};

template<>
struct Formatter<Gfx::ClipType> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::ClipType& result)
    {
        switch (result) {
        case Gfx::ClipType::Intersection:
            Formatter<StringView>::format(builder, "Intersection");
            break;
        case Gfx::ClipType::Union:
            Formatter<StringView>::format(builder, "Union");
            break;
        case Gfx::ClipType::Difference:
            Formatter<StringView>::format(builder, "Difference");
            break;
        case Gfx::ClipType::DifferenceReversed:
            Formatter<StringView>::format(builder, "DifferenceReversed");
            break;
        case Gfx::ClipType::Xor:
            Formatter<StringView>::format(builder, "Xor");
            break;
        }
    }
};

}
