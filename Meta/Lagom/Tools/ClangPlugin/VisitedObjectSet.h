/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <mutex>
#include <unordered_set>

template<typename T>
class VisitedObjectSet {
public:
    bool has_visited(T value)
    {
        std::lock_guard guard { m_mutex };
        if (m_visited_objects.contains(value))
            return true;
        m_visited_objects.insert(value);
        return false;
    }

private:
    std::mutex m_mutex;
    std::unordered_set<T> m_visited_objects;
};
