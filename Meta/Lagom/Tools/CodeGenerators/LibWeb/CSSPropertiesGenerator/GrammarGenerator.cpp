/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GrammarGenerator.h"

#include <GeneratorUtil.h>
#include <AK/SourceGenerator.h>

static u16 next_class_id = 1;
static u16 next_type_id = 1;

// template<typename T>
// static void visit_node(NonnullRefPtr<GrammarNode const> node, T& visitor)
// {
//     if (auto const* keyword_node = dynamic_cast<KeywordNode const*>(node.ptr())) {
//         visitor.visit_keyword(*keyword_node);
//     } else if (auto const* literal_node = dynamic_cast<LiteralNode const*>(node.ptr())) {
//         visitor.visit_literal(*literal_node);
//     } else if (auto const* non_terminal_node = dynamic_cast<NonTerminalNode const*>(node.ptr())) {
//         visitor.visit_non_terminal(*non_terminal_node);
//     } else if (auto const* combinator_node = dynamic_cast<CombinatorNode const*>(node.ptr())) {
//         visitor.visit_combinator(*combinator_node);
//     } else if (auto const* multiplier_node = dynamic_cast<MultiplierNode const*>(node.ptr())) {
//         visitor.visit_mutliplier(*multiplier_node);
//     } else {
//         VERIFY_NOT_REACHED();
//     }
// }

// class FieldDeclarationVisitor {
// public:
//     FieldDeclarationVisitor(size_t var_offset = 0)
//         : m_var_offset(var_offset)
//     {
//     }
//
//     void visit_keyword(KeywordNode const& node)
//     {
//         VERIFY_NOT_REACHED();
//     }
//
//     void visit_literal(LiteralNode const& node)
//     {
//
//     }
//
//     void visit_non_terminal(NonTerminalNode const& node)
//     {
//
//     }
//
//     void visit_combinator(CombinatorNode const& node)
//     {
//
//     }
//
//     void visit_multiplier(MultiplierNode const& node)
//     {
//
//     }
//
// private:
//     struct Field {
//         ByteString name; // without m_ prefix
//         ByteString type;
//         Optional<ByteString> definition_if_necessary;
//     };
//
//     size_t m_var_offset;
//     Vector<Field> m_fields;
// };

// <a> | <b>
// -> Variant<A, B>
// [<a> | <b>] && <c>
// -> struct { Variant<A, B>, C };

// "hsla( [<hue> | none] [<percentage> | <number> | none] [<percentage> | <number> | none] [ / [<alpha-value> | none] ]? ) | hsla()"
class ModernHslaSyntax {
public:
    struct Hsla0 {
        struct None {};
        using Group0 = Variant<Hue, None>;
        using Group1 = Variant<Percentage, Number, None>;
        using Group2 = Variant<Percentage, Number, None>;
        using Group3 = Variant<AlphaValue, None>;

        Optional<Group0> group_0;
        Optional<Group1> group_1;
        Optional<Group2> group_2;
        Optional<Group3> group_3;
    };

    struct Hsla1 {};

    using Group0 = Variant<Hsla0, Hsla1>;

    Group0 const& group_0() const { return m_group_0; }

private:
    Group0 m_group_0;
};

void foo()
{
    ModernHslaSyntax modern_hsla_syntax;

    if (auto const* hsla0 = modern_hsla_syntax.group_0().get_pointer<ModernHslaSyntax::Hsla0>()) {
        if (hsla0->group_2.has_value()) {
            // ...
        }
    }
}

// "hsla@with-args( @hue[<hue> | none] @saturation[<percentage> | <number> | none] @lightness[<percentage> | <number> | none] @alpha[ / [<alpha-value> | none] ]? ) | hsla@empty()"
class ModernHslaSyntax2 {
public:
    struct WithArgs {
        struct None {};
        using Hue = Variant<Hue, None>;
        using Saturation = Variant<Percentage, Number, None>;
        using Lightness = Variant<Percentage, Number, None>;
        using Alpha = Variant<AlphaValue, None>;

        Optional<Hue> hue;
        Optional<Saturation> saturation;
        Optional<Lightness> lightness;
        Optional<Alpha> alpha;
    };

    struct NoArgs {};

    using Root = Variant<WithArgs, NoArgs>;

    Root const& root() const { return m_root; }

private:
    Root m_root;
};

void foo()
{
    ModernHslaSyntax2 modern_hsla_syntax;

    if (auto const* with_args = modern_hsla_syntax.root().get_pointer<ModernHslaSyntax2::WithArgs>()) {
        if (with_args->lightness.has_value()) {
            // ...
        }
    }
}

struct NodeGenInfo {
    // No 'm_' prefix
    ByteString name;

    // Fully qualified type name
    ByteString type;

    // If 'type' is a new type, this is a list of any new declarations contained in that type that will be
    // placed at the top of the relevant class
    Vector<ByteString> custom_type_declarations;

    // If 'type' is a Vector, these are the size restrictions
    Optional<size_t> vector_min_size;
    Optional<size_t> vector_max_size;
};

// foo || [<a> <b>] || bar

static NodeGenInfo get_node_gen_info(ByteString css_name, NonnullRefPtr<GrammarNode const> node)
{
    if (auto const* literal_node = dynamic_cast<LiteralNode const*>(node.ptr())) {
        VERIFY_NOT_REACHED();
    }

    if (auto const* non_terminal_node = dynamic_cast<NonTerminalNode const*>(node.ptr())) {
        return non_terminal_node->visit(
            [&](NonTerminalNode::Base base) {

            });
    }

    if (auto const* combinator_node = dynamic_cast<CombinatorNode const*>(node.ptr())) {

    } else if (auto const* multiplier_node = dynamic_cast<MultiplierNode const*>(node.ptr())) {

    } else {
        VERIFY_NOT_REACHED();
    }
}

static void generate_type_header(SourceGenerator& parent_generator, ByteString css_class_name, NonnullRefPtr<GrammarNode const> node)
{
    auto cpp_class_name = title_casify(css_class_name);

    auto generator = parent_generator.fork();
    generator.set("@cpp_class_name@", cpp_class_name);
    generator.set("@class_id@", MUST(String::number(next_class_id++)));

    generator.append(R"~~~(
class @cpp_class_name@ : public Value {
public:
    static constexpr u16 ID = @class_id@;
)~~~");



    generator.append(R"~~~(
};
)~~~");
}

void generate_grammar_header_file(Core::File& file, GrammarContext& context)
{
    (void)context;

    StringBuilder builder;
    SourceGenerator generator { builder };

    generator.append(R"~~~(
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

    context.for_each_type([&](auto const& key, auto const& value) {
        generate_header(generator, key, value);
    });

    generator.append(R"~~~(

}
)~~~");

    TRY(file.write_until_depleted(generator.as_string_view().bytes()));
}

void generate_grammar_implementation_file(Core::File& file, GrammarContext& context)
{
    (void)context;

    StringBuilder builder;
    SourceGenerator generator { builder };

    TRY(file.write_until_depleted(generator.as_string_view().bytes()));
}
