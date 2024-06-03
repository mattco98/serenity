/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GrammarParser.h"
#include "GrammarContext.h"
#include "ctype.h"
#include <AK/ScopeGuard.h>

#define CSS_GRAMMAR_DEBUG 0

void NonTerminalNode::link(GrammarContext& context)
{
    visit(
        [&](NonTerminalNode::PropertyReference& ref) {
            ref.node = context.find_property(ref.property_name);
        },
        [&](NonTerminalNode::TerminalReference& ref) {
            ref.node = context.find_terminal(ref.name);
        },
        [&](auto const&) {});
}

ByteString NonTerminalNode::to_string() const
{
    return visit(
        [](PropertyReference range) { return ByteString::formatted("<'{}'>", range.property_name); },
        [](TerminalReference ref) {
            if (ref.range_restrictions.has_value()) {
                auto min = ref.range_restrictions->min.visit(
                    [](Infinity inf) { return ByteString { inf.negative ? "-∞"sv : "∞"sv }; },
                    [](auto const& str) { return str; });
                auto max = ref.range_restrictions->max.visit(
                    [](Infinity inf) { return ByteString { inf.negative ? "-∞"sv : "∞"sv }; },
                    [](auto const& str) { return str; });
                return ByteString::formatted("<{} [{},{}]>", ref.name, min, max);
            }
            return ByteString::formatted("<{}>", ref.name);
        },
        [](Function function) {
            StringBuilder builder;
            builder.append(function.name.substring_view(0, function.name.length() - 2));
            builder.append("( "sv);
            builder.append(function.argument->to_string());
            builder.append(" )"sv);
            return builder.to_byte_string();
        },
        [](Base base) { return ByteString::formatted("<{}>", base.name); });
}

ByteString LiteralNode::to_string() const
{
    if (m_literal.is_one_of("/"sv, ","sv, "("sv, ")"sv))
        return m_literal;
    return ByteString::formatted("'{}'", m_literal);
}

CombinatorNode::CombinatorNode(Kind kind, Vector<NonnullRefPtr<GrammarNode>> const& nodes)
    : m_kind(kind)
    , m_nodes(nodes)
{
}

void CombinatorNode::link(GrammarContext& context)
{
    for (auto& node : m_nodes)
        node->link(context);
}

ByteString CombinatorNode::to_string() const
{
    StringBuilder builder;
    if (m_kind == Kind::Group)
        builder.append("[ "sv);

    bool first = true;
    for (auto const& node : m_nodes) {
        if (!first) {
            switch (m_kind) {
            case Kind::Both:
                builder.append(" && "sv);
                break;
            case Kind::OneOrMore:
                builder.append(" || "sv);
                break;
            case Kind::One:
                builder.append(" | "sv);
                break;
            case Kind::Juxtaposition:
            case Kind::Group:
                builder.append(' ');
                break;
            }
        }
        first = false;
        builder.append(node->to_string());
    }

    if (m_kind == Kind::Group)
        builder.append(" ]"sv);

    return builder.to_byte_string();
}

void MultiplierNode::link(GrammarContext& context)
{
    m_target->link(context);
}

ByteString MultiplierNode::to_string() const
{
    return visit(
        [&](MultiplierNode::ExactRepetition repetition) {
            return ByteString::formatted("{}{{{}}}", m_target->to_string(), repetition.count);
        },
        [&](MultiplierNode::RepetitionRange range) {
            if (range.max.has_value())
                return ByteString::formatted("{}{{{},{}}}", m_target->to_string(), range.min, *range.max);
            return ByteString::formatted("{}{{{},}}", m_target->to_string(), range.min);
        },
        [&](MultiplierNode::CommaSeparatedList list) {
            if (list.range.has_value()) {
                return list.range->visit(
                    [&](MultiplierNode::ExactRepetition repetition) {
                        return ByteString::formatted("{}#{{{}}}", m_target->to_string(), repetition.count);
                    },
                    [&](MultiplierNode::RepetitionRange range) {
                        if (range.max.has_value())
                            return ByteString::formatted("{}#{{{},{}}}", m_target->to_string(), range.min, *range.max);
                        return ByteString::formatted("{}#{{{},}}", m_target->to_string(), range.min);
                    });
            }
            return ByteString::formatted("{}#", m_target->to_string());
        },
        [&](MultiplierNode::NonEmpty) { return ByteString::formatted("{}!", m_target->to_string()); },
        [&](MultiplierNode::Optional) { return ByteString::formatted("{}?", m_target->to_string()); });
}

GrammarParser::ErrorOr<NonnullRefPtr<GrammarNode>> GrammarParser::parse()
{
    auto node = TRY(parse_node({}));
    if (!node)
        return ByteString::formatted("Unexpected character '{}'", m_lexer.peek());
    if (!m_lexer.is_eof())
        return ByteString::formatted("Expected eof, found '{}'", m_lexer.peek());
    return *node;
}

GrammarParser::ErrorOr<RefPtr<GrammarNode>> GrammarParser::parse_node(Optional<CombinatorNode::Kind> lhs_combinator)
{
    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] start");
    ScopeGuard guard { [] { dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] end"); } };

    Vector<NonnullRefPtr<GrammarNode>> juxtaposed_nodes;
    while (!m_lexer.is_eof()) {
        auto node = TRY(parse_primary_node());
        if (!node) {
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] failed to parse primary node");
            break;
        }
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] node = {}", node->to_string());
        while (auto new_node = TRY(parse_multiplier(*node))) {
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] node with multiplier: {}", new_node->to_string());
            node = new_node;
        }
        juxtaposed_nodes.append(*node);
        skip_whitespace();

        if (m_lexer.next_is('|') || m_lexer.next_is("&&"sv))
            break;
    }

    if (juxtaposed_nodes.is_empty())
        return RefPtr<GrammarNode> {};

    auto node = juxtaposed_nodes.size() == 1 ? juxtaposed_nodes[0] : adopt_ref(*new CombinatorNode { CombinatorNode::Kind::Juxtaposition, juxtaposed_nodes });
    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] result before suffix = {}", node->to_string());

    size_t lexer_pos = m_lexer.tell();
    if (auto suffix = parse_suffix(); suffix.has_value()) {
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] found suffix");
        if (lhs_combinator.has_value() && to_underlying(*suffix) > to_underlying(*lhs_combinator)) {
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] ...it has a higher precedence, aborting");
            // a b && c d || e f
            m_lexer.seek(lexer_pos);
            return node;
        }

        // a b || c d && e f
        auto rhs = TRY(parse_node(suffix));
        if (!rhs) {
            // This is only an error if the suffix has characters (i.e. isn't juxtaposition)
            if (*suffix != CombinatorNode::Kind::Juxtaposition) {
                dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] ERROR: failed to parse rhs of suffix");
                return ByteString::formatted("Unexpected character after combinator: '{}'", m_lexer.peek());
            }

            m_lexer.seek(lexer_pos);
            return node;
        }
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_node] rhs of suffix = {}", rhs->to_string());
        return adopt_ref(*new CombinatorNode { *suffix, { node, *rhs } });
    }

    return node;
}

GrammarParser::ErrorOr<RefPtr<GrammarNode>> GrammarParser::parse_primary_node()
{
    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] start");
    ScopeGuard guard { [] { dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] end"); } };

    skip_whitespace();
    if (m_lexer.consume_specific('[')) {
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] parsing bracket group");
        auto node = TRY(parse_node({}));
        if (!m_lexer.consume_specific(']'))
            return "Expected ']' after group opened with '['"sv;
        if (node) {
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] bracket contents = {}", node->to_string());
            return adopt_ref(*new CombinatorNode { CombinatorNode::Kind::Group, { *node } });
        }
        return RefPtr<GrammarNode> {};
    }

    if (m_lexer.consume_specific('<')) {
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] parsing non-terminal");
        auto non_terminal = TRY([&] -> ErrorOr<NonnullRefPtr<GrammarNode>> {
            if (m_lexer.consume_specific('\'')) {
                auto ident = parse_ident();
                if (!ident.has_value())
                    return "Expected a property identifier"sv;
                dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] found property ref, property = {}", *ident);
                if (!m_lexer.consume_specific('\''))
                    return "Malformed property value reference: expected closing \"'\""sv;
                return adopt_ref(*new NonTerminalNode { NonTerminalNode::PropertyReference { *ident, } });
            }

            auto ident = parse_ident();
            if (!ident.has_value())
                return "Expected a terminal identifier"sv;
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] ident = {}", *ident);
            if (m_lexer.consume_specific("()"sv)) {
                auto function_name = ByteString::formatted("{}()", *ident);
                dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] function = {}", function_name);
                return adopt_ref(*new NonTerminalNode { NonTerminalNode::TerminalReference { function_name, {} } });
            }

            skip_whitespace();
            if (m_lexer.consume_specific('[')) {
                dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] parsing numeric restriction");
                auto parse_range_restriction = [&] -> NonTerminalNode::RangeRestriction {
                    if (m_lexer.consume_specific("-∞"))
                        return NonTerminalNode::Infinity { .negative = true };
                    if (m_lexer.consume_specific("∞"))
                        return NonTerminalNode::Infinity { };
                    return ByteString { m_lexer.consume_until([](auto ch) { return ch == ',' || ch == ']'; }) };
                };

                auto min = parse_range_restriction();
                if (!m_lexer.consume_specific(','))
                    return "Expected comma after first numeric range restriction"sv;
                dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node]   min = {}", min.visit([](NonTerminalNode::Infinity) { return ByteString { "Infinity"sv }; }, [](auto const& s) { return s; }));
                auto max = parse_range_restriction();
                if (!m_lexer.consume_specific(']'))
                    return "Expected closing ']' after numeric range restriction"sv;
                dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node]   max = {}", max.visit([](NonTerminalNode::Infinity) { return ByteString { "Infinity"sv }; }, [](auto const& s) { return s; }));

                return adopt_ref(*new NonTerminalNode { NonTerminalNode::TerminalReference { *ident, NonTerminalNode::RangeRestrictions { min, max } } });
            }

            return adopt_ref(*new NonTerminalNode { NonTerminalNode::TerminalReference { *ident, {} } });
        }());

        if (!m_lexer.consume_specific('>'))
            return "Malformed property value reference: expected closing \">\""sv;

        return non_terminal;
    }

    switch (m_lexer.consume()) {
    case ',':
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] parsed ','");
        return adopt_ref(*new LiteralNode(","sv));
    case '/':
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] parsed '/'");
        return adopt_ref(*new LiteralNode("/"sv));
    case '\'': {
        auto literal = m_lexer.consume_until([](auto ch) { return ch == '\''; });
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] parsed '{}'", literal);
        return adopt_ref(*new LiteralNode(literal));
    }
    default:
        m_lexer.retreat();
        break;
    }

    auto ident = parse_ident();
    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] non-terminal ident: {}", ident);
    if (!ident.has_value())
        return RefPtr<GrammarNode> {};

    skip_whitespace();
    if (m_lexer.consume_specific('(')) {
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] found function args");
        auto node = TRY(parse_node({}));
        if (!node) {
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] ERROR: no function args");
            return "Unexpected function call with no arguments"sv;
        }
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] function args = {}", node->to_string());

        skip_whitespace();
        if (!m_lexer.consume_specific(')'))
            return "Expected ')' after function arguments opened with '('"sv;

        auto function_name = ByteString::formatted("{}()", ident.release_value());
        return adopt_ref(*new NonTerminalNode { NonTerminalNode::Function { function_name, *node } });
    }

    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_primary_node] keyword = {}", *ident);
    return adopt_ref(*new KeywordNode(ident.release_value()));
}

GrammarParser::ErrorOr<RefPtr<GrammarNode>> GrammarParser::parse_multiplier(NonnullRefPtr<GrammarNode> target)
{
    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_multiplier] start");
    ScopeGuard guard { [] { dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_multiplier] end"); } };

    skip_whitespace();

    auto parse_repetition = [&] -> ErrorOr<Optional<MultiplierNode::Repetition>> {
        if (!m_lexer.consume_specific('{'))
            return Optional<MultiplierNode::Repetition> {};
        auto min_bound = m_lexer.consume_decimal_integer<size_t>();
        if (min_bound.is_error())
            return "Expected numeric minimum repetition bound after opening '{'"sv;
        if (m_lexer.consume_specific(',')) {
            Optional<size_t> max_bound;
            if (auto max = m_lexer.consume_decimal_integer<size_t>(); !max.is_error())
                max_bound = max.release_value();
            if (!m_lexer.consume_specific('}'))
                return "Expected closing '}' for repetition range"sv;
            return MultiplierNode::RepetitionRange { min_bound.release_value(), max_bound };
        }
        if (!m_lexer.consume_specific('}'))
            return "Expected closing '}' for repetition range"sv;
        return MultiplierNode::ExactRepetition { min_bound.release_value() };
    };

    RefPtr<GrammarNode> result_node;

    if (m_lexer.consume_specific('+')) {
        result_node = adopt_ref(*new MultiplierNode(target, MultiplierNode::RepetitionRange { 1, {} }));
    } else if (m_lexer.consume_specific('*')) {
        result_node = adopt_ref(*new MultiplierNode(target, MultiplierNode::RepetitionRange { 0, {} }));
    } else if (m_lexer.consume_specific('#')) {
        if (auto range = TRY(parse_repetition()); range.has_value()) {
            result_node = adopt_ref(*new MultiplierNode(target, MultiplierNode::CommaSeparatedList { range.value() }));
        } else {
            result_node = adopt_ref(*new MultiplierNode(target, MultiplierNode::CommaSeparatedList { {} }));
        }
    } else if (auto range = TRY(parse_repetition()); range.has_value()) {
        result_node = adopt_ref(*new MultiplierNode(target, *range));    
    } else if (m_lexer.consume_specific('!')) {
        result_node = adopt_ref(*new MultiplierNode(target, MultiplierNode::NonEmpty {}));
    } else if (m_lexer.consume_specific('?')) {
        result_node = adopt_ref(*new MultiplierNode(target, MultiplierNode::Optional {}));
    }

    if (result_node) {
        if (auto nested_node = TRY(parse_multiplier(*result_node))) {
            dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_multiplier] nested_node = {}", nested_node->to_string());
            return nested_node;
        }
        dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_multiplier] result_node = {}", result_node->to_string()); 
        return result_node;
    }

    dbgln_if(CSS_GRAMMAR_DEBUG, "[parse_multiplier] no multiplier found");
    return RefPtr<GrammarNode> {};
}

Optional<CombinatorNode::Kind> GrammarParser::parse_suffix()
{
    skip_whitespace();
    if (m_lexer.consume_specific("&&"sv))
        return CombinatorNode::Kind::Both;
    if (m_lexer.consume_specific("||"sv))
        return CombinatorNode::Kind::OneOrMore;
    if (m_lexer.consume_specific("|"sv))
        return CombinatorNode::Kind::One;
    if (!m_lexer.is_eof())
        return CombinatorNode::Kind::Juxtaposition;
    return {};
}

Optional<ByteString> GrammarParser::parse_ident()
{
    auto str = m_lexer.consume_while([](auto ch) {
        return isalnum(ch) || ch == '-';
    });

    if (str.is_empty())
        return {};
    return str;
}


void GrammarParser::skip_whitespace()
{
    m_lexer.ignore_while(isspace);
}
