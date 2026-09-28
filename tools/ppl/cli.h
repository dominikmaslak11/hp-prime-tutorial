#pragma once

#include <string>
#include <vector>

namespace ppl::cli {

// Reads a source file (UTF-8 with or without BOM, or UTF-16 LE/BE with BOM) as UTF-8.
bool readSourceFile(const std::string &path, std::string &utf8, std::string *error = nullptr);
bool writeTextFile(const std::string &path, const std::string &utf8);

int runCheck(const std::vector<std::string> &args);
int runFormat(const std::vector<std::string> &args);
int runHelp(const std::vector<std::string> &args);
int runSearch(const std::vector<std::string> &args);
int runGuide(const std::vector<std::string> &args);
int runRun(const std::vector<std::string> &args);
struct BuildOptions {
    bool check = true;         // refuse to build a program with errors
    bool force = false;        // allow a template that already holds compiled code
    std::string templatePath;  // empty = template embedded in ppl.exe
    std::string sourceName = "program";
};
// Writes `src` as an HP Prime program file (.hpprgm); messages go to `log`. 0 = built.
int buildProgram(const std::string &src, const std::string &output, const BuildOptions &opt, std::string &log);
int runBuild(const std::vector<std::string> &args);
int runExtract(const std::vector<std::string> &args);
int runVerify(const std::vector<std::string> &args);
void printUsage();

} // namespace ppl::cli
