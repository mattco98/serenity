/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PathClipperWidget.h"
#include <LibGUI/Action.h>
#include <LibGUI/ActionGroup.h>
#include <LibGUI/Application.h>
#include <LibGUI/Menu.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/BoxLayout.h>

PathClipperWidget::PathClipperWidget()
{
    set_layout<GUI::VerticalBoxLayout>();
    m_splitter = add<GUI::HorizontalSplitter>();
    m_input_grid = m_splitter->add<InputGridWidget>();
    m_output_grid = m_splitter->add<OutputGridWidget>();

    set_input_paths();
}

void PathClipperWidget::initialize_menubar(GUI::Menubar& menubar)
{
    auto& file_menu = menubar.add_menu("&File");
    file_menu.add_action(GUI::CommonActions::make_go_back_action([this](auto&) {
        go_to_previous_demo();
    }));
    file_menu.add_action(GUI::CommonActions::make_go_forward_action([this](auto&) {
        go_to_next_demo();
    }));
    file_menu.add_separator();
    file_menu.add_action(GUI::CommonActions::make_quit_action([](auto&) {
        GUI::Application::the()->quit();
    }));

    auto& view_menu = menubar.add_menu("&View");

    m_clip_type_group.set_exclusive(true);
    auto& clip_menu = view_menu.add_submenu("&Clip Type");

    auto intersection_action = GUI::Action::create_checkable("Intersection", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::ClipType::Intersection);
    });
    auto union_action = GUI::Action::create_checkable("Union", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::ClipType::Union);
    });
    auto difference_action = GUI::Action::create_checkable("Difference", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::ClipType::Difference);
    });
    auto difference_reversed_action = GUI::Action::create_checkable("Difference (reversed)", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::ClipType::DifferenceReversed);
    });
    auto xor_action = GUI::Action::create_checkable("Xor", [this](auto&) {
        m_output_grid->set_clip_type(Gfx::ClipType::Xor);
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
}

void PathClipperWidget::go_to_next_demo()
{
    if (m_demo_index <= DemoList::path_count() - 1) {
        m_demo_index++;
        set_input_paths();
    }
}

void PathClipperWidget::go_to_previous_demo()
{
    if (m_demo_index > 0) {
        m_demo_index--;
        set_input_paths();
    }
}

void PathClipperWidget::set_input_paths()
{
    auto primary_path = DemoList::get_primary_path(m_demo_index);
    auto secondary_path = DemoList::get_secondary_path(m_demo_index);

    m_input_grid->set_primary_path(primary_path);
    m_input_grid->set_secondary_path(secondary_path);
    m_output_grid->update(primary_path, secondary_path);
}
