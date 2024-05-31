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

// template<typename T>
// class SimpleCollectMatchesCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
// public:
//     explicit SimpleCollectMatchesCallback(std::string name)
//         : m_name(std::move(name))
//     {
//     }
//
//     void run(clang::ast_matchers::MatchFinder::MatchResult const& result) override
//     {
//         if (auto const* node = result.Nodes.getNodeAs<T>(m_name))
//             m_matches.push_back(node);
//     }
//
//     auto const& matches() const { return m_matches; }
//
// private:
//     std::string m_name;
//     std::vector<T const*> m_matches;
// };
//
// bool record_inherits_from_cell(clang::CXXRecordDecl const& record)
// {
//     if (!record.isCompleteDefinition())
//         return false;
//
//     bool inherits_from_cell = record.getQualifiedNameAsString() == "JS::Cell";
//     record.forallBases([&](clang::CXXRecordDecl const* base) -> bool {
//         if (base->getQualifiedNameAsString() == "JS::Cell") {
//             inherits_from_cell = true;
//             return false;
//         }
//         return true;
//     });
//     return inherits_from_cell;
// }
//
// std::vector<clang::QualType> get_all_qualified_types(clang::QualType const& type)
// {
//     std::vector<clang::QualType> qualified_types;
//
//     if (auto const* template_specialization = type->getAs<clang::TemplateSpecializationType>()) {
//         auto specialization_name = template_specialization->getTemplateName().getAsTemplateDecl()->getQualifiedNameAsString();
//         // Do not unwrap GCPtr/NonnullGCPtr/MarkedVector
//         static std::unordered_set<std::string> gc_relevant_type_names {
//             "JS::GCPtr",
//             "JS::NonnullGCPtr",
//             "JS::RawGCPtr",
//             "JS::MarkedVector",
//             "JS::Handle",
//             "JS::SafeFunction",
//         };
//
//         if (gc_relevant_type_names.contains(specialization_name)) {
//             qualified_types.push_back(type);
//         } else {
//             auto const template_arguments = template_specialization->template_arguments();
//             for (size_t i = 0; i < template_arguments.size(); i++) {
//                 auto const& template_arg = template_arguments[i];
//                 if (template_arg.getKind() == clang::TemplateArgument::Type) {
//                     auto template_qualified_types = get_all_qualified_types(template_arg.getAsType());
//                     std::move(template_qualified_types.begin(), template_qualified_types.end(), std::back_inserter(qualified_types));
//                 }
//             }
//         }
//     } else {
//         qualified_types.push_back(type);
//     }
//
//     return qualified_types;
// }
//
// enum class OuterType {
//     GCPtr,
//     RawGCPtr,
//     Handle,
//     SafeFunction,
//     Ptr,
//     Ref,
// };
//
// struct QualTypeGCInfo {
//     std::optional<OuterType> outer_type { {} };
//     bool base_type_inherits_from_cell { false };
// };
//
// std::optional<QualTypeGCInfo> validate_qualified_type(clang::QualType const& type)
// {
//     if (auto const* pointer_decl = type->getAs<clang::PointerType>()) {
//         if (auto const* pointee = pointer_decl->getPointeeCXXRecordDecl())
//             return QualTypeGCInfo { OuterType::Ptr, record_inherits_from_cell(*pointee) };
//     } else if (auto const* reference_decl = type->getAs<clang::ReferenceType>()) {
//         if (auto const* pointee = reference_decl->getPointeeCXXRecordDecl())
//             return QualTypeGCInfo { OuterType::Ref, record_inherits_from_cell(*pointee) };
//     } else if (auto const* specialization = type->getAs<clang::TemplateSpecializationType>()) {
//         auto template_type_name = specialization->getTemplateName().getAsTemplateDecl()->getQualifiedNameAsString();
//
//         OuterType outer_type;
//         if (template_type_name == "JS::GCPtr" || template_type_name == "JS::NonnullGCPtr") {
//             outer_type = OuterType::GCPtr;
//         } else if (template_type_name == "JS::RawGCPtr") {
//             outer_type = OuterType::RawGCPtr;
//         } else if (template_type_name == "JS::Handle") {
//             outer_type = OuterType::Handle;
//         } else if (template_type_name == "JS::SafeFunction") {
//             return QualTypeGCInfo { OuterType::SafeFunction, false };
//         } else {
//             return {};
//         }
//
//         auto template_args = specialization->template_arguments();
//         if (template_args.size() != 1)
//             return {}; // Not really valid, but will produce a compilation error anyway
//
//         auto const& type_arg = template_args[0];
//         auto const* record_type = type_arg.getAsType()->getAs<clang::RecordType>();
//         if (!record_type)
//             return {};
//
//         auto const* record_decl = record_type->getAsCXXRecordDecl();
//         if (!record_decl->hasDefinition())
//             return {};
//
//         return QualTypeGCInfo { outer_type, record_inherits_from_cell(*record_decl) };
//     }
//
//     return {};
// }
//
// std::optional<QualTypeGCInfo> validate_field_qualified_type(clang::FieldDecl const* field_decl)
// {
//     auto type = field_decl->getType();
//     if (auto const* elaborated_type = llvm::dyn_cast<clang::ElaboratedType>(type.getTypePtr()))
//         type = elaborated_type->desugar();
//
//     for (auto const& qualified_type : get_all_qualified_types(type)) {
//         if (auto error = validate_qualified_type(qualified_type))
//             return error;
//     }
//
//     return {};
// }
//
// struct CellTypeWithOrigin {
//     clang::CXXRecordDecl const& base_origin;
//     LibJSCellMacro::Type type;
// };
//
// std::optional<CellTypeWithOrigin> find_cell_type_with_origin(clang::CXXRecordDecl const& record)
// {
//     for (auto const& base : record.bases()) {
//         if (auto const* base_record = base.getType()->getAsCXXRecordDecl()) {
//             auto base_name = base_record->getQualifiedNameAsString();
//
//             if (base_name == "JS::Cell")
//                 return CellTypeWithOrigin { *base_record, LibJSCellMacro::Type::JSCell };
//
//             if (base_name == "JS::Object")
//                 return CellTypeWithOrigin { *base_record, LibJSCellMacro::Type::JSObject };
//
//             if (base_name == "JS::Environment")
//                 return CellTypeWithOrigin { *base_record, LibJSCellMacro::Type::JSEnvironment };
//
//             if (base_name == "JS::PrototypeObject")
//                 return CellTypeWithOrigin { *base_record, LibJSCellMacro::Type::JSPrototypeObject };
//
//             if (base_name == "Web::Bindings::PlatformObject")
//                 return CellTypeWithOrigin { *base_record, LibJSCellMacro::Type::WebPlatformObject };
//
//             if (auto origin = find_cell_type_with_origin(*base_record))
//                 return CellTypeWithOrigin { *base_record, origin->type };
//         }
//     }
//
//     return {};
// }
//
// char const* LibJSCellMacro::type_name(Type type)
// {
//     switch (type) {
//     case Type::JSCell:
//         return "JS_CELL";
//     case Type::JSObject:
//         return "JS_OBJECT";
//     case Type::JSEnvironment:
//         return "JS_ENVIRONMENT";
//     case Type::JSPrototypeObject:
//         return "JS_PROTOTYPE_OBJECT";
//     case Type::WebPlatformObject:
//         return "WEB_PLATFORM_OBJECT";
//     default:
//         __builtin_unreachable();
//     }
// }

class LibJSPPCallbacks : public clang::PPCallbacks {
public:
    explicit LibJSPPCallbacks(clang::Preprocessor& preprocessor)
        : m_preprocessor(preprocessor)
    {
    }

    auto const& macro_map() const { return m_macro_map; }

    virtual void LexedFileChanged(clang::FileID curr_fid, LexedFileChangeReason reason, clang::SrcMgr::CharacteristicKind, clang::FileID, clang::SourceLocation) override
    {
        (void)curr_fid;
        (void)reason;
        // if (reason == LexedFileChangeReason::EnterFile) {
        //     m_curr_fid_hash_stack.push_back(curr_fid.getHashValue());
        // } else {
        //     assert(!m_curr_fid_hash_stack.empty());
        //     m_curr_fid_hash_stack.pop_back();
        // }
    }

    virtual void MacroExpands(clang::Token const& name_token, clang::MacroDefinition const&, clang::SourceRange range, clang::MacroArgs const* args) override
    {
        (void)name_token;
        (void)range;
        (void)args;
        // if (auto const* ident_info = name_token.getIdentifierInfo()) {
        //     static llvm::StringMap<LibJSCellMacro::Type> libjs_macro_types {
        //         { "JS_CELL", LibJSCellMacro::Type::JSCell },
        //         { "JS_OBJECT", LibJSCellMacro::Type::JSObject },
        //         { "JS_ENVIRONMENT", LibJSCellMacro::Type::JSEnvironment },
        //         { "JS_PROTOTYPE_OBJECT", LibJSCellMacro::Type::JSPrototypeObject },
        //         { "WEB_PLATFORM_OBJECT", LibJSCellMacro::Type::WebPlatformObject },
        //     };
        //
        //     auto name = ident_info->getName();
        //     if (auto it = libjs_macro_types.find(name); it != libjs_macro_types.end()) {
        //         LibJSCellMacro macro { range, it->second, {} };
        //
        //         for (size_t arg_index = 0; arg_index < args->getNumMacroArguments(); arg_index++) {
        //             auto const* first_token = args->getUnexpArgument(arg_index);
        //             auto stringified_token = clang::MacroArgs::StringifyArgument(first_token, m_preprocessor, false, range.getBegin(), range.getEnd());
        //             // The token includes leading and trailing quotes
        //             auto len = strlen(stringified_token.getLiteralData());
        //             std::string arg_text { stringified_token.getLiteralData() + 1, len - 2 };
        //             macro.args.push_back({ arg_text, first_token->getLocation() });
        //         }
        //
        //         assert(!m_curr_fid_hash_stack.empty());
        //         auto curr_fid_hash = m_curr_fid_hash_stack.back();
        //         if (m_macro_map.contains(curr_fid_hash))
        //             m_macro_map[curr_fid_hash] = {};
        //         m_macro_map[curr_fid_hash].push_back(macro);
        //     }
        // }
    }

private:
    clang::Preprocessor& m_preprocessor;
    std::vector<unsigned int> m_curr_fid_hash_stack;
    LibJSCellMacroMap m_macro_map;
};

class Consumer
    : public clang::ASTConsumer
    , public clang::ast_matchers::internal::CollectMatchesCallback {
public:
    Consumer(LibJSCellMacroMap const& macro_map, bool detect_invalid_function_members, size_t pointer_width)
        : m_macro_map(macro_map)
        , m_detect_invalid_function_members(detect_invalid_function_members)
        , m_pointer_width(pointer_width)
    {
        using namespace clang::ast_matchers;

        // m_finder.addMatcher(
        //     traverse(
        //         clang::TK_IgnoreUnlessSpelledInSource,
        //         cxxRecordDecl(
        //             anyOf(isClass(), isStruct()),
        //             unless(hasName("JS::Cell"))).bind("record-to-validate")),
        //     this);
    }

    // void HandleTranslationUnit(clang::ASTContext& Ctx) override
    // {
    //     m_finder.matchAST(Ctx);
    // }

    void run(clang::ast_matchers::MatchFinder::MatchResult const&) override
    {
        // if (auto const* record = result.Nodes.getNodeAs<clang::CXXRecordDecl>("record-to-validate"))
        //     validate_cxx_record_decl(result.Context, record);
    }

private:
    struct CellMacroExpectation {
        LibJSCellMacro::Type type;
        std::string base_name;
    };

    // void validate_cxx_record_decl(clang::ASTContext* context, clang::CXXRecordDecl const* record)
    // {
    //     using namespace clang::ast_matchers;
    //
    //     size_t total_field_size_in_bits = 0;
    //     auto const& layout = context->getASTRecordLayout(record);
    //     for (auto const* field : record->fields())
    //         total_field_size_in_bits += context->getTypeSize(field->getType());
    //     auto record_size_in_bits = context->toBits(layout.getSize());
    //     printf("record size in bits: %zu, field size in bits: %zu, pointer width: %zu\n", record_size_in_bits, total_field_size_in_bits, m_pointer_width);
    //     if (record_size_in_bits - total_field_size_in_bits > m_pointer_width) {
    //
    //     }
    //
    //     auto& diag_engine = context->getDiagnostics();
    //     std::vector<clang::FieldDecl const*> fields_that_need_visiting;
    //     auto record_is_cell = record_inherits_from_cell(*record);
    //
    //     for (clang::FieldDecl const* field : record->fields()) {
    //         auto validation_results = validate_field_qualified_type(field);
    //         if (!validation_results)
    //             continue;
    //
    //         auto [outer_type, base_type_inherits_from_cell] = *validation_results;
    //
    //         if (outer_type == OuterType::Ptr || outer_type == OuterType::Ref) {
    //             if (base_type_inherits_from_cell) {
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "%0 to JS::Cell type should be wrapped in %1");
    //                 auto builder = diag_engine.Report(field->getLocation(), diag_id);
    //                 if (outer_type == OuterType::Ref) {
    //                     builder << "reference"
    //                             << "JS::NonnullGCPtr";
    //                 } else {
    //                     builder << "pointer"
    //                             << "JS::GCPtr";
    //                 }
    //             }
    //         } else if (outer_type == OuterType::GCPtr || outer_type == OuterType::RawGCPtr) {
    //             if (!base_type_inherits_from_cell) {
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Specialization type must inherit from JS::Cell");
    //                 diag_engine.Report(field->getLocation(), diag_id);
    //             } else if (outer_type == OuterType::GCPtr) {
    //                 fields_that_need_visiting.push_back(field);
    //             }
    //         } else if (outer_type == OuterType::Handle || outer_type == OuterType::SafeFunction) {
    //             if (record_is_cell && m_detect_invalid_function_members) {
    //                 // FIXME: Change this to an Error when all of the use cases get addressed and remove the plugin argument
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "Types inheriting from JS::Cell should not have %0 fields");
    //                 auto builder = diag_engine.Report(field->getLocation(), diag_id);
    //                 builder << (outer_type == OuterType::Handle ? "JS::Handle" : "JS::SafeFunction");
    //             }
    //         }
    //     }
    //
    //     if (!record_is_cell)
    //         return;
    //
    //     validate_record_macros(context, *record);
    //
    //     clang::DeclarationName name = &context->Idents.get("visit_edges");
    //     auto const* visit_edges_method = record->lookup(name).find_first<clang::CXXMethodDecl>();
    //     if (!visit_edges_method && !fields_that_need_visiting.empty()) {
    //         auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "JS::Cell-inheriting class %0 contains a GC-allocated member %1 but has no visit_edges method");
    //         auto builder = diag_engine.Report(record->getLocation(), diag_id);
    //         builder << record->getName()
    //                 << fields_that_need_visiting[0];
    //     }
    //     if (!visit_edges_method || !visit_edges_method->getBody())
    //         return;
    //
    //     // Search for a call to Base::visit_edges. Note that this also has the nice side effect of
    //     // ensuring the classes use JS_CELL/JS_OBJECT, as Base will not be defined if they do not.
    //
    //     MatchFinder base_visit_edges_finder;
    //     SimpleCollectMatchesCallback<clang::MemberExpr> base_visit_edges_callback("member-call");
    //
    //     auto base_visit_edges_matcher = cxxMethodDecl(
    //         ofClass(hasName(record->getQualifiedNameAsString())),
    //         functionDecl(hasName("visit_edges")),
    //         isOverride(),
    //         hasDescendant(memberExpr(member(hasName("visit_edges"))).bind("member-call")));
    //
    //     base_visit_edges_finder.addMatcher(base_visit_edges_matcher, &base_visit_edges_callback);
    //     base_visit_edges_finder.matchAST(*context);
    //
    //     bool call_to_base_visit_edges_found = false;
    //
    //     for (auto const* call_expr : base_visit_edges_callback.matches()) {
    //         // FIXME: Can we constrain the matcher above to avoid looking directly at the source code?
    //         auto const* source_chars = context->getSourceManager().getCharacterData(call_expr->getBeginLoc());
    //         if (strncmp(source_chars, "Base::", 6) == 0) {
    //             call_to_base_visit_edges_found = true;
    //             break;
    //         }
    //     }
    //
    //     if (!call_to_base_visit_edges_found) {
    //         auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Missing call to Base::visit_edges");
    //         diag_engine.Report(visit_edges_method->getBeginLoc(), diag_id);
    //     }
    //
    //     // Search for uses of all fields that need visiting. We don't ensure they are _actually_ visited
    //     // with a call to visitor.visit(...), as that is too complex. Instead, we just assume that if the
    //     // field is accessed at all, then it is visited.
    //
    //     if (fields_that_need_visiting.empty())
    //         return;
    //
    //     MatchFinder field_access_finder;
    //     SimpleCollectMatchesCallback<clang::MemberExpr> field_access_callback("member-expr");
    //
    //     auto field_access_matcher = memberExpr(
    //         hasAncestor(cxxMethodDecl(hasName("visit_edges"))),
    //         hasObjectExpression(hasType(pointsTo(cxxRecordDecl(hasName(record->getName()))))))
    //                                     .bind("member-expr");
    //
    //     field_access_finder.addMatcher(field_access_matcher, &field_access_callback);
    //     field_access_finder.matchAST(visit_edges_method->getASTContext());
    //
    //     std::unordered_set<std::string> fields_that_are_visited;
    //     for (auto const* member_expr : field_access_callback.matches())
    //         fields_that_are_visited.insert(member_expr->getMemberNameInfo().getAsString());
    //
    //     auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "GC-allocated member is not visited in %0::visit_edges");
    //
    //     for (auto const* field : fields_that_need_visiting) {
    //         if (!fields_that_are_visited.contains(field->getNameAsString())) {
    //             auto builder = diag_engine.Report(field->getBeginLoc(), diag_id);
    //             builder << record->getName();
    //         }
    //     }
    // }
    //
    // void validate_record_macros(clang::ASTContext* context, clang::CXXRecordDecl const& record) const
    // {
    //     auto& source_manager = context->getSourceManager();
    //     auto record_range = record.getSourceRange();
    //
    //     // FIXME: The current macro detection doesn't recursively search through macro expansion,
    //     //        so if the record itself is defined in a macro, the JS_CELL/etc won't be found
    //     if (source_manager.isMacroBodyExpansion(record_range.getBegin()))
    //         return;
    //
    //     auto [expected_cell_macro_type, expected_base_name] = get_record_cell_macro_expectation(context, record);
    //     auto file_id = context->getSourceManager().getFileID(record.getLocation());
    //     auto it = m_macro_map.find(file_id.getHashValue());
    //     auto& diag_engine = context->getDiagnostics();
    //
    //     auto report_missing_macro = [&] {
    //         auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Expected record to have a %0 macro invocation");
    //         auto builder = diag_engine.Report(record.getLocation(), diag_id);
    //         builder << LibJSCellMacro::type_name(expected_cell_macro_type);
    //     };
    //
    //     if (it == m_macro_map.end()) {
    //         report_missing_macro();
    //         return;
    //     }
    //
    //     std::vector<clang::SourceRange> sub_ranges;
    //     for (auto const& sub_decl : record.decls()) {
    //         if (auto const* sub_record = llvm::dyn_cast<clang::CXXRecordDecl>(sub_decl))
    //             sub_ranges.push_back(sub_record->getSourceRange());
    //     }
    //
    //     bool found_macro = false;
    //
    //     auto record_name = record.getDeclName().getAsString();
    //     if (record.getQualifier()) {
    //         // FIXME: There has to be a better way to get this info. getQualifiedNameAsString() gets too much info
    //         //        (outer namespaces that aren't part of the class identifier), and getNameAsString() doesn't get
    //         //        enough info (doesn't include parts before the namespace specifier).
    //         auto loc = record.getQualifierLoc();
    //         auto& sm = context->getSourceManager();
    //         auto begin_offset = sm.getFileOffset(loc.getBeginLoc());
    //         auto end_offset = sm.getFileOffset(loc.getEndLoc());
    //         auto const* file_buf = sm.getCharacterData(loc.getBeginLoc());
    //         auto prefix = std::string { file_buf, end_offset - begin_offset };
    //         record_name = prefix + "::" + record_name;
    //     }
    //
    //     for (auto const& macro : it->second) {
    //         if (record_range.fullyContains(macro.range)) {
    //             bool macro_is_in_sub_decl = false;
    //             for (auto const& sub_range : sub_ranges) {
    //                 if (sub_range.fullyContains(macro.range)) {
    //                     macro_is_in_sub_decl = true;
    //                     break;
    //                 }
    //             }
    //             if (macro_is_in_sub_decl)
    //                 continue;
    //
    //             if (found_macro) {
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Record has multiple JS_CELL-like macro invocations");
    //                 diag_engine.Report(record_range.getBegin(), diag_id);
    //             }
    //
    //             found_macro = true;
    //             if (macro.type != expected_cell_macro_type) {
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Invalid JS-CELL-like macro invocation; expected %0");
    //                 auto builder = diag_engine.Report(macro.range.getBegin(), diag_id);
    //                 builder << LibJSCellMacro::type_name(expected_cell_macro_type);
    //             }
    //
    //             // This is a compile error, no diagnostic needed
    //             if (macro.args.size() < 2)
    //                 return;
    //
    //             if (macro.args[0].text != record_name) {
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Expected first argument of %0 macro invocation to be %1");
    //                 auto builder = diag_engine.Report(macro.args[0].location, diag_id);
    //                 builder << LibJSCellMacro::type_name(expected_cell_macro_type) << record_name;
    //             }
    //
    //             if (expected_cell_macro_type == LibJSCellMacro::Type::JSPrototypeObject) {
    //                 // FIXME: Validate the args for this macro
    //             } else if (macro.args[1].text != expected_base_name) {
    //                 auto diag_id = diag_engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "Expected second argument of %0 macro invocation to be %1");
    //                 auto builder = diag_engine.Report(macro.args[1].location, diag_id);
    //                 builder << LibJSCellMacro::type_name(expected_cell_macro_type) << expected_base_name;
    //             }
    //         }
    //     }
    //
    //     if (!found_macro)
    //         report_missing_macro();
    // }
    //
    // static CellMacroExpectation get_record_cell_macro_expectation(clang::ASTContext* context, clang::CXXRecordDecl const& record)
    // {
    //     auto origin = find_cell_type_with_origin(record);
    //     assert(origin.has_value());
    //
    //     // Need to iterate the bases again to turn the record into the exact text that the user used as
    //     // the class base, since it doesn't have to be qualified (but might be).
    //     for (auto const& base : record.bases()) {
    //         if (auto const* base_record = base.getType()->getAsCXXRecordDecl()) {
    //             if (base_record == &origin->base_origin) {
    //                 auto& source_manager = context->getSourceManager();
    //                 auto char_range = source_manager.getExpansionRange({ base.getBaseTypeLoc(), base.getEndLoc() });
    //                 auto exact_text = clang::Lexer::getSourceText(char_range, source_manager, context->getLangOpts());
    //                 return { origin->type, exact_text.str() };
    //             }
    //         }
    //     }
    //
    //     assert(false);
    // }

    LibJSCellMacroMap const& m_macro_map;
    bool m_detect_invalid_function_members;
    size_t m_pointer_width;
    clang::ast_matchers::MatchFinder m_finder;
};

std::unique_ptr<clang::ASTConsumer> LibJSGCPluginAction::CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef)
{
    auto& preprocessor = compiler.getPreprocessor();
    auto pp_callbacks = std::make_unique<LibJSPPCallbacks>(preprocessor);
    auto const& macro_map = pp_callbacks->macro_map();
    // preprocessor.addPPCallbacks(std::move(pp_callbacks));
    return std::make_unique<Consumer>(macro_map, m_detect_invalid_function_members, compiler.getTarget().getPointerWidth(clang::LangAS::Default));
}

static clang::FrontendPluginRegistry::Add<LibJSGCPluginAction> X("libjs_gc_scanner", "analyze LibJS GC usage");
