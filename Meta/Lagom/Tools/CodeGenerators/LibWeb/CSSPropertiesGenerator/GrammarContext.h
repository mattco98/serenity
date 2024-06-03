/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "GrammarParser.h"
#include <AK/ByteString.h>
#include <AK/HashMap.h>
#include <AK/JsonObject.h>

class GrammarContext {
public:
    static GrammarContext create(JsonObject const& css_properties);

    NonnullRefPtr<GrammarNode> find_terminal(ByteString const& type);
    NonnullRefPtr<GrammarNode> find_property(ByteString const& property);

private:
    GrammarContext(JsonObject const* css_properties)
        : m_css_properties(css_properties)
    {
    }

    JsonObject const* m_css_properties;
    HashMap<ByteString, NonnullRefPtr<GrammarNode>> m_types;
    HashMap<ByteString, NonnullRefPtr<GrammarNode>> m_property_nodes;
};
