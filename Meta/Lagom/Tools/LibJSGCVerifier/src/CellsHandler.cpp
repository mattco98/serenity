/*
 * Copyright (c) 2023, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "CellsHandler.h"
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <filesystem>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_set>
#include <vector>

CollectCellsHandler::CollectCellsHandler()
{
    using namespace clang::ast_matchers;

    m_finder.addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            cxxRecordDecl(decl().bind("record-decl"))),
        this);
}

auto hasAnyName(std::vector<std::string> names)
{
    return clang::ast_matchers::internal::Matcher<clang::NamedDecl>(
        new clang::ast_matchers::internal::HasNameMatcher(names));
}

bool CollectCellsHandler::handleBeginSource(clang::CompilerInstance& ci)
{
    auto const& source_manager = ci.getSourceManager();
    ci.getFileManager().getNumUniqueRealFiles();
    auto file_id = source_manager.getMainFileID();
    auto const* file_entry = source_manager.getFileEntryForID(file_id);
    if (!file_entry)
        return false;

    auto current_filepath = std::filesystem::canonical(file_entry->getName().str());
    llvm::outs() << "Processing " << current_filepath.string() << "\n";
    m_file = current_filepath.string();

    return true;
}

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
            for (size_t i = 0; i < template_specialization->getNumArgs(); i++) {
                auto const& template_arg = template_specialization->getArg(i);
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
    bool should_be_visited { false };
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
                    result.should_be_visited = true;
                    return result;
                }
            }
        } else if (auto const* reference_decl = qualified_type->getAs<clang::ReferenceType>()) {
            if (auto const* pointee = reference_decl->getPointeeCXXRecordDecl()) {
                if (record_inherits_from_cell(*pointee)) {
                    result.is_valid = false;
                    result.is_wrapped_in_gcptr = false;
                    result.should_be_visited = true;
                    return result;
                }
            }
        } else if (auto const* specialization = qualified_type->getAs<clang::TemplateSpecializationType>()) {
            auto template_type_name = specialization->getTemplateName().getAsTemplateDecl()->getName();
            if (template_type_name != "GCPtr" && template_type_name != "NonnullGCPtr")
                return result;

            if (specialization->getNumArgs() != 1)
                return result; // Not really valid, but will produce a compilation error anyway

            auto const& type_arg = specialization->getArg(0);
            auto const* record_type = type_arg.getAsType()->getAs<clang::RecordType>();
            if (!record_type)
                return result;

            auto const* record_decl = record_type->getAsCXXRecordDecl();
            if (!record_decl->hasDefinition())
                return result;

            result.is_wrapped_in_gcptr = true;
            result.should_be_visited = true;
            result.is_valid = record_inherits_from_cell(*record_decl);
        }
    }

    return result;
}

void CollectCellsHandler::run(clang::ast_matchers::MatchFinder::MatchResult const& result)
{
    using namespace clang::ast_matchers;

    clang::CXXRecordDecl const* record = result.Nodes.getNodeAs<clang::CXXRecordDecl>("record-decl");
    if (!record || !record->isCompleteDefinition() || (!record->isClass() && !record->isStruct()))
        return;

    auto& diag_engine = result.Context->getDiagnostics();
    std::unordered_set<std::string> fields_that_need_visiting;

    for (auto const* field : record->fields()) {
        auto validation_results = validate_field(field);
        if (!validation_results.is_valid) {
            if (validation_results.is_wrapped_in_gcptr) {
                auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "Specialization type must inherit from JS::Cell");
                diag_engine.Report(field->getLocation(), diag_id);
            } else {
                auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "%0 to JS::Cell type should be wrapped in %1");
                auto builder = diag_engine.Report(field->getLocation(), diag_id);
                if (field->getType()->isReferenceType()) {
                    builder << "reference"
                            << "JS::NonnullGCPtr";
                } else {
                    builder << "pointer"
                            << "JS::GCPtr";
                }
            }
        } else if (validation_results.should_be_visited) {
            fields_that_need_visiting.insert(field->getNameAsString());
        }
    }

    static std::unordered_set<std::string_view> const cells_without_visit_edges_whitelist = {
        "FreelistEntry", "WeakSet",
    };

    if (fields_that_need_visiting.empty() || !record_inherits_from_cell(*record) || cells_without_visit_edges_whitelist.contains(record->getNameAsString()))
        return;

    clang::CXXMethodDecl const* visit_method = nullptr;
    for (auto const* record_decl : record->decls()) {
        auto const *method = clang::dyn_cast<clang::CXXMethodDecl>(record_decl);
        if (method && method->getNameAsString() == "visit_edges" && !method->isImplicit())
            visit_method = method;
    }

    if (!visit_method) {
        auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "Class with JS::Cell members does not override visit_impl");
        diag_engine.Report(record->getLocation(), diag_id);
        return;
    }

    auto const* definition = visit_method->getDefinition();
    if (!definition || !definition->getBody())
        return;

    auto& ast_context = definition->getASTContext();
    auto& source_manager = ast_context.getSourceManager();
    auto record_name = record->getQualifiedNameAsString();

    // Check for call to Base::visit_edges()
    auto base_visit_edges_matcher = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("visit_edges")))).bind("member-expr");
    bool found_base_call = false;

    for (auto const& child : definition->getBody()->children()) {
        if (auto const* member_call = clang::dyn_cast<clang::CXXMemberCallExpr>(child)) {
            // FIXME: Ideally this would not rely directly on the source code, however it seems to be
            //        the most reliable solution.
            bool invalid = false;
            const char* source_code_ptr = ast_context.getSourceManager().getCharacterData(member_call->getBeginLoc(), &invalid);
            if (invalid)
                continue;

            auto begin = member_call->getBeginLoc();
            auto end = member_call->getEndLoc();
            auto length = source_manager.getFileOffset(end) - source_manager.getFileOffset(begin);
            std::string source_code(source_code_ptr, length);
            if (source_code.starts_with("Base::visit_edges(")) {
                found_base_call = true;
                break;
            }
        }
    }

    if (!found_base_call) {
        auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "%0::visit_edges has no call to Base::visit_edges");
        auto builder = diag_engine.Report(definition->getLocation(), diag_id);
        builder << record_name;
    }

    // Check for a read of each field that needs visiting. We just check for any read
    // to account for complex fields such as "Vector<GCPtr<Foo>>", assuming that a read
    // in visit_edges will only ever happen if the field is getting visited.
    auto names_vector = std::vector(fields_that_need_visiting.begin(), fields_that_need_visiting.end());
    auto matcher = memberExpr(
        isExpansionInMainFile(),
        member(::hasAnyName(names_vector)),
        hasAncestor(functionDecl(hasName("visit_edges")))
    ).bind("member-expr");

    for (auto const& bound_expr : match(matcher, ast_context)) {
        if (auto const* member_expr = bound_expr.getNodeAs<clang::MemberExpr>("member-expr"))
            fields_that_need_visiting.erase(member_expr->getMemberDecl()->getNameAsString());
    }

    for (auto const& name : fields_that_need_visiting) {
        auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "GC field %0 is not visited in visit_edges");
        auto builder = diag_engine.Report(definition->getLocation(), diag_id);
        builder << name;
    }
}
