/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/GenericLexer.h>
#include <AK/JsonValue.h>

namespace AK {

namespace Detail {

template<bool IsJson5>
class JsonParser : private GenericLexer {
public:
    explicit JsonParser(StringView input)
        : GenericLexer(input)
    {
    }

    ErrorOr<JsonValue> parse();

private:
    ErrorOr<JsonValue> parse_helper();

    ErrorOr<ByteString> consume_and_unescape_string();
    ErrorOr<JsonValue> parse_array();
    ErrorOr<JsonValue> parse_object();
    ErrorOr<JsonValue> parse_number();
    ErrorOr<JsonValue> parse_string();
    ErrorOr<JsonValue> parse_false();
    ErrorOr<JsonValue> parse_true();
    ErrorOr<JsonValue> parse_null();

    void skip_whitespace();
};

}

using JsonParser = Detail::JsonParser<false>;

// FIXME: This is not a fully compliant JSON5 parser, but it is close enough to use for convenience
//        features like comments and trailing commas
using Json5Parser = Detail::JsonParser<true>;

}

#if USING_AK_GLOBALLY
using AK::JsonParser;
using AK::Json5Parser;
#endif
