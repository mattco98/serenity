/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Tooling/Tooling.h>
#include <unordered_set>

class LambdaCapturePluginAction
    : public clang::PluginASTAction
    , public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    static constexpr char const* action_name() { return "LambdaCapturePluginAction"; }

    LambdaCapturePluginAction();

    virtual bool ParseArgs(clang::CompilerInstance const&, std::vector<std::string> const&) override
    {
        return true;
    }

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) override
    {
        return m_finder.newASTConsumer();
    }

private:
    virtual void run(clang::ast_matchers::MatchFinder::MatchResult const& result) override;

    clang::ast_matchers::MatchFinder m_finder;
};
