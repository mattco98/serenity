/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Array.h>
#include <AK/DoublyLinkedList.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Vector.h>
#include <LibGfx/Point.h>

namespace Gfx {

class PathClipping {
public:
    enum class IsInside {
        Yes,
        No,
        Unknown,
    };

    struct Annotations {
        IsInside above { IsInside::Unknown };
        IsInside below { IsInside::Unknown };
    };

    struct Segment {
        Gfx::FloatPoint start;
        Gfx::FloatPoint end;
        Annotations self {};
        Annotations other {};
    };

    struct Polygon {
        using Region = Vector<Segment>;

        Vector<Region> regions;
        bool inverted;
    };

    class Event;

    enum class Op {
        Intersection,
        Union,
        Difference,
        Xor,
    };

    static Polygon make_polygon(Path&);

private:
    struct Transition {
        DoublyLinkedList<RefPtr<Event>>::Iterator above;
        DoublyLinkedList<RefPtr<Event>>::Iterator below;
    };

    PathClipping() = default;

    void process_events();
    void add_event(RefPtr<Event> const&);
    Transition find_transition(RefPtr<Event> const&);
    void split_event(RefPtr<Event>&, Gfx::FloatPoint const& point);

    bool m_primary_poly_inverted { false };
    bool m_secondary_poly_inverted { false };

    DoublyLinkedList<RefPtr<Event>> m_event_queue;
    DoublyLinkedList<RefPtr<Event>> m_status;
};

}
