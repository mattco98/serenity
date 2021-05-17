/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GridWidget.h"

GridWidget::GridWidget()
{
    set_fill_with_background_color(true);
}

void GridWidget::paint_event(GUI::PaintEvent& event)
{
    GUI::Widget::paint_event(event);
    GUI::Painter painter(*this);

    painter.fill_rect(rect(), BACKGROUND_COLOR);

    if (m_grid_enabled)
        draw_grid(painter);
}

void GridWidget::draw_grid(GUI::Painter& painter) const
{
    for (int y = GRID_SPACING; y < height(); y += GRID_SPACING)
        painter.draw_line({ 0, y }, { width(), y }, GRID_COLOR, 1);

    for (int x = GRID_SPACING; x < width(); x += GRID_SPACING)
        painter.draw_line({ x, 0 }, { x, height() }, GRID_COLOR, 1);
}

void GridWidget::draw_path(GUI::Painter& painter, Gfx::Path& path, Color stroke_color, Color fill_color)
{
    painter.fill_path(path, fill_color);
    painter.stroke_path(path, stroke_color, 2);

    auto split_lines = path.split_lines();
    Gfx::IntSize point_size { 7, 7 };

    for (auto& split_line : split_lines) {
        auto pos = split_line.from - Gfx::FloatPoint { point_size.width() / 2, point_size.height() / 2 };
        painter.fill_ellipse({ pos.to_type<int>(), point_size }, stroke_color);
    }
}

InputGridWidget::InputGridWidget()
{
}

// Adds a point between the first and second points
void InputGridWidget::add_point(bool primary_path)
{
    auto& path = primary_path ? m_primary_path : m_secondary_path;
    Gfx::Path new_path;
    Gfx::FloatPoint cursor;
    bool add_to_next_point = true;

    auto& segments = path.segments();
    for (auto& segment : segments) {
        switch (segment.type()) {
        case Gfx::Segment::Type::MoveTo:
            new_path.move_to(segment.point());
            cursor = segment.point();
            break;
        case Gfx::Segment::Type::LineTo: {
            if (add_to_next_point) {
                auto mid = (cursor + segment.point()) / 2.0f;
                auto aligned_mid = get_closest_grid_point_to(mid.to_type<int>());
                if (aligned_mid != cursor && aligned_mid != segment.point()) {
                    new_path.line_to(aligned_mid.to_type<float>());
                    new_path.line_to(segment.point());
                    cursor = segment.point();
                    add_to_next_point = false;
                    continue;
                }
            }
            new_path.line_to(segment.point());
            cursor = segment.point();
            break;
        }
        default:
            VERIFY_NOT_REACHED();
        }
    }

    if (primary_path) {
        m_primary_path = new_path;
    } else {
        m_secondary_path = new_path;
    }

    update();
}

void InputGridWidget::paint_event(GUI::PaintEvent& event)
{
    GridWidget::paint_event(event);

    GUI::Painter painter(*this);

    draw_path(painter, m_primary_path, PRIMARY_STROKE_COLOR, PRIMARY_FILL_COLOR);
    draw_path(painter, m_secondary_path, SECONDARY_STROKE_COLOR, SECONDARY_FILL_COLOR);
}

void InputGridWidget::mousedown_event(GUI::MouseEvent& event)
{
    auto dragged_point = get_closest_grid_point_to(event.position());

    for (auto& line : m_primary_path.split_lines()) {
        if (dragged_point == line.from || dragged_point == line.to) {
            m_point_being_dragged = dragged_point;
            m_primary_path_being_dragged = true;
            return;
        }
    }

    for (auto& line : m_secondary_path.split_lines()) {
        if (dragged_point == line.from || dragged_point == line.to) {
            m_point_being_dragged = dragged_point;
            m_primary_path_being_dragged = false;
            return;
        }
    }
}

void InputGridWidget::mouseup_event(GUI::MouseEvent&)
{
    m_point_being_dragged = {};
}

void InputGridWidget::mousemove_event(GUI::MouseEvent& event)
{
    if (!m_point_being_dragged.has_value())
        return;

    auto dragged_point = m_point_being_dragged.value();
    auto current_point = get_closest_grid_point_to(event.position());
    if (current_point == dragged_point)
        return;

    // FIXME: Add a way to modify a path without essentially recreating the entire
    // thing, because this is pretty expensive.

    auto& path = m_primary_path_being_dragged ? m_primary_path : m_secondary_path;

    Gfx::Path new_path;
    for (auto& segment : path.segments()) {
        auto point = segment.point();
        if (point == dragged_point)
            point = current_point.to_type<float>();

        switch (segment.type()) {
        case Gfx::Segment::Type::MoveTo:
            new_path.move_to(point);
            break;
        case Gfx::Segment::Type::LineTo:
            new_path.line_to(point);
            break;
        default:
            VERIFY_NOT_REACHED();
        }
    }

    if (m_primary_path_being_dragged) {
        m_primary_path = new_path;
    } else {
        m_secondary_path = new_path;
    }

    m_point_being_dragged = current_point;

    update();
    on_input_paths_changed(m_primary_path, m_secondary_path);
}

// static float round_to_grid_spacing(float f)
// {
//     if (f < 0)
//         return 0;
//
//     if (f == floorf(f) && static_cast<int>(f) % GRID_SPACING == 0)
//         return f;
//
//     auto r = fmodf(f, static_cast<float>(GRID_SPACING));
//     auto left = f - r;
//     if (r < GRID_SPACING / 2.0f)
//         return left;
//     return left + GRID_SPACING;
// }

static int round_to_grid_spacing(int n)
{
    if (n < 0)
        return 0;

    if (n % GRID_SPACING == 0)
        return n;

    auto r = n % GRID_SPACING;
    auto left = n - r;
    if (r < GRID_SPACING / 2)
        return left;
    return left + GRID_SPACING;
}

Gfx::IntPoint InputGridWidget::get_closest_grid_point_to(const Gfx::IntPoint& point) const
{
    return {
        round_to_grid_spacing(point.x()),
        round_to_grid_spacing(point.y()),
    };
}

OutputGridWidget::OutputGridWidget()
{
}

void OutputGridWidget::paint_event(GUI::PaintEvent& event)
{
    GridWidget::paint_event(event);

    GUI::Painter painter(*this);

    for (auto& path : m_paths)
        draw_path(painter, path, RESULT_STROKE_COLOR, RESULT_FILL_COLOR);
}

void OutputGridWidget::set_clip_type(Gfx::ClipType type)
{
    m_clip_type = type;
    m_paths = Gfx::PathClipping::select_segments(m_polygon, type);
    GridWidget::update();
}

void OutputGridWidget::update(Gfx::Path& primary, Gfx::Path& secondary)
{
    auto primary_poly = Gfx::PathClipping::convert_to_polygon(primary, true);
    auto secondary_poly = Gfx::PathClipping::convert_to_polygon(secondary, false);
    m_polygon = Gfx::PathClipping::combine(primary_poly, secondary_poly);
    m_paths = Gfx::PathClipping::select_segments(m_polygon, m_clip_type);
    GridWidget::update();
}

namespace AK {

template<typename T>
struct Formatter<Vector<T>> : Formatter<StringView> {
    void format(FormatBuilder& format_builder, const Vector<T>& vec)
    {
        StringBuilder builder;
        builder.append("[\n");
        for (auto& el : vec)
            builder.appendff("  {}\n", el);
        builder.append(']');
        return Formatter<StringView>::format(format_builder, builder.to_string());
    }
};

}
