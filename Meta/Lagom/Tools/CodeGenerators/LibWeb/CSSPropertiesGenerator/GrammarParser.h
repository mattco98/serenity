/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteString.h>
#include <AK/GenericLexer.h>

class GrammarContext;

class GrammarNode : public RefCounted<GrammarNode> {
public:
    virtual ~GrammarNode() = default;

    virtual void link(GrammarContext&) {}
    virtual ByteString to_string() const = 0;
};

class LiteralNode : public GrammarNode {
public:
    enum class IsKeyword {
        Yes,
        No,
    };

    LiteralNode(ByteString literal, IsKeyword is_keyword)
        : m_literal(move(literal))
        , m_is_keyword(is_keyword)
    {
    }

    virtual ~LiteralNode() override = default;

    ByteString const& literal() const { return m_literal; }
    IsKeyword is_keyword() const { return m_is_keyword; }

    virtual ByteString to_string() const override;

private:
    ByteString m_literal;
    IsKeyword m_is_keyword;
};

class NonTerminalNode;

namespace Detail {

struct Infinity {
    bool negative;
};

using RangeRestriction = Variant<Infinity, ByteString>;
struct RangeRestrictions {
    RangeRestriction min;
    RangeRestriction max;
};

enum class IsNumeric {
    Yes,
    No,
};

struct Base {
    ByteString name;
    IsNumeric is_numeric;
};

struct PropertyReference {
    ByteString property_name;

    // Null until linking
    RefPtr<GrammarNode> node {};
};

// <foo-bar-baz> -> CSS::Parse::FooBarBaz
// <angle> -> CSS::Parse::Angle
struct TerminalReference {
    ByteString name;
    Optional<RangeRestrictions> range_restrictions;

    // Null until linking
    RefPtr<GrammarNode> node {};
};

struct Function {
    ByteString name;
    NonnullRefPtr<GrammarNode> argument;
};

}

class NonTerminalNode
    : public GrammarNode
    , public Variant<Detail::PropertyReference, Detail::Base, Detail::TerminalReference, Detail::Function> {
public:
    using PropertyReference = Detail::PropertyReference;
    using Base = Detail::Base;
    using TerminalReference = Detail::TerminalReference;
    using Infinity = Detail::Infinity;
    using IsNumeric = Detail::IsNumeric;
    using RangeRestriction = Detail::RangeRestriction;
    using RangeRestrictions = Detail::RangeRestrictions;
    using Function = Detail::Function;

    using Variant::Variant;

    virtual ~NonTerminalNode() override = default;

    virtual void link(GrammarContext&) override;

    virtual ByteString to_string() const override;
};

class CombinatorNode : public GrammarNode {
public:
    // Ordered by operator precedence
    enum class Kind {
        Group,       // [ a b ]
        Juxtaposition, // a b
        Both,          // a && b
        OneOrMore,     // a || b
        One,           // a | b
    };

    CombinatorNode(Kind kind, Vector<NonnullRefPtr<GrammarNode>> const& nodes);

    virtual ~CombinatorNode() override = default;

    Kind kind() const { return m_kind; }
    auto const& nodes() const { return m_nodes; }

    virtual void link(GrammarContext&) override;

    virtual ByteString to_string() const override;

private:
    Kind m_kind;
    Vector<NonnullRefPtr<GrammarNode>> m_nodes;
};

namespace Detail {

struct ExactRepetition {
    size_t count;
};

struct RepetitionRange {
    size_t min;
    Optional<size_t> max;
};

using Repetition = Variant<ExactRepetition, RepetitionRange>;

struct CommaSeparatedList {
    Optional<Repetition> range;
};

struct NonEmpty {};

struct Optional {};

}

class MultiplierNode
    : public GrammarNode
    , public Variant<Detail::ExactRepetition, Detail::RepetitionRange, Detail::Optional, Detail::CommaSeparatedList, Detail::NonEmpty> {
public:
    using Repetition = Detail::Repetition;
    using ExactRepetition = Detail::ExactRepetition;
    using RepetitionRange = Detail::RepetitionRange;
    using Optional = Detail::Optional;
    using CommaSeparatedList = Detail::CommaSeparatedList;
    using NonEmpty = Detail::NonEmpty;

    template<typename... Args>
    MultiplierNode(NonnullRefPtr<GrammarNode> target, Args&&... args)
        : Variant(forward<Args>(args)...)
        , m_target(target)
    {
    }

    virtual ~MultiplierNode() override = default;

    virtual void link(GrammarContext&) override;

    virtual ByteString to_string() const override;

private:
    NonnullRefPtr<GrammarNode> m_target;
};

class GrammarParser {
    template<typename T>
    using ErrorOr = ErrorOr<T, ByteString>;

public:
    explicit GrammarParser(StringView grammar)
        : m_lexer(grammar)
    {
    }

    ErrorOr<NonnullRefPtr<GrammarNode>> parse();

private:
    ErrorOr<RefPtr<GrammarNode>> parse_node(Optional<CombinatorNode::Kind> lhs_combinator);
    ErrorOr<RefPtr<GrammarNode>> parse_primary_node();
    GrammarParser::ErrorOr<RefPtr<GrammarNode>> parse_multiplier(NonnullRefPtr<GrammarNode> target);
    Optional<CombinatorNode::Kind> parse_suffix();

    Optional<ByteString> parse_ident();

    void skip_whitespace();

    GenericLexer m_lexer;
};
