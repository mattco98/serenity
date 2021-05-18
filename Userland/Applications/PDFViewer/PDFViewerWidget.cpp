/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PDFViewerWidget.h"
#include <AK/Optional.h>
#include <AK/StringBuilder.h>
#include <LibCore/File.h>
#include <LibGUI/Action.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/FilePicker.h>
#include <LibGUI/Menu.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/Statusbar.h>
#include <LibGUI/TextBox.h>
#include <LibGUI/ToolbarContainer.h>
#include <LibGUI/Toolbar.h>

PDFViewerWidget::PDFViewerWidget()
{
    set_fill_with_background_color(true);
    set_layout<GUI::VerticalBoxLayout>();

    create_toolbar();
    m_viewer = add<PDFViewer>();
}

PDFViewerWidget::~PDFViewerWidget()
{
}

void PDFViewerWidget::initialize_menubar(GUI::Menubar& menubar)
{
    auto& file_menu = menubar.add_menu("File");
    file_menu.add_action(GUI::CommonActions::make_open_action([&](auto&) {
        Optional<String> open_path = GUI::FilePicker::get_open_filepath(window());
        if (open_path.has_value())
            open_file(open_path.value());
    }));
    file_menu.add_action(GUI::CommonActions::make_quit_action([](auto&) {
        GUI::Application::the()->quit();
    }));
}

void PDFViewerWidget::open_file(const String& path)
{
    window()->set_title(String::formatted("{} - PDFViewer", path));
    auto file_result = Core::File::open(path, Core::IODevice::OpenMode::ReadOnly);
    VERIFY(!file_result.is_error());
    m_buffer = file_result.value()->read_all();
    m_viewer->set_document(PDF::Document::from(m_buffer));
}

void PDFViewerWidget::create_toolbar()
{
    auto& toolbar_container = add<GUI::ToolbarContainer>();
    auto& toolbar = toolbar_container.add<GUI::Toolbar>();

    auto previous_page_action = GUI::Action::create("Go to &Previous page", { Mod_Ctrl, Key_Up }, Gfx::Bitmap::load_from_file("/res/icons/16x16/go-up.png"), [&](auto&) {
        m_viewer->go_to_previous_page();
    }, nullptr);
    previous_page_action->set_status_tip("Go to the previous page");

    auto next_page_action = GUI::Action::create("Go to &Next page", { Mod_Ctrl, Key_Down }, Gfx::Bitmap::load_from_file("/res/icons/16x16/go-down.png"), [&](auto&) {
        m_viewer->go_to_next_page();
    }, nullptr);
    next_page_action->set_status_tip("Go to the next page");

    toolbar.add_action(*previous_page_action);
    toolbar.add_action(*next_page_action);

    auto& text_box = toolbar.add<GUI::TextBox>();
    dbgln("{} {} {} {}", text_box.width(), text_box.content_width(), text_box.max_width(), text_box.min_width());
    dbgln("{} {} {} {}", text_box.height(), text_box.content_height(), text_box.max_height(), text_box.min_height());
    text_box.set_width(text_box.height());
}
