#include "cli.h"
#include "generators.h"
#include "lsp.h"
#include "mcp.h"

#include <cstdio>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args[0] == "-h" || args[0] == "--help") {
        ppl::cli::printUsage();
        return args.empty() ? 2 : 0;
    }
    std::string cmd = args[0];
    std::vector<std::string> rest(args.begin() + 1, args.end());

    if (cmd == "check") return ppl::cli::runCheck(rest);
    if (cmd == "format") return ppl::cli::runFormat(rest);
    if (cmd == "help") return ppl::cli::runHelp(rest);
    if (cmd == "search") return ppl::cli::runSearch(rest);
    if (cmd == "guide") return ppl::cli::runGuide(rest);
    if (cmd == "lsp") return ppl::lsp::run();
    if (cmd == "mcp") return ppl::mcp::run();
    if (cmd == "gen") return ppl::gen::run(rest);
    if (cmd == "version" || cmd == "--version") {
        std::printf("ppl %s\n", PPL_VERSION);
        return 0;
    }
    // "ppl PLIK.hpppl" = check
    if (cmd.size() > 0 && cmd[0] != '-')
        return ppl::cli::runCheck(args);
    ppl::cli::printUsage();
    return 2;
}
