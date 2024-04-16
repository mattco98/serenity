/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <string>

template<typename T>
class SimpleCollectMatchesCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    explicit SimpleCollectMatchesCallback(std::string name)
        : m_name(std::move(name))
    {
    }

    template<typename M>
    void add_matcher(M&& matcher)
    {
        m_finder.addMatcher(matcher, this);
    }

    void match(clang::ASTContext& context)
    {
        m_finder.matchAST(context);
    }

    auto const& matches() const { return m_matches; }

private:
    virtual void run(clang::ast_matchers::MatchFinder::MatchResult const& result) override
    {
        if (auto const* node = result.Nodes.getNodeAs<T>(m_name))
            m_matches.push_back(node);
    }

    std::string m_name;
    std::vector<T const*> m_matches;
    clang::ast_matchers::MatchFinder m_finder;
};
