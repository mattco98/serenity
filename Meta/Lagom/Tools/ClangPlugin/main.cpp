/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FileProcessor.h"
#include <cassert>
#include <clang/Tooling/CommonOptionsParser.h>
#include <filesystem>
#include <llvm/Support/CommandLine.h>
#include <thread>

llvm::cl::OptionCategory s_tool_category { "ClangPlugin", "A plugin that performs SerenityOS-specific analysis" };

llvm::cl::opt<bool> s_test_mode {
    "test-mode",
    llvm::cl::desc("Run the plugin in a manner more hospitable to the test runner"),
    llvm::cl::init(false),
    llvm::cl::cat(s_tool_category)
};

llvm::cl::opt<bool> s_use_lagom_build {
    "lagom",
    llvm::cl::desc("Use the lagom build instead of the x86_64clang build"),
    llvm::cl::cat(s_tool_category),
};

enum class ScanType {
    None,
    JS,
    All,
};
llvm::cl::opt<ScanType> s_scan_type {
    "scan",
    llvm::cl::desc("Automatically find files to analyze"),
    llvm::cl::values(
        clEnumValN(ScanType::None, "none", "Scan only the files provided by the user"),
        clEnumValN(ScanType::JS, "js", "Scan all JS-related files"),
        clEnumValN(ScanType::All, "all", "Scan the entire SerenityOS source tree")),
    llvm::cl::init(ScanType::None),
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

void scan_directory_for_relevant_files(std::vector<std::string>& paths, std::filesystem::path const& path)
{
    assert(std::filesystem::is_directory(path));

    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(path)) {
        if (!dir_entry.is_regular_file())
            continue;

        auto const& file_path = dir_entry.path();
        if (file_path.extension() == ".h" || file_path.extension() == ".cpp")
            paths.push_back(file_path.string());
    }
}

std::optional<std::vector<std::string>> get_source_path_list(std::filesystem::path const& project_root)
{
    if (s_scan_type == ScanType::None && s_file_paths.empty()) {
        llvm::errs() << "Expected at least one source file to be specified without --scan\n";
        return {};
    }

    std::vector<std::string> paths;
    for (auto const& path : s_file_paths) {
        if (std::filesystem::path(path).is_absolute()) {
            paths.push_back(path);
        } else {
            paths.push_back(project_root / path);
        }
    }

    if (s_scan_type == ScanType::JS) {
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Libraries" / "LibJS");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Libraries" / "LibMarkdown");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Libraries" / "LibWeb");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Services" / "WebContent");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Services" / "WebWorker");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Applications" / "Assistant");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Applications" / "Browser");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Applications" / "Spreadsheet");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "Applications" / "TextEditor");
        scan_directory_for_relevant_files(paths, project_root / "Userland" / "DevTools" / "HackStudio");
    } else if (s_scan_type == ScanType::All) {
        scan_directory_for_relevant_files(paths, project_root / "AK");
        scan_directory_for_relevant_files(paths, project_root / "Kernel");
        scan_directory_for_relevant_files(paths, project_root / "Ladybird");
        scan_directory_for_relevant_files(paths, project_root / "Userland");
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
    return processor.run(s_num_threads);
}
