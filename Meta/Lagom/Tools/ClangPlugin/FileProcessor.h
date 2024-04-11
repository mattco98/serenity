/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <clang/Basic/Diagnostic.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class FileProcessor {
public:
    FileProcessor(clang::tooling::CompilationDatabase& db, std::vector<std::string> const& file_paths)
        : m_file_paths(file_paths)
        , m_db(db)
    {
    }

    void run(size_t num_threads);

private:
    void process();

    std::vector<std::string> const& m_file_paths;
    clang::tooling::CompilationDatabase& m_db;

    size_t m_next_file_path_index { 0 };
    std::vector<std::thread> m_threads;
    std::mutex m_mutex;
};
