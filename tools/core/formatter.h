#pragma once

#include <string>
#include <vector>

namespace ppl {

struct FormatOptions {
    int indentSize = 2;
    bool uppercaseKeywords = true;   // begin → BEGIN
};

namespace Formatter {

// Block depth for each line (index 0 = line 1). -1 means "leave the line untouched"
// (inside a multi-line string or #cas block).
std::vector<int> indentLevels(const std::u32string &text);

// Re-indents the whole program.
std::u32string format(const std::u32string &text, const FormatOptions &options = {});

// Indentation (in levels) expected for a new line inserted after the given 1-based line.
int indentForNewLine(const std::u32string &text, int line);

// <> <= >= → ≠ ≤ ≥ and back (outside strings and comments).
std::u32string toCalculatorSymbols(const std::u32string &text);
std::u32string toAscii(const std::u32string &text);

} // namespace Formatter
} // namespace ppl
