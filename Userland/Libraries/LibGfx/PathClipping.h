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
    Difference,
    Xor,
};

class PathClipping {
public:
    struct Segment {
        FloatPoint start;
        FloatPoint end;
    };

    struct Event;

    static Vector<Segment> clip(ClipType, Path& a, Path& b);

private:
    PathClipping(ClipType, Path& a, Path& b);

    Vector<Segment> clip();

    void add_region(Path&);
    void add_segment(const Segment& segment);
    DoublyLinkedList<RefPtr<Event>>::Iterator find_transition(RefPtr<Event>);

    // Return of true means the first event is redundant and should be removed
    bool intersect_events(RefPtr<Event>&, RefPtr<Event>&);
    void split_event(RefPtr<Event>&, const FloatPoint& point_to_split_at);
    void add_event(const RefPtr<Event>&);

    ClipType m_clip_type;
    Path& m_a;
    Path& m_b;

    DoublyLinkedList<RefPtr<Event>> m_event_queue;
    DoublyLinkedList<RefPtr<Event>> m_status_stack;
};

}

namespace AK {

template<>
struct Formatter<Gfx::PathClipping::Segment> : Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::PathClipping::Segment& segment)
    {
        Formatter<StringView>::format(builder, String::formatted("{{ start={} end={} }}", segment.start, segment.end));
    }
};

template<>
struct Formatter<Gfx::PathClipping::Event>;

}
