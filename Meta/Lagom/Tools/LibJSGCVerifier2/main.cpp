/*
 * Copyright (c) 2023, Leon Albrecht <leon.a@serenityos.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/DiagnosticFrontend.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Tooling/Tooling.h>
#include <unordered_set>

bool record_inherits_from_cell(clang::CXXRecordDecl const& record)
{
    if (!record.isCompleteDefinition())
        return false;

    bool inherits_from_cell = record.getQualifiedNameAsString() == "JS::Cell";
    record.forallBases([&](clang::CXXRecordDecl const* base) -> bool {
        if (base->getQualifiedNameAsString() == "JS::Cell") {
            inherits_from_cell = true;
            return false;
        }
        return true;
    });
    return inherits_from_cell;
}

std::vector<clang::QualType> get_all_qualified_types(clang::QualType const& type)
{
    std::vector<clang::QualType> qualified_types;

    if (auto const* template_specialization = type->getAs<clang::TemplateSpecializationType>()) {
        auto specialization_name = template_specialization->getTemplateName().getAsTemplateDecl()->getQualifiedNameAsString();
        // Do not unwrap GCPtr/NonnullGCPtr
        if (specialization_name == "JS::GCPtr" || specialization_name == "JS::NonnullGCPtr") {
            qualified_types.push_back(type);
        } else {
            for (size_t i = 0; i < template_specialization->template_arguments().size(); i++) {
                auto const& template_arg = template_specialization->template_arguments()[i];
                if (template_arg.getKind() == clang::TemplateArgument::Type) {
                    auto template_qualified_types = get_all_qualified_types(template_arg.getAsType());
                    std::move(template_qualified_types.begin(), template_qualified_types.end(), std::back_inserter(qualified_types));
                }
            }
        }
    } else {
        qualified_types.push_back(type);
    }

    return qualified_types;
}

struct FieldValidationResult {
    bool is_valid { false };
    bool is_wrapped_in_gcptr { false };
    bool needs_visiting { false };
};

FieldValidationResult validate_field(clang::FieldDecl const* field_decl)
{
    auto type = field_decl->getType();
    if (auto const* elaborated_type = llvm::dyn_cast<clang::ElaboratedType>(type.getTypePtr()))
        type = elaborated_type->desugar();

    FieldValidationResult result { .is_valid = true };

    for (auto const& qualified_type : get_all_qualified_types(type)) {
        if (auto const* pointer_decl = qualified_type->getAs<clang::PointerType>()) {
            if (auto const* pointee = pointer_decl->getPointeeCXXRecordDecl()) {
                if (record_inherits_from_cell(*pointee)) {
                    result.is_valid = false;
                    result.is_wrapped_in_gcptr = false;
                    result.needs_visiting = true;
                    return result;
                }
            }
        } else if (auto const* reference_decl = qualified_type->getAs<clang::ReferenceType>()) {
            if (auto const* pointee = reference_decl->getPointeeCXXRecordDecl()) {
                if (record_inherits_from_cell(*pointee)) {
                    result.is_valid = false;
                    result.is_wrapped_in_gcptr = false;
                    result.needs_visiting = true;
                    return result;
                }
            }
        } else if (auto const* specialization = qualified_type->getAs<clang::TemplateSpecializationType>()) {
            auto template_type_name = specialization->getTemplateName().getAsTemplateDecl()->getName();
            if (template_type_name != "GCPtr" && template_type_name != "NonnullGCPtr")
                return result;

            if (specialization->template_arguments().size() != 1)
                return result; // Not really valid, but will produce a compilation error anyway

            auto const& type_arg = specialization->template_arguments()[0];
            auto const* record_type = type_arg.getAsType()->getAs<clang::RecordType>();
            if (!record_type)
                return result;

            auto const* record_decl = record_type->getAsCXXRecordDecl();
            if (!record_decl->hasDefinition())
                return result;

            result.is_wrapped_in_gcptr = true;
            result.is_valid = record_inherits_from_cell(*record_decl);
            result.needs_visiting = true;
        }
    }

    return result;
}

class ProcessCellsVisitor : public clang::RecursiveASTVisitor<ProcessCellsVisitor> {
public:
    explicit ProcessCellsVisitor(clang::ASTContext* context)
        : m_context(context)
    {
    }

    bool VisitCXXRecordDecl(clang::CXXRecordDecl const* decl)
    {
        if (!decl->getDefinition())
            return true;
        if (decl->getNumBases() == 0)
            return true;
        if (!decl->isClass() && !decl->isStruct())
            return true;

        auto name = decl->getQualifiedNameAsString();
        if (m_records.contains(name))
            return true;

        // llvm::outs() << "Looking at " << name << "\n";
        bool is_cell = false;
        decl->forallBases([&](clang::CXXRecordDecl const* base) -> bool {
            auto base_name = base->getQualifiedNameAsString();
            if (base_name == "JS::Cell") {
                is_cell = true;
                return false;
            }

            if (auto it = m_records.find(base_name); it != m_records.end()) {
                if (it->second.is_cell) {
                    is_cell = true;
                    return false;
                }
            }

            return true;
        });

        m_records.insert({ name, RecordEntry {
            is_cell,
            decl,
        }});

        if (is_cell) {
            llvm::outs() << "Found JS::Cell inheritor: " << name << "\n";
        }

        auto& diag_engine = m_context->getDiagnostics();
        std::vector<clang::FieldDecl const*> fields_that_need_visiting;

        for (clang::FieldDecl const* field : decl->fields()) {
            auto validation_results = validate_field(field);
            if (!validation_results.is_valid) {
                if (validation_results.is_wrapped_in_gcptr) {
                    auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Specialization type must inherit from JS::Cell");
                    diag_engine.Report(field->getLocation(), diag_id);
                } else {
                    auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "%0 to JS::Cell type should be wrapped in %1");
                    auto builder = diag_engine.Report(field->getLocation(), diag_id);
                    if (field->getType()->isReferenceType()) {
                        builder << "reference"
                                << "JS::NonnullGCPtr";
                    } else {
                        builder << "pointer"
                                << "JS::GCPtr";
                    }
                }
            } else if (validation_results.needs_visiting) {
                fields_that_need_visiting.push_back(field);
            }
        }

        if (!is_cell && !fields_that_need_visiting.empty()) {
            clang::DeclarationName const name = &m_context->Idents.get("visit_edges");
            auto const* visit_edges_method = decl->lookup(name).find_first<clang::CXXMethodDecl>();
            bool has_visit_edges = !!visit_edges_method;

            auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Class or struct that has cell fields must inherit from cell and visit them (%0)");
            diag_engine.Report(decl->getLocation(), diag_id) << has_visit_edges;
            return true;
        } 

        return true;
    }

private:
    struct RecordEntry {
        bool is_cell;
        clang::CXXRecordDecl const* record;
    };

    clang::ASTContext* m_context;
    std::unordered_map<std::string, RecordEntry> m_records;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
    explicit FindNamedClassConsumer(clang::ASTContext* context)
        : m_visitor(context)
    {
    }

    virtual void HandleTranslationUnit(clang::ASTContext& context)
    {
        m_visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    ProcessCellsVisitor m_visitor;
};

class LibJSGCVerifierPlugin : public clang::PluginASTAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef in_file) override
    {
        (void)in_file;
        return std::make_unique<FindNamedClassConsumer>(&compiler.getASTContext());
    }

    virtual bool ParseArgs(clang::CompilerInstance const&, std::vector<std::string> const&) override
    {
        return true;
    };

    PluginASTAction::ActionType getActionType() override
    {
        return AddBeforeMainAction;
    }
};

static clang::FrontendPluginRegistry::Add<LibJSGCVerifierPlugin> X(
    "LibJSGCVerifier", "Verifies various aspects of the LibJS garbage collector");
