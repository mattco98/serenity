/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "VisitedObjectSet.h"
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>

extern llvm::cl::opt<bool> s_test_mode;

std::lock_guard<std::mutex> get_print_lock();

// This consumer emits diagnostics in an easy-to-compare manner when running in test mode,
// as clang diagnostics include the full file path, which of course will differ depending
// on the machine. When outside of test mode, diagnostics are delegated to Clang's default
// diagnostic consumer.
//
// Regardless of the mode, diagnostics resulting from the same location are never re-emitted
// on a later execution of the tool.
class DiagConsumer : public clang::DiagnosticConsumer {
public:
    static bool did_emit_diagnostic();
    static void install(clang::DiagnosticsEngine& engine);

private:
    explicit DiagConsumer(std::unique_ptr<clang::DiagnosticConsumer> base_consumer)
        : m_base_consumer(std::move(base_consumer))
    {
    }

    virtual void HandleDiagnostic(clang::DiagnosticsEngine::Level, clang::Diagnostic const&) override;

    std::unique_ptr<clang::DiagnosticConsumer> m_base_consumer;
};
