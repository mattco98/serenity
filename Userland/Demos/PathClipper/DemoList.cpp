/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DemoList.h"

#define MAKE_PATH(...)                              \
    Gfx::Path path;                                 \
    Vector<Gfx::FloatPoint> points { __VA_ARGS__ }; \
    path.move_to(points[0]);                        \
    for (size_t i = 1; i < points.size(); i++)      \
        path.line_to(points[i]);                    \
    path.line_to(points[0]);

#define MAKE_PRIMARY_PATH(...)        \
    {                                 \
        MAKE_PATH(__VA_ARGS__)        \
        m_primary_paths.append(path); \
    }

#define MAKE_SECONDARY_PATH(...)        \
    {                                   \
        MAKE_PATH(__VA_ARGS__)          \
        m_secondary_paths.append(path); \
    }

bool DemoList::m_initialized = false;
Vector<Gfx::Path> DemoList::m_primary_paths = {};
Vector<Gfx::Path> DemoList::m_secondary_paths = {};

Gfx::Path& DemoList::get_primary_path(size_t index)
{
    if (!m_initialized)
        initialize();
    return m_primary_paths[index];
}

Gfx::Path& DemoList::get_secondary_path(size_t index)
{
    if (!m_initialized)
        initialize();
    return m_secondary_paths[index];
}

size_t DemoList::path_count()
{
    if (!m_initialized)
        initialize();
    return m_primary_paths.size();
}

void DemoList::initialize()
{
    VERIFY(!m_initialized);
    m_initialized = true;

    // // Simple parallelograms
    // MAKE_PRIMARY_PATH({ 40, 40 }, { 140, 40 }, { 160, 120 }, { 60, 120 });
    // MAKE_SECONDARY_PATH({ 60, 80 }, { 200, 100 }, { 200, 180 }, { 100, 140 });
    //
    // // Rectangles with vertical lines
    // MAKE_PRIMARY_PATH({ 40, 100 }, { 180, 100 }, { 180, 200 }, { 40, 200 });
    // MAKE_SECONDARY_PATH({ 60, 120 }, { 10, 120 }, { 200, 220 }, { 60, 220 });

    // Rectangles with a shared left edge (secondary side fully enclosed in primary side)
    MAKE_PRIMARY_PATH({ 40, 100 }, { 180, 100 }, { 180, 200 }, { 40, 200 });
    MAKE_SECONDARY_PATH({ 40, 120 }, { 160, 120 }, { 160, 180 }, { 40, 180 });

    VERIFY(m_primary_paths.size() == m_secondary_paths.size());
}
