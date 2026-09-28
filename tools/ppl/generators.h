#pragma once

#include <string>
#include <vector>

namespace ppl::gen {

// TextMate grammar (JSON) for VS Code.
std::string textMateGrammar();

// Notepad++ User Defined Language XML and auto-completion API XML.
std::string notepadUdl();
std::string notepadApi();

int run(const std::vector<std::string> &args);

} // namespace ppl::gen
