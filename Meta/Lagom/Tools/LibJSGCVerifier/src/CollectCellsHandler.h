#pragma once

#include <unordered_set>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Tooling.h>

class CollectCellsHandler : public clang::tooling::SourceFileCallbacks
                          , public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    CollectCellsHandler();
    virtual ~CollectCellsHandler() override = default;

    virtual bool handleBeginSource(clang::CompilerInstance&) override;

    virtual void run(clang::ast_matchers::MatchFinder::MatchResult const& result) override;

    clang::ast_matchers::MatchFinder& finder() { return m_finder; }

private:
    std::unordered_set<std::string> m_visited_classes;
    clang::ast_matchers::MatchFinder m_finder;
    bool m_run_has_printed { false };
};
