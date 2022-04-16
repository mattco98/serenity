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

        String to_string() const
        {
            return String::formatted("[{}, {}]", start, end);
        }
    };

    struct Event : public RefCounted<Event> {
        bool is_start;
        bool is_primary;
        Segment segment;
        RefPtr<Event> other_event;

        Event(bool is_start, bool is_primary, Segment const& segment, RefPtr<Event> const& other_event);

        FloatPoint const& point() const { return is_start ? segment.start : segment.end; }
        FloatPoint const& other_point() const { return is_start ? segment.end : segment.start; }

        void update_other_segment() { other_event->segment = segment; }

        // Determines the ordering of this event in the status queue relative to another event,
        // i.e., which event should be processed first.
        int operator<=>(Event const& event) const;
    };

    using Polygon = Vector<Segment>;

    static Vector<Path> clip(Path& a, Path& b, ClipType);
    static Polygon convert_to_polygon(Path&, bool is_primary);
    static Polygon combine(Polygon const&, Polygon const&);
    static Vector<Path> select_segments(Polygon const&, ClipType);
    static Polygon clip_polygon(Polygon const&, ClipType);
    static Vector<Path> convert_to_path(Polygon&);

private:
    struct Transition {
        using It = DoublyLinkedList<RefPtr<PathClipping::Event>>::Iterator;
        It before;
        It after;
    };

    explicit PathClipping(bool is_combining_phase);

    Polygon create_polygon();

    void add_segment(Segment const& segment, bool is_primary);
    Transition find_transition(RefPtr<Event>);

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
