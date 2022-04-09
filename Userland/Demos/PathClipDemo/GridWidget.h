/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "DemoList.h"
#include <LibGUI/Frame.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Widget.h>
#include <LibGfx/Path.h>
#include <LibGfx/PathClipping.h>

static constexpr Color GRID_COLOR { 0xb0, 0xb0, 0xb0 };
static constexpr Color BACKGROUND_COLOR { 0xff, 0xff, 0xff };

static constexpr Color PRIMARY_STROKE_COLOR { 0xff, 0x66, 0x66, 0xa0 };
static constexpr Color PRIMARY_FILL_COLOR { 0xff, 0xcc, 0xcc, 0xa0 };
static constexpr Color SECONDARY_STROKE_COLOR { 0x66, 0x66, 0xff, 0xa0 };
static constexpr Color SECONDARY_FILL_COLOR { 0xcc, 0xcc, 0xff, 0xa0 };
static constexpr Color RESULT_STROKE_COLOR { 0x26, 0xbb, 0x26 };
static constexpr Color RESULT_FILL_COLOR { 0x4c, 0xff, 0x4c };

class GridWidget : public GUI::Widget {
   C_OBJECT(GridWidget)

public:
   GridWidget();
   virtual ~GridWidget() = default;

   void set_grid_enabled(bool enabled) { m_grid_enabled = enabled; }

protected:
   void draw_path(GUI::Painter&, Gfx::Path& path, Color stroke_color, Color fill_color);

   virtual void paint_event(GUI::PaintEvent&) override;

private:
   void draw_grid(GUI::Painter&) const;

   bool m_is_result_grid;
   bool m_grid_enabled { true };
};

class InputGridWidget final : public GridWidget {
   C_OBJECT(InputGridWidget)

public:
   InputGridWidget();
   virtual ~InputGridWidget() = default;

   const Gfx::Path& primary_path() const { return m_primary_path; }
   const Gfx::Path& secondary_path() const { return m_secondary_path; }

   void set_primary_path(Gfx::Path& path)
   {
       update();
       m_primary_path = path;
   }

   void set_secondary_path(Gfx::Path& path)
   {
       update();
       m_secondary_path = path;
   }

   void add_point(bool primary_path);

   // Called when a user drags the path
   Function<void(Gfx::Path& primary, Gfx::Path& secondary)> on_input_paths_changed;

private:
   virtual void paint_event(GUI::PaintEvent&) override;
   virtual void mousedown_event(GUI::MouseEvent&) override;
   virtual void mouseup_event(GUI::MouseEvent&) override;
   virtual void mousemove_event(GUI::MouseEvent&) override;

   Gfx::IntPoint get_closest_grid_point_to(const Gfx::IntPoint& point) const;

   Gfx::Path m_primary_path;
   Gfx::Path m_secondary_path;

   Optional<Gfx::FloatPoint> m_point_being_dragged;
   bool m_primary_path_being_dragged { false };
};

class OutputGridWidget final : public GridWidget {
   C_OBJECT(OutputGridWidget)

public:
   OutputGridWidget();
   virtual ~OutputGridWidget() = default;

   const Vector<Gfx::Path>& paths() const { return m_paths; }

   void update(Gfx::Path& primary, Gfx::Path& secondary);

   void set_clip_type(Gfx::PathClipping::ClipType type);

private:
   virtual void paint_event(GUI::PaintEvent&) override;

   Gfx::PathClipping::ClipType m_clip_type { Gfx::PathClipping::ClipType::Intersection };
   Gfx::PathClipping::Polygon m_polygon;
   Vector<Gfx::Path> m_paths;
};
