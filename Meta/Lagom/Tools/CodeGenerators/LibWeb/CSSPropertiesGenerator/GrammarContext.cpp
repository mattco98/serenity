/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GrammarContext.h"
#include <AK/JsonArray.h>

static ByteString grammar_value_to_string(JsonValue const& value)
{
    if (value.is_string())
        return value.as_string();
    VERIFY(value.is_object());
    return value.as_object().get_byte_string("supported-value"sv).value();
}

GrammarContext GrammarContext::create(JsonObject const& css_properties)
{
    auto const& grammar_object = css_properties.get_object("grammar"sv).value();

    GrammarContext context { &css_properties };

    grammar_object.get_array("non-numeric-base-types"sv).value().for_each([&](auto const& value) {
        VERIFY(value.is_string());
        auto name = value.as_string();
        auto type = adopt_ref(*new NonTerminalNode { NonTerminalNode::Base { name, NonTerminalNode::IsNumeric::No }});
        context.m_types.set(name, type);
    });

    grammar_object.get_array("numeric-base-types"sv).value().for_each([&](auto const& value) {
        VERIFY(value.is_string());
        auto name = value.as_string();
        auto type = adopt_ref(*new NonTerminalNode { NonTerminalNode::Base { name, NonTerminalNode::IsNumeric::Yes }});
        context.m_types.set(name, type);
    });

    grammar_object.get_object("derived-types"sv)->for_each_member([&](auto const& key, auto const& value) {
        // Due to the recursive nature of the grammar parsing, this type may have already been parsed
        if (context.m_types.contains(key))
            return;

        dbgln("[GrammarContext] parsing terminal {}", key);
        auto property_grammar = grammar_value_to_string(value);
        auto node = GrammarParser { property_grammar.view() }.parse();
        if (node.is_error()) {
            dbgln("Failed to parse grammar for derived type '{}': {}", key, node.release_error());
            VERIFY_NOT_REACHED();
        }

        dbgln("[GrammarContext] parsed terminal '{}': \"{}\"", key, node.value()->to_string());
        context.m_types.set(key, node.release_value());
    });

    auto const& properties_object = css_properties.get_object("properties"sv).value();
    properties_object.for_each_member([&](auto const& key, auto const& value) {
        // Due to the recursive nature of the grammar parsing, this property may have already been parsed
        if (context.m_property_nodes.contains(key))
            return;

        VERIFY(value.is_object());
        auto grammar = value.as_object().get("grammar"sv);
        if (!grammar.has_value()) {
            // This property hasn't been converted to the new generator, so skip it for now
            // FIXME: Remove this check when it is no longer necessary
            return;
        }

        dbgln("[GrammarContext] parsing property {}", key);

        auto property_grammar = grammar_value_to_string(*grammar);
        auto node = GrammarParser { property_grammar.view() }.parse();
        if (node.is_error()) {
            dbgln("Failed to parse grammar for property '{}': {}", key, node.release_error());
            VERIFY_NOT_REACHED();
        }

        dbgln("[GrammarContext] parsed property '{}': \"{}\"", key, node.value()->to_string());
        context.m_property_nodes.set(key, node.release_value());
    });

    // We no longer need this
    context.m_css_properties = nullptr;

    // Perform linking
    for (auto& [_, node] : context.m_types)
        node->link(context);
    for (auto& [_, node] : context.m_property_nodes)
        node->link(context);

    return context;
}

NonnullRefPtr<GrammarNode> GrammarContext::find_terminal(ByteString const& type)
{
    if (auto node = m_types.find(type); node != m_types.end())
        return node->value;
    dbgln("Invalid reference to undefined terminal '{}'", type);
    VERIFY_NOT_REACHED();
}

NonnullRefPtr<GrammarNode> GrammarContext::find_property(ByteString const& property)
{
    if (auto node = m_property_nodes.find(property); node != m_property_nodes.end())
        return node->value;
    dbgln("Invalid reference to undefined property '{}'", property);
    VERIFY_NOT_REACHED();
}
