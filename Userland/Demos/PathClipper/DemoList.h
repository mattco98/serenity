/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGfx/Path.h>
#include <LibGfx/Point.h>

static constexpr int GRID_SPACING = 20;

struct Entry {
    Vector<Gfx::FloatPoint> primary_points;
    Vector<Gfx::FloatPoint> secondary_points;
};

class DemoList {
public:
    static Gfx::Path& get_primary_path(size_t index);
    static Gfx::Path& get_secondary_path(size_t index);
    static size_t path_count();

private:
    static void initialize();

    static Vector<Gfx::Path> m_primary_paths;
    static Vector<Gfx::Path> m_secondary_paths;
    static bool m_initialized;
};
