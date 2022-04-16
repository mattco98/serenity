/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Path.h>
#include <LibGfx/PathClipping.h>
#include <LibMain/Main.h>

#define MAKE_PATH(...)                                  \
    ({                                                  \
        Gfx::Path path;                                 \
        Vector<Gfx::FloatPoint> points { __VA_ARGS__ }; \
        path.move_to(points[0]);                        \
        for (size_t i = 1; i < points.size(); i++)      \
            path.line_to(points[i]);                    \
        path.line_to(points[0]);                        \
        path;                                           \
    })

ErrorOr<int> serenity_main(Main::Arguments)
{
    auto primary_path = MAKE_PATH({ 40, 120 }, { 300, 120 }, { 300, 200 }, { 40, 200 });
    auto secondary_path = MAKE_PATH({ 100, 60 }, { 240, 60 }, { 300, 120 }, { 100, 120 });
    auto primary_polygon = Gfx::PathClipping::convert_to_polygon(primary_path, true);
    auto secondary_polygon = Gfx::PathClipping::convert_to_polygon(secondary_path, false);
    auto combined = Gfx::PathClipping::combine(primary_polygon, secondary_polygon);

    return 0;
}
