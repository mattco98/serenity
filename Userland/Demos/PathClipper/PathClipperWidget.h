/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "GridWidget.h"
#include "DemoList.h"
#include <LibGUI/ActionGroup.h>
#include <LibGUI/Splitter.h>
#include <LibGUI/Widget.h>
#include <LibGfx/Color.h>
#include <LibGfx/Path.h>
#include <LibGfx/Point.h>

class PathClipperWidget final : public GUI::Widget {
    C_OBJECT(PathClipperWidget)

public:
    PathClipperWidget();
    virtual ~PathClipperWidget() = default;

    void initialize_menubar(GUI::Menubar&);

    void go_to_next_demo();
    void go_to_previous_demo();

private:
    void load_current_demo();
    void set_input_paths(Gfx::Path& primary, Gfx::Path& secondary);

    GUI::ActionGroup m_clip_type_group;
    RefPtr<GUI::HorizontalSplitter> m_splitter;
    RefPtr<InputGridWidget> m_input_grid;
    RefPtr<OutputGridWidget> m_output_grid;
    bool m_draw_grid { true };
    size_t m_demo_index { 0 };
};
