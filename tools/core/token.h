#pragma once

#include <string>

namespace ppl {

enum class TokenKind {
    Identifier,
    Keyword,
    Number,      // 12, 1.5E-3, 5_m, 9.81_(m/s^2)
    Integer,     // #FFh, #1101b
    String,      // "text"
    QuotedExpr,  // 'X^2'
    Operator,    // := == <> + - ...
    Punct,       // ( ) { } [ ] , ;
    Comment,     // // ...
    Pragma,      // #pragma ...
    CasBlock,    // #cas ... #end
    Invalid,
    EndOfFile
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::u32string text;
    std::u32string upper;   // ASCII upper-cased text
    int offset = 0;         // 0-based code point offset
    int length = 0;         // in code points
    int line = 1;           // 1-based
    int column = 1;         // 1-based, code points
    int endLine = 1;        // line of the last character
    bool spaceBefore = true;

    bool isKeyword(const char32_t *kw) const { return kind == TokenKind::Keyword && upper == kw; }
    bool isOp(const char32_t *op) const { return kind == TokenKind::Operator && text == op; }
    bool isPunct(char32_t c) const { return kind == TokenKind::Punct && text.size() == 1 && text[0] == c; }
    bool isEof() const { return kind == TokenKind::EndOfFile; }
};

} // namespace ppl
