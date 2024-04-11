/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FileProcessor.h"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <llvm/Support/CommandLine.h>
#include <thread>

llvm::cl::OptionCategory s_tool_category { "ClangPlugin", "A plugin that performs SerenityOS-specific analysis" };

llvm::cl::opt<bool> s_use_lagom_build {
    "lagom",
    llvm::cl::desc("Use the lagom build instead of the x86_64clang build"),
    llvm::cl::cat(s_tool_category),
};

llvm::cl::list<std::string> s_file_paths {
    llvm::cl::Positional,
    llvm::cl::desc("<source files>"),
    llvm::cl::cat(s_tool_category),
};

llvm::cl::opt<size_t> s_num_threads {
    "j",
    llvm::cl::desc("Number of threads to use"),
    llvm::cl::init(std::max(std::thread::hardware_concurrency(), 1u)),
    llvm::cl::cat(s_tool_category),
};

std::optional<std::vector<std::string>> get_source_path_list(std::filesystem::path const& project_root)
{
    std::vector<std::string> paths;
    for (auto const& path : s_file_paths) {
        if (std::filesystem::path(path).is_absolute()) {
            paths.push_back(path);
        } else {
            paths.push_back(project_root / path);
        }
    }

    return paths;
}

std::unique_ptr<clang::tooling::CompilationDatabase> get_compilation_database(std::filesystem::path const& project_root)
{
    auto const* build_folder = s_use_lagom_build ? "lagom" : "x86_64clang";
    auto compile_commands_path = project_root / "Build" / build_folder / "compile_commands.json";
    if (!std::filesystem::is_regular_file(compile_commands_path)) {
        llvm::errs() << "Could not find compile_commands.json file in " << compile_commands_path.parent_path().string() << "; did you forget to build?\n";
        return {};
    }

    std::string err;
    auto db = clang::tooling::CompilationDatabase::autoDetectFromSource(compile_commands_path.string(), err);

    if (!err.empty()) {
        llvm::errs() << "Failed to load compile_commands.json: " << err << "\n";
        return {};
    }

    return db;
}

int main(int argc, char const** argv)
{
    auto* serenity_source_dir = std::getenv("SERENITY_SOURCE_DIR");
    if (!serenity_source_dir) {
        llvm::errs() << "ClangPlugin requires the SERENITY_SOURCE_DIR environment variable to be set\n";
        return {};
    }

    llvm::cl::HideUnrelatedOptions(s_tool_category);
    llvm::cl::ParseCommandLineOptions(argc, argv);

    auto compilation_database = get_compilation_database(serenity_source_dir);
    if (!compilation_database)
        return 1;

    auto source_paths = get_source_path_list(serenity_source_dir);
    if (!source_paths)
        return 1;

    FileProcessor processor { *compilation_database, *source_paths };
    processor.run(s_num_threads);
    return 0;
}
