#pragma once

#include "diagnostic.h"
#include "token.h"

#include <string>
#include <vector>

namespace ppl {

class Lexer {
public:
    // Splits PPL source into tokens. The last token is always EndOfFile.
    // Comments, pragmas and #cas blocks are returned only when keepTrivia is true.
    static std::vector<Token> tokenize(const std::u32string &source,
                                       std::vector<Diagnostic> *diagnostics = nullptr,
                                       bool keepTrivia = false);

    static bool isKeyword(const std::u32string &upper);
    static const std::vector<std::u32string> &keywords();
    static bool isIdentifierChar(char32_t c);
    static bool isIdentifierStart(char32_t c, char32_t next);
};

} // namespace ppl
