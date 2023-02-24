#include "CollectCellsHandler.h"
#include <filesystem>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

CollectCellsHandler::CollectCellsHandler()
{
    using namespace clang::ast_matchers;

    m_finder.addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            cxxRecordDecl(
                decl().bind("record-decl"),
                isDerivedFrom(hasName("::JS::Cell"))
            )
        ),
        this
    );
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

    return true;
}

static bool record_inherits_from_cell(clang::CXXRecordDecl const& record)
{
    if (!record.isCompleteDefinition())
        return false;

    bool inherits_from_cell = false;
    record.forallBases([&](clang::CXXRecordDecl const* base) -> bool {
        if (base->getQualifiedNameAsString() == "JS::Cell") {
            inherits_from_cell = true;
            return false;
        }
        return true;
    });
    return inherits_from_cell;
}

static bool validate_gcptr_field(clang::QualType type)
{
    if (auto const* elaborated_type = llvm::dyn_cast<clang::ElaboratedType>(type.getTypePtr()))
        type = elaborated_type->desugar();

    auto const* specialization = type->getAs<clang::TemplateSpecializationType>();
    if (!specialization)
        return true;

    auto template_type_name = specialization->getTemplateName().getAsTemplateDecl()->getName();
    if (template_type_name != "GCPtr" && template_type_name != "NonnullGCPtr")
        return true;

    if (specialization->getNumArgs() != 1)
        return true;

    auto const& type_arg = specialization->getArg(0);
    auto const* record_type = type_arg.getAsType()->getAs<clang::RecordType>();
    if (!record_type)
        return true;

    auto const* record_decl = record_type->getAsCXXRecordDecl();
    if (!record_decl->hasDefinition())
        return true;

    return record_inherits_from_cell(*record_decl);
}

static std::vector<clang::QualType> get_all_qualified_types(clang::QualType const& type)
{
    std::vector<clang::QualType> qualified_types;

    if (auto const* template_specialization = type->getAs<clang::TemplateSpecializationType>()) {
        for (size_t i = 0; i < template_specialization->getNumArgs(); i++) {
            auto const& template_arg = template_specialization->getArg(i);
            if (template_arg.getKind() == clang::TemplateArgument::Type) {
                auto template_qualified_types = get_all_qualified_types(template_arg.getAsType());
                std::move(template_qualified_types.begin(), template_qualified_types.end(), std::back_inserter(qualified_types));
            }
        }
    } else {
        qualified_types.push_back(type);
    }

    return qualified_types;
}

static bool validate_non_gcptr_field(clang::QualType const& type)
{
    for (auto const& qualified_type : get_all_qualified_types(type)) {
        if (auto const* pointer_decl = qualified_type->getAs<clang::PointerType>()) {
            if (auto const* pointee = pointer_decl->getPointeeCXXRecordDecl(); pointee && record_inherits_from_cell(*pointee))
                return false;
        } else if (auto const* reference_decl = qualified_type->getAs<clang::ReferenceType>()) {
            if (auto const* pointee = reference_decl->getPointeeCXXRecordDecl(); pointee && record_inherits_from_cell(*pointee))
                return false;
        }
    }

    return true;
}

void CollectCellsHandler::run(clang::ast_matchers::MatchFinder::MatchResult const& result)
{
    clang::CXXRecordDecl const* record = result.Nodes.getNodeAs<clang::CXXRecordDecl>("record-decl");
    if (!record || !record->isCompleteDefinition() || (!record->isClass() && !record->isStruct()))
        return;

    auto const class_name = record->getQualifiedNameAsString();
    if (m_visited_classes.contains(class_name))
        return;

    m_visited_classes.insert(class_name);

    for (auto const& field : record->fields()) {
        auto const& type = field->getType();

        if (!validate_gcptr_field(type)) {
            llvm::errs() << "    Invalid specialization of (Nonnull)GCPtr:\n";
            llvm::errs() << "        " << field->getType().getAsString() << " \033[34m" << class_name << "\033[0m::\033[33m" << field->getName() << "\033[0m\n";
        }

        if (!validate_non_gcptr_field(type)) {
            llvm::errs() << "    Type that should be wrapped in (Nonnull)GCPtr:\n";
            llvm::errs() << "        " << field->getType().getAsString() << " \033[34m" << class_name << "\033[0m::\033[33m" << field->getName() << "\033[0m\n";
        }
    }
}
