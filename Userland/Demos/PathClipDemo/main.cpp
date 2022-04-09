/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PathClipWidget.h"
#include <LibGUI/Application.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/Window.h>

int main(int argc, char** argv)
{
   auto app = GUI::Application::construct(argc, argv);

   auto window = GUI::Window::construct();
   window->set_title("Path Clipper Demo");
   window->resize(640, 400);

   auto& path_clipper_widget = window->set_main_widget<PathClipWidget>();
   auto menubar = GUI::Menubar::construct();
   path_clipper_widget.initialize_menubar(*window);
   window->show();

   return app->exec();
}
