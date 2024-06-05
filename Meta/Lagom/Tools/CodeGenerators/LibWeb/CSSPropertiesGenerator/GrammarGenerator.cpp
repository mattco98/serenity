/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GrammarGenerator.h"

#include <GeneratorUtil.h>
#include <AK/HashTable.h>
#include <AK/SourceGenerator.h>

static ByteString indent(ByteString const& str, size_t indent)
{
    StringBuilder builder;
    for (auto ch : str) {
        builder.append(ch);
        if (ch == '\n') {
            for (size_t i = 0; i < indent; i++)
                builder.append("    "sv);
        }
    }
    return builder.to_byte_string();
}

struct FieldInfo {
    ByteString name;
    ByteString type;
    Optional<ByteString> type_declaration;
};

struct FieldInfoState {
    HashTable<ByteString>& literals;
    ByteString top_level_class_name;
    size_t& next_class_id;
    size_t depth;

    FieldInfoState with_added_depth() const
    {
        return { literals, top_level_class_name, next_class_id, depth + 1 };
    }
};

static FieldInfo generate_non_terminal_field_type(NonTerminalNode const& node, FieldInfoState);
static FieldInfo generate_combinator_field_type(CombinatorNode const& node, FieldInfoState);
static FieldInfo generate_multiplier_field_type(MultiplierNode const& node, FieldInfoState);

static FieldInfo generate_field_type(GrammarNode const& node, FieldInfoState state)
{
    if (auto const* literal_node = dynamic_cast<LiteralNode const*>(&node)) {
        auto literal = literal_node->literal();
        if (literal == "/"sv)
            literal = "slash";
        else if (literal == ","sv)
            literal = "comma";
        auto title_case = title_casify(literal).to_byte_string();
        auto snake_case = snake_casify(literal).to_byte_string();
        state.literals.set(title_case);
        return { snake_case, ByteString::formatted("Literal::{}", title_case), {} };
    }

    if (auto const* non_terminal_node = dynamic_cast<NonTerminalNode const*>(&node))
        return generate_non_terminal_field_type(*non_terminal_node, move(state));

    if (auto const* combinator_node = dynamic_cast<CombinatorNode const*>(&node))
        return generate_combinator_field_type(*combinator_node, move(state));
    
    if (auto const* multiplier_node = dynamic_cast<MultiplierNode const*>(&node))
        return generate_multiplier_field_type(*multiplier_node, move(state));

    VERIFY_NOT_REACHED();
}

static FieldInfo generate_non_terminal_field_type(NonTerminalNode const& node, FieldInfoState state)
{
    return node.visit(
        [&](NonTerminalNode::Base const& base) {
            // FIXME: Use double for numeric properties?
            return FieldInfo { snake_casify(base.name).to_byte_string(), ByteString::formatted("CSS::Parse::{}", title_casify(base.name)), {} };
        },
        [&](NonTerminalNode::PropertyReference const& ref) {
            return FieldInfo { snake_casify(ref.property_name).to_byte_string(), ByteString::formatted("CSS::Parse::{}", title_casify(ref.property_name)), {} };
        },
        [&](NonTerminalNode::TerminalReference const& ref) {
            auto name = ref.name;
            if (name.ends_with("()"sv))
                name = ByteString::formatted("{}-func"sv, name.substring(0, name.length() - 2));
            return FieldInfo { snake_casify(name).to_byte_string(), ByteString::formatted("CSS::Parse::{}", title_casify(name)), {} };
        },
        [&](NonTerminalNode::Function const& func) {
            auto arg_type = generate_field_type(func.argument, state);
            auto name = ByteString::formatted("{}-call"sv, func.name.substring(0, func.name.length() - 2));
            arg_type.name = name;
            return arg_type;
        });
}

static FieldInfo generate_multiplier_field_type(MultiplierNode const& node, FieldInfoState state)
{
    // Same depth, since this will never generate a new class type
    auto base_type = generate_field_type(node.target(), move(state));

    return node.visit(
        [&](MultiplierNode::Optional) {
            return FieldInfo {
                move(base_type.name),
                ByteString::formatted("Optional<{}>", move(base_type.type)),
                move(base_type.type_declaration),
            };
        },
        [&](MultiplierNode::NonEmpty) { return base_type; },
        [&](auto const&) {
            // The rest of them are just Vectors
            return FieldInfo { 
                move(base_type.name),
                ByteString::formatted("Vector<{}>", move(base_type.type)),
                move(base_type.type_declaration),
            };
        });
}

static FieldInfo generate_combinator_field_type(CombinatorNode const& node, FieldInfoState state)
{
    if (node.kind() == CombinatorNode::Kind::Group) {
        // This should always wrap a single child node, and if that node is a combinator, just delegate to
        // that node. Otherwise, we need to make a wrapper class
        VERIFY(node.nodes().size() == 1);
        return generate_field_type(*node.nodes()[0], move(state));
    }

    Vector<FieldInfo> child_types;
    for (auto const& child : node.nodes())
        child_types.append(generate_field_type(child, state.with_added_depth()));

    if (node.kind() == CombinatorNode::Kind::One) {
        // Just generate a variant
        StringBuilder declaration_builder;
        StringBuilder type_builder;
        type_builder.append("Variant<"sv);
        bool first = true;

        for (auto const& [_, type, declaration] : child_types) {
            if (!first)
                type_builder.append(", "sv);

            // Preserve any necessary declarations;
            if (declaration.has_value())
                declaration_builder.append(*declaration);

            first = false;
            type_builder.append(type);
        }

        type_builder.append('>');

        // FIXME: Better default field names
        auto name = ByteString::formatted("field_{}"sv, state.next_class_id++);
        if (declaration_builder.is_empty())
            return { move(name), type_builder.to_byte_string(), {} };
        return { move(name), type_builder.to_byte_string(), declaration_builder.to_byte_string() };
    }

    // The rest of the type require a custom class, and are mostly the same. Juxtaposition has all
    // non-optional fields, whereas the other combinators are all optional
    auto wrap_type = [&](auto const& type) {
        if (node.kind() == CombinatorNode::Kind::Juxtaposition)
            return type;
        return ByteString::formatted("Optional<{}>"sv, type);
    };

    StringBuilder builder;
    SourceGenerator generator { builder };

    // FIXME: Custom combinator names
    auto id = state.next_class_id++;
    auto class_name = state.top_level_class_name;
    if (state.depth != 0)
        class_name = ByteString::formatted("Group{}"sv, id);
    generator.set("class_name"sv, class_name);
    generator.set("class_id"sv, ByteString::number(id));

    generator.append(R"~~~(
class @class_name@ {
public:)~~~");

    StringBuilder ctor_params;
    StringBuilder ctor_initializer;
    bool first = true;

    for (auto const& [name, type, declaration] : child_types) {
        if (declaration.has_value()) {
            generator.append(*declaration);
        }

        if (!first)
            ctor_params.append(", "sv);
        ctor_initializer.appendff("\n        {} "sv, first ? ':' : ',');
        first = false;

        ctor_params.appendff("{} m_{}"sv, wrap_type(type), name);
        ctor_initializer.appendff("m_{}({})"sv, name, name);
    }

    generator.set("ctor_params"sv, ctor_params.to_byte_string());
    generator.set("ctor_init"sv, ctor_initializer.to_byte_string());

    generator.append(R"~~~(
    @class_name@(@ctor_params@)@ctor_init@ 
    {
    }
)~~~");

    for (auto const& [name, type, _] : child_types) {
        generator.set("field_name", name);
        generator.append(R"~~~(
    auto const& @field_name@() const { return m_@field_name@; })~~~");
    };

    generator.append(R"~~~(

private:)~~~");

    for (auto const& [name, type, _] : child_types) {
        generator.set("field_name", name);
        generator.set("field_type", wrap_type(type));
        generator.append(R"~~~(
    @field_type@ m_@field_name@;)~~~");
    }

    generator.append(R"~~~(
};
)~~~");

    return {
        snake_casify(class_name).to_byte_string(),
        ByteString::formatted("CSS::Parse::{}", class_name),
        indent(builder.to_byte_string(), state.depth),
    };
}

static ByteString generate_type_header(ByteString css_class_name, NonnullRefPtr<GrammarNode const> node, HashTable<ByteString>& literals)
{
    auto cpp_class_name = title_casify(css_class_name).to_byte_string();
    if (cpp_class_name.ends_with("()"sv))
        cpp_class_name = ByteString::formatted("{}Func"sv, cpp_class_name.substring(0, cpp_class_name.length() - 2));

    StringBuilder builder;
    
    size_t next_class_id = 0;
    dbgln("generate_field_type for '{}'", node->to_string());
    auto type = generate_field_type(node, { literals, cpp_class_name, next_class_id, 0 });

    if (type.type_declaration.has_value()) {
        builder.append(*type.type_declaration);
    } else {
        builder.appendff("\nusing {} = {};\n"sv, cpp_class_name, type.type);
    }

    return builder.to_byte_string();
}

ErrorOr<void> generate_grammar_header_file(Core::File& file, GrammarContext& context)
{
    StringBuilder builder;
    SourceGenerator generator { builder };

    generator.append(R"~~~(
#include <LibWeb/CSS/ParseBuiltins.h>

namespace CSS::Parse {

class Value : public RefCounted<Value> {
public:
    Value(u16 id)
        : m_id(id)
    {
    }

    u16 id() const { return m_id; }

private:
    u16 m_id;
};

)~~~");

    HashTable<ByteString> literals;
    StringBuilder type_builder;

    context.for_each_type([&](auto const& key, auto const& value) {
        type_builder.append(generate_type_header(key, value, literals));
    });

    generator.append("// Literal markers\nnamespace Literal {\n\n");
    for (auto const& literal : literals) {
        generator.set("literal"sv, title_casify(literal));
        generator.append("struct @literal@ {};\n");
    }
    generator.append("\n}");

    generator.append(type_builder.to_byte_string());

    generator.append(R"~~~(

}

)~~~");

    TRY(file.write_until_depleted(generator.as_string_view().bytes()));
    return {};
}

ErrorOr<void> generate_grammar_implementation_file(Core::File& file, GrammarContext& context)
{
    (void)context;

    StringBuilder builder;
    SourceGenerator generator { builder };

    TRY(file.write_until_depleted(generator.as_string_view().bytes()));
    return {};
}
