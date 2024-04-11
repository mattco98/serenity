/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "DiagConsumer.h"
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

    int run(size_t num_threads);

private:
    void process();

    template<typename T>
    void run_plugin_action(clang::tooling::ClangTool& tool, std::string const& path)
    {
        if (!s_test_mode) {
            std::lock_guard guard { m_print_mutex };
            llvm::outs() << "\033[38;5;48m[" << T::action_name() << "]\033[0m Processing " << path << "\n";
        }

        auto action = clang::tooling::newFrontendActionFactory<T>();
        (void)tool.run(action.get());
    }

    std::vector<std::string> const& m_file_paths;
    clang::tooling::CompilationDatabase& m_db;

    size_t m_next_file_path_index { 0 };
    std::vector<std::thread> m_threads;
    std::mutex m_mutex;
    std::mutex m_print_mutex;
};
