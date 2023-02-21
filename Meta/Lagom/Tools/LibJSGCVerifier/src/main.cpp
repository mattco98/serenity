#include "CollectCellsHandler.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

#include <unordered_set>
#include <filesystem>

using namespace llvm;

static cl::OptionCategory s_tool_category("libjs-gc-verifier options");
static cl::extrahelp s_common_help(clang::tooling::CommonOptionsParser::HelpMessage);

int main(int argc, char const** argv)
{
    auto maybe_parser = clang::tooling::CommonOptionsParser::create(argc, argv, s_tool_category);
    if (!maybe_parser) {
        errs() << maybe_parser.takeError();
        return 1;
    }
    auto& parser = maybe_parser.get();
    clang::tooling::ClangTool tool(parser.getCompilations(), parser.getSourcePathList());

    CollectCellsHandler collect_handler;

    auto collect_action = clang::tooling::newFrontendActionFactory(&collect_handler.finder(), &collect_handler);

    if (auto value = tool.run(collect_action.get()); value != 0)
        return value;

    return 0;
}
