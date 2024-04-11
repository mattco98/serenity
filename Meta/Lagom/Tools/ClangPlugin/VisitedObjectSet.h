/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <llvm/Support/raw_ostream.h>
#include <mutex>
#include <unordered_set>

namespace {
std::mutex s_mutex;
}

template<typename T>
class VisitedObjectSet {
public:
    bool has_visited(T value)
    {
        std::lock_guard guard { s_mutex };
        llvm::outs() << "mutex addr: " << &s_mutex << "\n";
        if (m_visited_objects.contains(value))
            return true;
        m_visited_objects.insert(value);
        return false;
    }

private:
    std::unordered_set<T> m_visited_objects;
};
