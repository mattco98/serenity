/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "DiagConsumer.h"
#include <clang/AST/ASTConsumer.h>

class DiagDelegatingASTConsumer : public clang::ASTConsumer {
public:
    explicit DiagDelegatingASTConsumer(std::unique_ptr<clang::ASTConsumer> delegate)
        : m_delegate(std::move(delegate))
    {
    }

    virtual ~DiagDelegatingASTConsumer() override = default;

    virtual void Initialize(clang::ASTContext& context) override
    {
        m_delegate->Initialize(context);
        DiagConsumer::install(context.getDiagnostics());
    }

    virtual bool HandleTopLevelDecl(clang::DeclGroupRef decl) override { return m_delegate->HandleTopLevelDecl(decl); }
    virtual void HandleInlineFunctionDefinition(clang::FunctionDecl* decl) override { m_delegate->HandleInlineFunctionDefinition(decl); }
    virtual void HandleInterestingDecl(clang::DeclGroupRef decl) override { m_delegate->HandleInterestingDecl(decl); }
    virtual void HandleTranslationUnit(clang::ASTContext& context) override { m_delegate->HandleTranslationUnit(context); }
    virtual void HandleTagDeclDefinition(clang::TagDecl* decl) override { m_delegate->HandleTagDeclDefinition(decl); }
    virtual void HandleTagDeclRequiredDefinition(clang::TagDecl const* decl) override { m_delegate->HandleTagDeclRequiredDefinition(decl); }
    virtual void HandleCXXImplicitFunctionInstantiation(clang::FunctionDecl* decl) override { m_delegate->HandleCXXImplicitFunctionInstantiation(decl); }
    virtual void HandleTopLevelDeclInObjCContainer(clang::DeclGroupRef decl) override { m_delegate->HandleTopLevelDeclInObjCContainer(decl); }
    virtual void HandleImplicitImportDecl(clang::ImportDecl* decl) override { m_delegate->HandleImplicitImportDecl(decl); }
    virtual void CompleteTentativeDefinition(clang::VarDecl* decl) override { m_delegate->CompleteTentativeDefinition(decl); }
    virtual void CompleteExternalDeclaration(clang::VarDecl* decl) override { m_delegate->CompleteExternalDeclaration(decl); }
    virtual void AssignInheritanceModel(clang::CXXRecordDecl* decl) override { m_delegate->AssignInheritanceModel(decl); }
    virtual void HandleCXXStaticMemberVarInstantiation(clang::VarDecl* decl) override { m_delegate->HandleCXXStaticMemberVarInstantiation(decl); }
    virtual void HandleVTable(clang::CXXRecordDecl* decl) override { m_delegate->HandleVTable(decl); }
    virtual clang::ASTMutationListener* GetASTMutationListener() override { return m_delegate->GetASTMutationListener(); }
    virtual clang::ASTDeserializationListener* GetASTDeserializationListener() override { return m_delegate->GetASTDeserializationListener(); }
    virtual void PrintStats() override { m_delegate->PrintStats(); }
    virtual bool shouldSkipFunctionBody(clang::Decl* decl) override { return m_delegate->shouldSkipFunctionBody(decl); }

private:
    std::unique_ptr<clang::ASTConsumer> m_delegate;
};
