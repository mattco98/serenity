/*
 * Copyright (c) 2021, Matthew Olsson <matthewcolsson@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <LibGfx/AffineTransform.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Path.h>

namespace Gfx {

enum class LineStyle {
    Solid,
    Dotted,
    Dashed
};

class AAPainter {
public:
    // These types help add context to function parameters. Painter types are types
    // which would be passed in through an external caller such as LibGui, and have not
    // yet been transformed. Physical types are types which have already been transformed
    // and lie within the clip_rect.
    using PainterPoint = IntPoint;
    using PhysicalPoint = IntPoint;
    using PainterRect = IntRect;
    using PhysicalRect = IntRect;

    explicit AAPainter(Gfx::Bitmap*);
    ~AAPainter();

    void draw_path(const Gfx::Path& path, Color color);
    void draw_line(PainterPoint p0, PainterPoint p1, Color color, float thickness = 1);

    [[nodiscard]] ALWAYS_INLINE FloatPoint translation() { return transform().translation(); }
    ALWAYS_INLINE void translate(const FloatPoint& delta) { transform().translate(delta); }
    ALWAYS_INLINE void translate(float dx, float dy) { transform().translate(dx, dy); }
    [[nodiscard]] ALWAYS_INLINE FloatPoint scale() { return transform().scale(); }
    ALWAYS_INLINE void scale(const FloatPoint& delta) { transform().scale(delta); }
    ALWAYS_INLINE void scale(float dx, float dy) { transform().scale(dx, dy); }

    [[nodiscard]] ALWAYS_INLINE const PhysicalRect& clip_rect() const { return state().clip_rect; }
    ALWAYS_INLINE void set_clip_rect(const PainterRect& clip_rect) { state().clip_rect = clip_rect; }
    ALWAYS_INLINE void add_clip_rect(const PainterRect& clip_rect) { state().clip_rect.intersect(clip_rect); }

    [[nodiscard]] ALWAYS_INLINE LineStyle line_style() const { return state().line_style; }
    ALWAYS_INLINE void set_line_style(LineStyle line_style) { state().line_style = line_style; }

    ALWAYS_INLINE void push_state() { m_state_stack.append(m_state_stack.last()); }
    ALWAYS_INLINE void pop_state() { m_state_stack.take_last(); }

private:
    struct State {
        AffineTransform transform;
        PhysicalRect clip_rect;
        LineStyle line_style { LineStyle::Solid };
    };

    // void draw_wu_line(const PhysicalPoint& p0, const PhysicalPoint& p1, Color color, bool steep);

    ALWAYS_INLINE State& state() { return m_state_stack.last(); }
    ALWAYS_INLINE const State& state() const { return m_state_stack.last(); }

    ALWAYS_INLINE AffineTransform& transform() { return state().transform; }

    ALWAYS_INLINE PhysicalPoint convert_to_physical(PainterPoint& point) { return point.transformed(transform()); }
    ALWAYS_INLINE PhysicalRect convert_to_physical(PainterRect& rect) { return rect.transformed(transform()); }

    Bitmap* m_target;
    Vector<State, 4> m_state_stack;
};

}
