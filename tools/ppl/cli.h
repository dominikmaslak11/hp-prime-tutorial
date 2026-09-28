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
void printUsage();

} // namespace ppl::cli
