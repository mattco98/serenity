/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DemoList.h"

#define MAKE_PATH(...)                              \
    Gfx::Path path;                                 \
    Vector<Gfx::FloatPoint> points { __VA_ARGS__ }; \
    path.move_to(points[0] * GRID_SPACING);         \
    for (size_t i = 1; i < points.size(); i++)      \
        path.line_to(points[i] * GRID_SPACING);     \
    path.line_to(points[0] * GRID_SPACING);

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

Gfx::Path DemoList::get_primary_path(size_t index)
{
    if (!m_initialized)
        initialize();
    return m_primary_paths[index];
}

Gfx::Path DemoList::get_secondary_path(size_t index)
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

    // Simple parallelograms
    MAKE_PRIMARY_PATH({ 2, 2 }, { 7, 2 }, { 8, 6 }, { 3, 6 });
    MAKE_SECONDARY_PATH({ 4, 4 }, { 10, 5 }, { 10, 9 }, { 5, 7 });

    VERIFY(m_primary_paths.size() == m_secondary_paths.size());
}
