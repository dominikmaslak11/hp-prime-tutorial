#include "lexer.h"

#include "utf8.h"

#include <algorithm>

namespace ppl {

namespace {

bool isTypographicQuote(char32_t c)
{
    return c == 0x201E || c == 0x201D || c == 0x201C || c == 0x2018 || c == 0x2019 || c == 0x00AB || c == 0x00BB;
}

bool isHexRunChar(char32_t c)
{
    return isDigit(c) || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U':';
}

bool validIntegerLiteral(const std::u32string &body)
{
    // body without '#': [0-9A-F]+ (':' digits)? [bodh]?
    if (body.empty())
        return false;
    size_t i = 0;
    size_t n = body.size();
    size_t colon = body.find(U':');
    std::u32string main = colon == std::u32string::npos ? body : body.substr(0, colon);
    std::u32string rest = colon == std::u32string::npos ? std::u32string() : body.substr(colon + 1);
    if (main.empty())
        return false;
    for (i = 0; i < main.size(); ++i) {
        char32_t c = main[i];
        bool hex = isDigit(c) || (c >= U'a' && c <= U'f') || (c >= U'A' && c <= U'F');
        bool suffix = (i == main.size() - 1) && (c == U'b' || c == U'B' || c == U'o' || c == U'O' || c == U'd'
                                                 || c == U'D' || c == U'h' || c == U'H');
        if (!hex && !suffix)
            return false;
    }
    if (colon != std::u32string::npos) {
        if (rest.empty())
            return false;
        for (i = 0; i < rest.size(); ++i) {
            char32_t c = rest[i];
            bool suffix = (i == rest.size() - 1) && (c == U'b' || c == U'o' || c == U'd' || c == U'h' || c == U'B'
                                                     || c == U'O' || c == U'D' || c == U'H');
            if (!isDigit(c) && !suffix)
                return false;
        }
    }
    (void)n;
    return true;
}

} // namespace

const std::vector<std::u32string> &Lexer::keywords()
{
    static const std::vector<std::u32string> kw = {
        U"BEGIN", U"END", U"RETURN", U"KILL", U"IF", U"THEN", U"ELSE", U"CASE", U"DEFAULT", U"IFERR",
        U"FOR", U"FROM", U"TO", U"DOWNTO", U"STEP", U"DO", U"WHILE", U"REPEAT", U"UNTIL", U"BREAK",
        U"CONTINUE", U"LOCAL", U"EXPORT", U"VIEW", U"KEY", U"AND", U"OR", U"XOR", U"NOT", U"MOD"};
    return kw;
}

bool Lexer::isKeyword(const std::u32string &upper)
{
    const auto &kw = keywords();
    return std::find(kw.begin(), kw.end(), upper) != kw.end();
}

bool Lexer::isIdentifierChar(char32_t c)
{
    return isLetter(c) || isDigit(c) || c == U'_' || c == 0x2192 /* → */;
}

bool Lexer::isIdentifierStart(char32_t c, char32_t next)
{
    if (isLetter(c) || c == U'_')
        return true;
    if (c == 0x2192 && isLetter(next))
        return true; // →HMS
    if (c == U'%' && isLetter(next))
        return true; // %CHANGE, %TOTAL
    return false;
}

std::vector<Token> Lexer::tokenize(const std::u32string &s, std::vector<Diagnostic> *diags, bool keepTrivia)
{
    std::vector<Token> out;
    const int n = static_cast<int>(s.size());
    int i = 0;
    int line = 1;
    int lineStart = 0;
    bool space = true;

    auto at = [&](int k) -> char32_t { return k < n ? s[k] : 0; };

    auto diag = [&](Severity sev, int off, int len, int ln, int col, std::string msg, const char *code) {
        if (!diags)
            return;
        Diagnostic d;
        d.severity = sev;
        d.offset = off;
        d.length = std::max(1, len);
        d.line = ln;
        d.column = col;
        d.message = std::move(msg);
        d.code = code;
        diags->push_back(std::move(d));
    };

    while (i < n) {
        char32_t c = s[i];
        if (c == U'\n') {
            ++i;
            ++line;
            lineStart = i;
            space = true;
            continue;
        }
        if (isSpace(c)) {
            ++i;
            space = true;
            continue;
        }

        const int start = i;
        const int tline = line;
        const int tcol = i - lineStart + 1;
        TokenKind kind = TokenKind::Invalid;

        auto finish = [&](TokenKind k) {
            Token t;
            t.kind = k;
            t.offset = start;
            t.length = i - start;
            t.text = s.substr(start, i - start);
            t.upper = asciiUpper(t.text);
            t.line = tline;
            t.column = tcol;
            for (int j = start; j < i; ++j) {
                if (s[j] == U'\n') {
                    ++line;
                    lineStart = j + 1;
                }
            }
            t.endLine = line;
            t.spaceBefore = space;
            space = false;
            return t;
        };

        // Comments
        if (c == U'/' && at(i + 1) == U'/') {
            while (i < n && s[i] != U'\n')
                ++i;
            Token t = finish(TokenKind::Comment);
            space = true;
            if (keepTrivia)
                out.push_back(std::move(t));
            continue;
        }

        // Directives and binary integers
        if (c == U'#') {
            int j = i + 1;
            while (j < n && isLetter(s[j]))
                ++j;
            std::u32string word = asciiUpper(s.substr(i + 1, j - i - 1));
            if (word == U"PRAGMA") {
                while (i < n && s[i] != U'\n')
                    ++i;
                Token t = finish(TokenKind::Pragma);
                if (keepTrivia)
                    out.push_back(std::move(t));
                continue;
            }
            if (word == U"CAS") {
                // skip until a line containing #end
                int k = j;
                bool found = false;
                while (k < n) {
                    if (s[k] == U'#') {
                        std::u32string w = asciiUpper(s.substr(k + 1, 3));
                        if (w == U"END" && !Lexer::isIdentifierChar(at(k + 4))) {
                            k += 4;
                            found = true;
                            break;
                        }
                    }
                    ++k;
                }
                if (!found)
                    diag(Severity::Error, i, 4, tline, tcol, "Blok #cas nie jest zamknięty przez #end.", "unterminated-cas");
                i = k;
                Token t = finish(TokenKind::CasBlock);
                if (keepTrivia)
                    out.push_back(std::move(t));
                continue;
            }
            if (word == U"END") {
                i = j;
                Token t = finish(TokenKind::Invalid);
                diag(Severity::Error, start, i - start, tline, tcol, "#end bez odpowiadającego #cas.", "stray-end-directive");
                continue;
            }
            j = i + 1;
            while (j < n && isHexRunChar(s[j]))
                ++j;
            std::u32string body = s.substr(i + 1, j - i - 1);
            i = j;
            if (!validIntegerLiteral(body)) {
                out.push_back(finish(TokenKind::Invalid));
                diag(Severity::Error, start, i - start, tline, tcol,
                     "Niepoprawna liczba całkowita. Poprawny zapis to np. #FFh, #1101b, #17o, #99d.", "bad-integer");
            } else {
                out.push_back(finish(TokenKind::Integer));
            }
            continue;
        }

        // Strings
        if (c == U'"') {
            int j = i + 1;
            bool closed = false;
            while (j < n) {
                if (s[j] == U'"') {
                    if (at(j + 1) == U'"') {
                        j += 2;
                        continue;
                    }
                    closed = true;
                    break;
                }
                if (s[j] == U'\\') {
                    j += 2;
                    continue;
                }
                ++j;
            }
            i = closed ? j + 1 : n;
            out.push_back(finish(TokenKind::String));
            if (!closed)
                diag(Severity::Error, start, 1, tline, tcol, "Niezamknięty tekst: brakuje cudzysłowu \".", "unterminated-string");
            continue;
        }

        if (isTypographicQuote(c)) {
            ++i;
            out.push_back(finish(TokenKind::Invalid));
            diag(Severity::Error, start, 1, tline, tcol,
                 "Cudzysłów typograficzny. Kalkulator akceptuje tylko prosty cudzysłów \" (typowe przy kopiowaniu z internetu lub Worda).",
                 "typographic-quote");
            continue;
        }

        // Quoted expressions 'X^2'
        if (c == U'\'') {
            int j = i + 1;
            while (j < n && s[j] != U'\'' && s[j] != U'\n')
                ++j;
            bool closed = j < n && s[j] == U'\'';
            i = closed ? j + 1 : j;
            out.push_back(finish(TokenKind::QuotedExpr));
            if (!closed)
                diag(Severity::Error, start, 1, tline, tcol, "Niezamknięte wyrażenie w apostrofach.", "unterminated-quote");
            continue;
        }

        // Numbers
        if (isDigit(c) || (c == U'.' && isDigit(at(i + 1)))) {
            while (i < n && isDigit(s[i]))
                ++i;
            if (at(i) == U'.' && !(at(i + 1) == U'*' || at(i + 1) == U'/' || at(i + 1) == U'^')) {
                ++i;
                while (i < n && isDigit(s[i]))
                    ++i;
            }
            char32_t e = at(i);
            if (e == U'E' || e == U'e' || e == 0x1D07) {
                int k = i + 1;
                if (at(k) == U'+' || at(k) == U'-' || at(k) == 0x2212)
                    ++k;
                if (isDigit(at(k))) {
                    i = k;
                    while (i < n && isDigit(s[i]))
                        ++i;
                }
            }
            // unit suffix: 5_m, 9.81_(m/s^2)
            if (at(i) == U'_' && (isLetter(at(i + 1)) || at(i + 1) == U'(')) {
                ++i;
                if (s[i] == U'(') {
                    int depth = 0;
                    while (i < n) {
                        if (s[i] == U'(')
                            ++depth;
                        else if (s[i] == U')' && --depth == 0) {
                            ++i;
                            break;
                        } else if (s[i] == U'\n')
                            break;
                        ++i;
                    }
                } else {
                    while (i < n && (isIdentifierChar(s[i]) || s[i] == U'^' || isDigit(s[i])))
                        ++i;
                }
            }
            out.push_back(finish(TokenKind::Number));
            continue;
        }

        // Identifiers and keywords
        if (isIdentifierStart(c, at(i + 1))) {
            ++i; // first character (may be % or →, which are not continuation characters)
            while (i < n && isIdentifierChar(s[i]))
                ++i;
            // qualified names: Function.Xmin, CAS.idivis
            while (at(i) == U'.' && isIdentifierStart(at(i + 1), at(i + 2))) {
                ++i;
                while (i < n && isIdentifierChar(s[i]))
                    ++i;
            }
            Token t = finish(TokenKind::Identifier);
            if (t.text.find(U'.') == std::u32string::npos && isKeyword(t.upper))
                t.kind = TokenKind::Keyword;
            out.push_back(std::move(t));
            continue;
        }
        if (c == 0x2202 || c == 0x222B || c == 0x221E) { // ∂ ∫ ∞
            ++i;
            out.push_back(finish(TokenKind::Identifier));
            continue;
        }

        // Operators
        static const char32_t *multi[] = {U":=", U"==", U"<>", U"<=", U">=", U".*", U"./", U".^"};
        bool matched = false;
        for (const char32_t *op : multi) {
            if (c == op[0] && at(i + 1) == op[1]) {
                i += 2;
                out.push_back(finish(TokenKind::Operator));
                matched = true;
                break;
            }
        }
        if (matched)
            continue;

        switch (c) {
        case U'(': case U')': case U'{': case U'}': case U'[': case U']': case U',': case U';':
            ++i;
            out.push_back(finish(TokenKind::Punct));
            continue;
        case U'+': case U'-': case U'*': case U'/': case U'^': case U'<': case U'>': case U'=':
        case U'!': case U'|': case U'%': case U'&':
        case 0x2260: /* ≠ */ case 0x2264: /* ≤ */ case 0x2265: /* ≥ */ case 0x25B6: /* ▶ */
        case 0x2212: /* − */ case 0x00D7: /* × */ case 0x00F7: /* ÷ */ case 0x221A: /* √ */
        case 0x00B0: /* ° */ case 0x2032: /* ′ */ case 0x2033: /* ″ */ case 0x00B2: /* ² */
        case 0x00B3: /* ³ */ case 0x2192: /* → */ case 0x2071: /* ⁱ */ case 0x207B: /* ⁻ */ case 0x00B9: /* ¹ */
            ++i;
            out.push_back(finish(TokenKind::Operator));
            continue;
        default:
            break;
        }

        // Anything else
        ++i;
        out.push_back(finish(TokenKind::Invalid));
        if (c == U':')
            diag(Severity::Error, start, 1, tline, tcol, "Samotny dwukropek. Czy chodziło o przypisanie ':='?", "stray-colon");
        else if (c == U'.')
            diag(Severity::Error, start, 1, tline, tcol, "Nieoczekiwana kropka.", "stray-dot");
        else
            diag(Severity::Error, start, 1, tline, tcol, "Nieznany znak '" + toUtf8(c) + "'.", "unknown-char");
        (void)kind;
    }

    Token eof;
    eof.kind = TokenKind::EndOfFile;
    eof.offset = n;
    eof.line = line;
    eof.column = n - lineStart + 1;
    eof.endLine = line;
    out.push_back(eof);
    return out;
}

} // namespace ppl
