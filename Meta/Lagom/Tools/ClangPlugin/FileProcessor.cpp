/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FileProcessor.h"
#include "LambdaCapturePluginAction.h"
#include "LibJSGCPluginAction.h"
#include <atomic>
#include <clang/Tooling/ArgumentsAdjusters.h>

void FileProcessor::run(size_t num_threads)
{
    num_threads = std::min(num_threads, m_file_paths.size());

    for (size_t i = 0; i < num_threads; i++)
        m_threads.emplace_back(&FileProcessor::process, this);

    for (auto& thread : m_threads)
        thread.join();
}

void FileProcessor::process()
{
    while (true) {
        size_t index;
        {
            std::lock_guard guard { m_mutex };
            if (m_next_file_path_index >= m_file_paths.size())
                return;

            index = m_next_file_path_index++;
        }

        std::string path = m_file_paths[index];
        clang::tooling::ClangTool tool(m_db, { path });
        tool.appendArgumentsAdjuster([](clang::tooling::CommandLineArguments const& input_args, llvm::StringRef) {
            // We want to insert arguments, but not at the very start, since that will be the binary name
            clang::tooling::CommandLineArguments args;

            args.push_back(input_args.front());
            args.push_back("-DNULL=0");
            args.push_back("-DUSING_AK_GLOBALLY=1");
            args.insert(args.end(), input_args.begin() + 1, input_args.end());

            return args;
        });

        run_plugin_action<LambdaCapturePluginAction>(tool, path);
        run_plugin_action<LibJSGCPluginAction>(tool, path);
    }
}
