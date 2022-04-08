/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DoublyLinkedList.h>
#include <AK/Vector.h>
#include <LibGfx/Point.h>

namespace Gfx {

enum class IsInside {
    Yes,
    No,
    Unknown,
};

// This is an implementation of the algorithm described in this excellent
// blog post: https://sean.cm/a/polygon-clipping-pt2
class PathClipping {
public:
    enum class ClipType {
        Intersection,
        Union,
        Difference, // a - b
        // FIXME: Is this necessary? Can the caller just supply reversed args?
        DifferenceReversed, // b - a
        Xor,
    };

    struct Annotation {
        IsInside above { IsInside::Unknown };
        IsInside below { IsInside::Unknown };
    };

    struct Segment {
        FloatPoint start;
        FloatPoint end;

        Annotation self {};
        Annotation other {};
    };

    struct Event;

    using Polygon = Vector<Segment>;

    static Vector<Path> clip(Path& a, Path& b, ClipType);
    static Polygon convert_to_polygon(Path&, bool is_primary);
    static Polygon combine(Polygon const&, Polygon const&);
    static Vector<Path> select_segments(Polygon const&, ClipType);
    static Polygon clip_polygon(Polygon const&, ClipType);
    static Vector<Path> convert_to_path(Polygon&);

private:
    explicit PathClipping(bool is_combining_phase);

    Polygon create_polygon();

    void add_segment(Segment const& segment, bool is_primary);
    DoublyLinkedList<RefPtr<Event>>::Iterator find_transition(RefPtr<Event>);

    // Return of true means the first event is redundant and should be removed
    RefPtr<Event> intersect_events(RefPtr<Event>&, RefPtr<Event>&);
    void split_event(RefPtr<Event>&, FloatPoint const& point_to_split_at);
    void add_event(RefPtr<Event> const&);

    bool m_is_combining_phase;
    DoublyLinkedList<RefPtr<Event>> m_event_queue;
    DoublyLinkedList<RefPtr<Event>> m_status_stack;
};

}

namespace AK {

template<>
struct Formatter<Gfx::PathClipping::Segment>;

template<>
struct Formatter<Gfx::PathClipping::Event>;

template<>
struct Formatter<Gfx::IsInside>;

}
