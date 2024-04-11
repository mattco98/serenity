/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DiagConsumer.h"
#include <AK/HashFunctions.h>

static thread_local std::unordered_set<clang::DiagnosticsEngine*> s_installed_engines;

static bool s_did_emit_diagnostic;
static VisitedObjectSet<unsigned int> s_visited_locations;

std::mutex print_mutex;

std::lock_guard<std::mutex> get_print_lock()
{
    return std::lock_guard(print_mutex);
}

bool DiagConsumer::did_emit_diagnostic()
{
    return s_did_emit_diagnostic;
}

void DiagConsumer::install(clang::DiagnosticsEngine& engine)
{
    if (!s_installed_engines.contains(&engine)) {
        s_installed_engines.insert(&engine);
        engine.setClient(new DiagConsumer { engine.takeClient() }, true);
    }
}

void DiagConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level, clang::Diagnostic const& info)
{
    // We don't visit the base method here since all it really does it cause the "X warnings generated." message
    // to show up, which would just be another line to put in every test case. If we're not in test mode, we
    // delegate to the base diagnostic consumer, which will invoke the base method itself.

    auto& source_manager = info.getSourceManager();
    auto const& location = info.getLocation();
    auto path = std::filesystem::path { source_manager.getFilename(location).str() }.filename().string();
    auto file_id = source_manager.getFileID(location);
    auto file_offset = source_manager.getFileOffset(location);
    auto line = source_manager.getLineNumber(file_id, file_offset);
    auto col = source_manager.getColumnNumber(file_id, file_offset);

    // We need to hash it ourselves, since for some reason location.getHashValue() changes for identical diagnostics
    auto path_hash = std::hash<std::string> {}(path);
    auto line_hash = std::hash<unsigned int> {}(line);
    auto col_hash = std::hash<unsigned int> {}(col);
    auto hash = pair_int_hash(u64_hash(path_hash), pair_int_hash(line_hash, col_hash));

    if (s_visited_locations.has_visited(hash))
        return;

    auto& diag_engine = source_manager.getDiagnostics();

    // FIXME: Only set this to true if this is a diagnostic emitted by this tool, so e.g. failing to find a random
    //        header file won't cause the program to return an error code.
    s_did_emit_diagnostic = true;

    if (!s_test_mode) {
        m_base_consumer->HandleDiagnostic(level, info);
        return;
    }

    // Can't use location.print() as it will output the entire absolute file path, which is of course dependent
    // on the user's machine. Instead, we only print the path, and also print in a more compact format.
    llvm::errs() << "[" << path << ":" << line << ":" << col << "] ";

    switch (level) {
    case clang::DiagnosticsEngine::Fatal:
        llvm::errs() << "fatal: ";
        break;
    case clang::DiagnosticsEngine::Error:
        llvm::errs() << "error: ";
        break;
    case clang::DiagnosticsEngine::Warning:
        llvm::errs() << "warning: ";
        break;
    case clang::DiagnosticsEngine::Note:
        llvm::errs() << "note: ";
        break;
    case clang::DiagnosticsEngine::Remark:
        llvm::errs() << "remark: ";
        break;
    case clang::DiagnosticsEngine::Ignored:
        llvm::errs() << "ignored: ";
        break;
    }

    llvm::SmallVector<char> message;
    info.FormatDiagnostic(message);
    llvm::errs() << message << "\n";

    diag_engine.Clear();
}
