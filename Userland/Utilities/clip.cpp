/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Path.h>
#include <LibGfx/PathClipping.h>

int main(int, char**)
{
    Gfx::Path path1;
    Gfx::Path path2;

    path1.move_to({ 0, 0 });
    path1.line_to({ 100, 0 });
    path1.line_to({ 130, 80 });
    path1.line_to({ 30, 80 });
    path1.line_to({ 0, 0 });

    path2.move_to({ 40, 30 });
    path2.line_to({ 150, 40 });
    path2.line_to({ 160, 110 });
    path2.line_to({ 60, 110 });
    path2.line_to({ 40, 30 });

    auto poly1 = Gfx::PathClipping::convert_to_polygon(path1, true);

    dbgln();
    dbgln("segments:");
    for (auto& segment : poly1) {
        dbgln("  {{ [{}, {}] self above/below={} {}, other above/below={} {}",
            segment.start,
            segment.end,
            segment.self_fill_above,
            segment.self_fill_below,
            segment.other_fill_above,
            segment.other_fill_below);
    }

    auto poly2 = Gfx::PathClipping::convert_to_polygon(path2, false);
    dbgln();
    dbgln("segments:");
    for (auto& segment : poly2) {
        dbgln("  {{ [{}, {}] self above/below={} {}, other above/below={} {}",
              segment.start,
              segment.end,
              segment.self_fill_above,
              segment.self_fill_below,
              segment.other_fill_above,
              segment.other_fill_below);
    }

    return 0;
}
