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
        // FIXME: Draw a circle
        auto pos = split_line.from - Gfx::FloatPoint { point_size.width() / 2, point_size.height() / 2 };
        painter.fill_ellipse({ pos.to_type<int>(), point_size }, stroke_color);
    }
}

InputGridWidget::InputGridWidget()
{
}

void InputGridWidget::paint_event(GUI::PaintEvent& event)
{
    GridWidget::paint_event(event);

    GUI::Painter painter(*this);

    draw_path(painter, m_primary_path, PRIMARY_STROKE_COLOR, PRIMARY_FILL_COLOR);
    draw_path(painter, m_secondary_path, SECONDARY_STROKE_COLOR, SECONDARY_FILL_COLOR);
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

void OutputGridWidget::update(Gfx::Path& primary, Gfx::Path& secondary)
{
    auto primary_poly = Gfx::PathClipping::convert_to_polygon(primary, true);
    auto secondary_poly = Gfx::PathClipping::convert_to_polygon(secondary, false);
    auto combined = Gfx::PathClipping::combine(primary_poly, secondary_poly);
    m_paths = Gfx::PathClipping::select_segments(combined, m_clip_type);
}
