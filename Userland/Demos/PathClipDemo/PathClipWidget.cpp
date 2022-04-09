/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PathClipWidget.h"
#include <LibGUI/Action.h>
#include <LibGUI/ActionGroup.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Menu.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/Toolbar.h>
#include <LibGUI/ToolbarContainer.h>
#include <LibGUI/Window.h>

PathClipWidget::PathClipWidget()
{
    set_layout<GUI::VerticalBoxLayout>();
    add_toolbar();

    m_splitter = add<GUI::HorizontalSplitter>();
    m_input_grid = m_splitter->add<InputGridWidget>();
    m_output_grid = m_splitter->add<OutputGridWidget>();

    m_input_grid->on_input_paths_changed = [&](Gfx::Path& primary, Gfx::Path& secondary) {
        m_output_grid->update(primary, secondary);
    };

    load_current_demo();
}

void PathClipWidget::add_toolbar()
{
    auto& toolbar_container = add<GUI::ToolbarContainer>();
    auto& toolbar = toolbar_container.add<GUI::Toolbar>();
    toolbar.add_action(GUI::CommonActions::make_go_back_action([this](auto&) {
        go_to_previous_demo();
    }));
    toolbar.add_action(GUI::CommonActions::make_go_forward_action([this](auto&) {
        go_to_next_demo();
    }));
}

static String dump_path(Gfx::Path const& path, bool is_primary)
{
    StringBuilder builder;
    builder.appendff("MAKE_{}_PATH(", is_primary ? "PRIMARY" : "SECONDARY");
    bool first = true;
    auto& segments = path.segments();

    // Skip the last point because it is the same as the first
    for (size_t i = 0; i < segments.size() - 1; i++) {
        auto& segment = segments[i];

        if (!first)
            builder.append(", ");
        first = false;

        switch (segment.type()) {
        case Gfx::Segment::Type::MoveTo:
        case Gfx::Segment::Type::LineTo:
            builder.appendff("{{ {}, {} }}", segment.point().x(), segment.point().y());
            break;
        default:
            VERIFY_NOT_REACHED();
        }
    }

    builder.append(");");

    return builder.to_string();
}

void PathClipWidget::initialize_menubar(GUI::Window& window)
{
    auto& file_menu = window.add_menu("&File");
    file_menu.add_separator();
    file_menu.add_action(GUI::CommonActions::make_quit_action([](auto&) {
        GUI::Application::the()->quit();
    }));

    auto& shapes_menu = window.add_menu("&Shapes");

    m_clip_type_group.set_exclusive(true);
    auto& clip_menu = shapes_menu.add_submenu("&Clip Type");

    auto intersection_action = GUI::Action::create_checkable("Intersection", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::PathClipping::ClipType::Intersection);
    });
    auto union_action = GUI::Action::create_checkable("Union", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::PathClipping::ClipType::Union);
    });
    auto difference_action = GUI::Action::create_checkable("Difference", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::PathClipping::ClipType::Difference);
    });
    auto difference_reversed_action = GUI::Action::create_checkable("Difference (reversed)", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::PathClipping::ClipType::DifferenceReversed);
    });
    auto xor_action = GUI::Action::create_checkable("Xor", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::PathClipping::ClipType::Xor);
    });

    m_clip_type_group.add_action(*intersection_action);
    m_clip_type_group.add_action(*union_action);
    m_clip_type_group.add_action(*difference_action);
    m_clip_type_group.add_action(*difference_reversed_action);
    m_clip_type_group.add_action(*xor_action);
    clip_menu.add_action(*intersection_action);
    clip_menu.add_action(*union_action);
    clip_menu.add_action(*difference_action);
    clip_menu.add_action(*difference_reversed_action);
    clip_menu.add_action(*xor_action);

    intersection_action->set_checked(true);

    shapes_menu.add_separator();

    shapes_menu.add_action(GUI::Action::create("Add Point to &Primary Shape", [&](auto&) {
        m_input_grid->add_point(true);
    }));

    shapes_menu.add_action(GUI::Action::create("Add Point to &Secondary Shape", [&](auto&) {
        m_input_grid->add_point(false);
    }));

    shapes_menu.add_action(GUI::Action::create("&Dump Input Path Macros", [&](auto&) {
        dbgln("{}", dump_path(m_input_grid->primary_path(), true));
        dbgln("{}", dump_path(m_input_grid->secondary_path(), false));
    }));

    // shapes_menu.add_separator();
    // shapes_menu.add_action(GUI::Action::create("Enable Debug", [](auto&) {
    //     Gfx::PathClipping::debug = true;
    // }));
    // shapes_menu.add_action(GUI::Action::create("Disable Debug", [](auto&) {
    //     Gfx::PathClipping::debug = false;
    // }));
}

void PathClipWidget::go_to_next_demo()
{
    if (m_demo_index <= DemoList::path_count() - 1) {
        m_demo_index++;
        load_current_demo();
    }
}

void PathClipWidget::go_to_previous_demo()
{
    if (m_demo_index > 0) {
        m_demo_index--;
        load_current_demo();
#        include "PathClipWidget.h"
    }
}

void PathClipWidget::load_current_demo()
{
    set_input_paths(DemoList::get_primary_path(m_demo_index), DemoList::get_secondary_path(m_demo_index));
}

void PathClipWidget::set_input_paths(Gfx::Path& primary_path, Gfx::Path& secondary_path)
{
    m_input_grid->set_primary_path(primary_path);
    m_input_grid->set_secondary_path(secondary_path);
    m_output_grid->update(primary_path, secondary_path);
}
