/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendAction.h>

struct LibJSCellMacro {
    enum class Type {
        JSCell,
        JSObject,
        JSEnvironment,
        JSPrototypeObject,
        WebPlatformObject,
    };

    struct Arg {
        std::string text;
        clang::SourceLocation location;
    };

    clang::SourceRange range;
    Type type;
    std::vector<Arg> args;

    static char const* type_name(Type);
};

using LibJSCellMacroMap = std::unordered_map<unsigned int, std::vector<LibJSCellMacro>>;

class LibJSGCPluginAction : public clang::PluginASTAction {
public:
    virtual bool ParseArgs(clang::CompilerInstance const&, std::vector<std::string> const& args) override
    {
        m_detect_invalid_function_members = std::find(args.begin(), args.end(), "detect-invalid-function-members") != args.end();
        return true;
    }

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef) override;

    ActionType getActionType() override
    {
        return AddAfterMainAction;
    }

private:
    bool m_detect_invalid_function_members { false };
};
