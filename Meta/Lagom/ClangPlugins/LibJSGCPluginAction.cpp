/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LibJSGCPluginAction.h"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/AST/RecordLayout.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Lex/MacroArgs.h>
#include <unordered_set>

class Consumer
    : public clang::ASTConsumer
    , public clang::ast_matchers::internal::CollectMatchesCallback {
public:
    Consumer()
    {
        using namespace clang::ast_matchers;

        m_finder.addMatcher(
            traverse(
                clang::TK_IgnoreUnlessSpelledInSource,
                cxxRecordDecl()),
            this);
    }

private:
    struct CellMacroExpectation {
        LibJSCellMacro::Type type;
        std::string base_name;
    };

    clang::ast_matchers::MatchFinder m_finder;
};

std::unique_ptr<clang::ASTConsumer> LibJSGCPluginAction::CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef)
{
    // printf("before\n");
    // auto size = compiler.getTarget().getPointerWidth(clang::LangAS::Default);
    // printf("after, size: %zu\n", size);
    return std::make_unique<Consumer>();
}

static clang::FrontendPluginRegistry::Add<LibJSGCPluginAction> X("libjs_gc_scanner", "analyze LibJS GC usage");
