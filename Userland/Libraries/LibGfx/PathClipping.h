/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGfx/Path.h>
#include <LibGfx/Point.h>
#include <AK/DoublyLinkedList.h>

namespace Gfx {

enum class ClipType {
    Intersection,
    Union,
    Difference, // a - b
    DifferenceReversed, // b - a
    Xor,
};



class PathClipping {
public:
    struct Segment {
        FloatPoint start;
        FloatPoint end;
        TriState self_fill_above { TriState::Unknown };
        TriState self_fill_below { TriState::Unknown };
        TriState other_fill_above { TriState::Unknown };
        TriState other_fill_below { TriState::Unknown };
    };

    using Polygon = Vector<Segment>;

    struct Event;

    static Vector<Path> clip(Path& a, Path& b, ClipType);
    static Polygon convert_to_polygon(Path&, bool is_primary);
    static Polygon combine(const Polygon&, const Polygon&);
    static Vector<Path> select_segments(const Polygon&, ClipType);

private:
    static Polygon clip_polygon(const Polygon&, ClipType);
    static Vector<Path> convert_to_path(Polygon&);

    explicit PathClipping(bool is_combining_phase);

    Polygon create_polygon();

    void add_segment(const Segment& segment, bool is_primary);
    DoublyLinkedList<RefPtr<Event>>::Iterator find_transition(RefPtr<Event>);

    // Return of true means the first event is redundant and should be removed
    RefPtr<Event> intersect_events(RefPtr<Event>&, RefPtr<Event>&);
    void split_event(RefPtr<Event>&, const FloatPoint& point_to_split_at);
    void add_event(const RefPtr<Event>&, const FloatPoint& other_point);

    bool m_is_combining_phase;
    DoublyLinkedList<RefPtr<Event>> m_event_queue;
    DoublyLinkedList<RefPtr<Event>> m_status_stack;
};

}

namespace AK {

template<>
struct Formatter<Gfx::PathClipping::Segment> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::PathClipping::Segment& segment)
    {
        String self = "???";
        String other = "???";

        if (segment.self_fill_above != TriState::Unknown || segment.self_fill_below != TriState::Unknown)
            self = String::formatted("{} {}", segment.self_fill_above, segment.self_fill_below);
        if (segment.other_fill_above != TriState::Unknown || segment.other_fill_below != TriState::Unknown)
            other = String::formatted("{} {}", segment.other_fill_above, segment.other_fill_below);


        auto str = String::formatted("{{ [{}, {}] self={} other={} }}", segment.start, segment.end, self, other);
        Formatter<StringView>::format(builder, str);
    }
};

template<>
struct Formatter<Gfx::PathClipping::Event>;

template<>
struct Formatter<TriState> : Formatter<StringView> {
    void format(FormatBuilder& builder, TriState state)
    {
        switch (state) {
        case TriState::False:
            Formatter<StringView>::format(builder, "False");
            break;
        case TriState::True:
            Formatter<StringView>::format(builder, "True");
            break;
        case TriState::Unknown:
            Formatter<StringView>::format(builder, "Unknown");
            break;
        }
    }
};

}
