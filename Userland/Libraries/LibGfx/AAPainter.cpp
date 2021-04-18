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

#include <LibGfx/AAPainter.h>

#define AAPAINTER_DEBUG 0

namespace Gfx {

AAPainter::AAPainter(Gfx::Bitmap* bitmap)
    : m_target(bitmap)
{
    m_state_stack.append({ {}, bitmap->physical_rect() });
}

AAPainter::~AAPainter()
{
}

void AAPainter::debug_point(PainterPoint point, Color color)
{
    IntRect rect {
        { point.x() - 1, point.y() - 1 },
        { 3, 3 }
    };

    for (int y = rect.top(); y <= rect.bottom(); y++) {
        for (int x = rect.left(); x <= rect.right(); x++) {
            m_target->set_pixel(x, y, color);
        }
    }

    m_target->set_pixel(point, Color::NamedColor::Yellow);
}

void AAPainter::draw_line(PainterPoint p0, PainterPoint p1, Color color, float thickness)
{
    if (p0 == p1)
        return; // FIXME: Incorrect behavior with square/round linecaps

    VERIFY(line_style() == LineStyle::Solid);
    VERIFY(thickness >= 1.0f);

    p0 = convert_to_physical(p0);
    p1 = convert_to_physical(p1);

    dbgln_if(AAPAINTER_DEBUG, "[draw_line] p0={} p1={} thickness={} clip_rect={}", p0, p1, thickness, clip_rect());

    bool steep = abs(p1.y() - p0.y()) > abs(p1.x() - p0.x());

    // We draw from top to bottom for steep lines, and left to right
    // for non-steep lines
    if ((steep && p0.y() > p1.y()) || (!steep && p0.x() > p1.x()))
        swap(p0, p1);

    dbgln("~~~ p0={} p1={}", p0, p1);

    auto dx = static_cast<float>(p1.x() - p0.x());
    auto dy = static_cast<float>(p1.y() - p0.y());

    float slope = dy / dx;

    /*
     * The following diagram is an example of the top of a steep line, i.e. a line
     * whose dY is greater than its dX (sign ignored). The "x" represents the point
     * p0, as we always draw from the top-down (and always from p0 to p1). Ignoring
     * line-caps while drawing would give us the line on the left, however we want
     * the line on the right, whose line cap angle agrees with the rest of the line.
     *
     *
     * the angle of this        V--- this edge point is cap1.
     * line is cap_angle --> _.'\    the other edge point is cap0
     *                    _:'    \
     * \---x---\          \---x---\
     *  \       \          \       \
     *   \       \          \       \
     *    \       \          \       \
     *     \       \          \       \
     *
     *     ^ without line caps
     *                            ^ with line caps (square)
     *
     * Some notes:
     *   [1] One of the annoyances here is that the line is now longer than expected.
     *       This is actually perfect if the line cap style is Square, however for
     *       styles None and Round, we need the "x" to actually be on the cap line.
     *       In this case, it's easier to move p0 along the line by the error amount,
     *       which results in a nice flat line cap (which we can then draw a circle
     *       over, if necessary).
     *   [2] Some of the cap can be drawn in the normal for loop. It is not visible in
     *       the diagram above, but some times cap0 will be above p0, which means that
     *       some of the rows can be calculated in the primary for loop. For this reason,
     *       we draw the cap until cap0.y instead of p0.y
     *   [3] The thickness parameter is a measurement of the distance between the center
     *       of the line and the edge. However, this distance is measured at an angle
     *       perpendicular to the line (i.e. cap_angle). However, we draw the line
     *       in columns, so we transform this to an equivalent distance in the vertical
     *       direction.
     *   [4] The line cap on both ends are identical copies rotated around the center of
     *       the line. The math is only calculated for one cap, and then both are drawn
     *       simultaneously.
     */

    if (steep) {
        slope = 1.0f / slope;
        auto half_thickness = thickness / 2.0f;
        auto current_x = static_cast<float>(p0.x());

        // // See note [2] above
        // for (int x = cap0.x(); x < cap1.x(); x++) {
        //     auto xdiff = static_cast<float>(x - cap0.x());
        //     float y0 = static_cast<float>(cap0.y()) - xdiff * tanf(cap_angle);
        //     float y1 = static_cast<float>(cap0.y()) + xdiff * tanf(M_PI_2f32 - cap_angle);
        //
        //     // Always prefer more area vs less, as explained above
        //     int y0i = floor(y0);
        //     int y1i = ceil(y1);
        //
        //     // See note [4] above
        //     // FIXME: Why is this -1 fudge factor necessary? And does it always work?
        //     int mirror_x = (p0.x() - x) + p1.x() - 1;
        //
        //     for (int y = y0i; y <= y1i; y++) {
        //         // FIXME: Why is this +1 fudge factor necessary? And does it always work?
        //         int mirror_y = (p0.y() - y) + p1.y() + 1;
        //         if (y >= y0 && y <= y1) {
        //             m_target->set_pixel(x, y, color);
        //             m_target->set_pixel(mirror_x, mirror_y, color);
        //         } else {
        //             float error = y < y0 ? y0 - y : y - y1;
        //                 VERIFY(error >= 0.0f && error <= 1.0f);
        //             auto alpha = static_cast<int>(255.0f * (1.0f - error));
        //             auto current_color = m_target->get_pixel(x, y);
        //             auto new_color = current_color.blend(color.with_alpha(alpha));
        //             m_target->set_pixel(x, y, new_color);
        //             m_target->set_pixel(mirror_x, mirror_y, new_color);
        //         }
        //     }
        // }

        // Line base
        for (int y = p0.y(); y <= p1.y(); y++) {
            if (!clip_rect().contains_vertically(y))
                continue;

            auto x0 = static_cast<int>(floor(current_x - half_thickness));
            auto x1 = static_cast<int>(ceil(current_x + half_thickness));

            for (int x = x0; x <= x1; x++) {
                if (!clip_rect().contains_horizontally(x))
                    continue;

                float error = fabs(current_x - static_cast<float>(x));

                if (error <= half_thickness) {
                    // This pixel is within the full bounds of the line, and
                    // should not be aliased at all
                    m_target->set_pixel(x, y, color);
                } else {
                    error -= half_thickness;
                    VERIFY(error <= 1);
                    auto new_color = m_target->get_pixel(x, y).blend(color.with_alpha(static_cast<u8>(255.0f * (1.0f - error))));
                    m_target->set_pixel(x, y, new_color);
                }
            }

            current_x += slope;
        }
    } else {
        float line_angle = atan2f(static_cast<float>(dy), static_cast<float>(dx));

        // The angle of the line cap, which is perpendicular to line_angle
        float cap_angle = M_PI_2f32 - line_angle;

        // See note [3] above
        float vertical_thickness = thickness / cosf(line_angle);
        auto half_vertical_thickness = vertical_thickness / 2.0f;

        // The left-most point of the cap
        float cap0_dx = cosf(M_PI_4f32 - line_angle) * thickness / M_SQRT2f32;
        // The top-most point of the cap
        float cap0_dy = tanf(M_PI_4f32 - line_angle) * cap0_dx;

        float cap0_x = static_cast<float>(p0.x()) - cap0_dx;
        float cap0_y = static_cast<float>(p0.y()) + cap0_dy;

        float cap1_y = static_cast<float>(p0.y()) - cap0_dx;
        float cap1_x = static_cast<float>(p0.x()) - cap0_dy;

        // We want to cover as much area as possible, as any extra area can be aliased,
        // but missing area will result in a hard non-aliased boundary. So we always
        // round away from the line.
        IntPoint cap0 { floor(cap0_x), ceil(cap0_y) };
        IntPoint cap1 { floor(cap1_x), ceil(cap1_y) };

        dbgln("cap0={}, cap1={}", cap0, cap1);

        // debug_point(cap0, Color::Red);
        // debug_point(cap1, Color::Red);
        // debug_point(p0, Color::Blue);
        // debug_point(p1, Color::Blue);

        int cap_top = cap1.y();
        int cap_bottom = cap1.y() + static_cast<int>(ceil(vertical_thickness));
        dbgln("top={} bottom={}", cap_top, cap_bottom);
        for (int y = cap_top; y < cap_bottom; y++) {
            float x0;
            if (static_cast<float>(y) < cap0_y) {
                auto y_diff = static_cast<float>(y) - cap1_y;
                x0 = cap1_x - y_diff / tanf(cap_angle);
            } else {
                auto y_diff = static_cast<float>(y) - cap0_y;
                x0 = cap0_x + y_diff / tanf(line_angle);
            }

            // Se note [2] above
            float x1 = cap1_x - 1.0f;

            dbgln("y={} x0={} x1={}", y, x0, x1);

            // Always prefer more area vs less, as explained above
            int x0i = floor(x0);
            int x1i = ceil(x1);

            // See note [4] above
            // FIXME: Why is this +1 fudge factor necessary? And does it always work?
            int mirror_y = (p0.y() - y) + p1.y() + 1;

            for (int x = x0i; x <= x1i; x++) {
                // FIXME: Why is this -1 fudge factor necessary? And does it always work?
                int mirror_x = (p0.x() - x) + p1.x() - 1;
                if (x >= x0 && x <= x1) {
                    m_target->set_pixel(x, y, color);
                    m_target->set_pixel(mirror_x, mirror_y, color);
                } else {
                    float error = x < x0 ? x0 - x : x - x1;
                    VERIFY(error >= 0.0f && error <= 1.0f);
                    dbgln("x={} y={} error={}", x, y, error);
                    auto alpha = static_cast<int>(255.0f * (1.0f - error));
                    auto current_color = m_target->get_pixel(x, y);
                    auto new_color = current_color.blend(color.with_alpha(alpha));
                    m_target->set_pixel(x, y, new_color);
                    m_target->set_pixel(mirror_x, mirror_y, new_color);
                }
            }
        }

        // The middle of the line
        auto current_y = static_cast<float>(cap1.y()) + half_vertical_thickness;

        bool print = true;

        for (int x = cap1.x(); x <= p1.x() + cap0_dy; x++) {
            auto y0 = static_cast<int>(floor(current_y - half_vertical_thickness));
            auto y1 = static_cast<int>(ceil(current_y + half_vertical_thickness));

            if (print) {
                print = false;
                dbgln("x={}, y0={}, y1={}", x, y0, y1);
            }

            for (int y = y0; y <= y1; y++) {
                float error = fabs(current_y - static_cast<float>(y));

                if (error <= half_vertical_thickness) {
                    // This pixel is within the full bounds of the line, and
                    // should not be aliased at all
                    m_target->set_pixel(x, y, color);
                } else {
                    error -= half_vertical_thickness;
                    VERIFY(error <= 1);
                    auto new_color = m_target->get_pixel(x, y).blend(color.with_alpha(static_cast<u8>(255.0f * (1.0f - error))));
                    m_target->set_pixel(x, y, new_color);
                }
            }

            current_y += slope;
        }
    }
}

[[maybe_unused]] void AAPainter::draw_path([[maybe_unused]] const Gfx::Path& path, [[maybe_unused]] Color color)
{
}

}
