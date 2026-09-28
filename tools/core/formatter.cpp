#include "formatter.h"

#include "lexer.h"
#include "utf8.h"

#include <algorithm>

namespace ppl {
namespace Formatter {

namespace {

enum class Open { Block, Then, Iferr };

std::vector<std::u32string> splitLines(const std::u32string &text)
{
    std::vector<std::u32string> lines;
    std::u32string cur;
    for (char32_t c : text) {
        if (c == U'\n') {
            if (!cur.empty() && cur.back() == U'\r')
                cur.pop_back();
            lines.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty() && cur.back() == U'\r')
        cur.pop_back();
    lines.push_back(cur);
    return lines;
}

int lineCount(const std::u32string &text)
{
    return static_cast<int>(std::count(text.begin(), text.end(), U'\n')) + 1;
}

struct Walker {
    std::vector<Open> stack;
    int pendingIf = 0;

    // Depth for a line whose first token is tok (before applying tok).
    int lineDepth(const Token &tok) const
    {
        int depth = static_cast<int>(stack.size());
        if (tok.kind == TokenKind::Keyword) {
            const auto &u = tok.upper;
            if (u == U"END" || u == U"UNTIL" || u == U"ELSE")
                return std::max(0, depth - 1);
            if (u == U"THEN" && pendingIf == 0 && !stack.empty() && stack.back() == Open::Iferr)
                return std::max(0, depth - 1);
        }
        return depth;
    }

    void pop()
    {
        if (!stack.empty())
            stack.pop_back();
    }

    void apply(const Token &tok)
    {
        if (tok.kind != TokenKind::Keyword)
            return;
        const auto &u = tok.upper;
        if (u == U"BEGIN" || u == U"REPEAT" || u == U"CASE" || u == U"DO") {
            stack.push_back(Open::Block);
        } else if (u == U"IFERR") {
            stack.push_back(Open::Iferr);
        } else if (u == U"IF") {
            ++pendingIf;
        } else if (u == U"THEN") {
            if (pendingIf > 0) {
                --pendingIf;
            } else {
                pop(); // THEN of IFERR
            }
            stack.push_back(Open::Then);
        } else if (u == U"ELSE") {
            pop();
            stack.push_back(Open::Block);
        } else if (u == U"END" || u == U"UNTIL") {
            pop();
        }
    }
};

} // namespace

std::vector<int> indentLevels(const std::u32string &text)
{
    const int lines = lineCount(text);
    std::vector<int> levels(lines, -2); // -2 = no token yet
    std::vector<int> depthAfter(lines, -1);
    std::vector<Token> tokens = Lexer::tokenize(text, nullptr, true);
    Walker w;
    for (const Token &tok : tokens) {
        if (tok.isEof())
            break;
        int li = tok.line - 1;
        if (li >= 0 && li < lines && levels[li] == -2)
            levels[li] = w.lineDepth(tok);
        // lines inside multi-line tokens must not be touched
        for (int k = tok.line; k < tok.endLine && k < lines; ++k)
            levels[k] = -1;
        if (tok.kind != TokenKind::Comment)
            w.apply(tok);
        int le = tok.endLine - 1;
        if (le >= 0 && le < lines)
            depthAfter[le] = static_cast<int>(w.stack.size());
    }
    int running = 0;
    for (int i = 0; i < lines; ++i) {
        if (levels[i] == -2)
            levels[i] = running; // blank line
        if (depthAfter[i] >= 0)
            running = depthAfter[i];
    }
    return levels;
}

std::u32string format(const std::u32string &text, const FormatOptions &options)
{
    std::vector<std::u32string> lines = splitLines(text);
    std::vector<int> levels = indentLevels(text);

    // keyword case normalisation (outside strings/comments)
    std::u32string source = text;
    if (options.uppercaseKeywords) {
        std::vector<Token> tokens = Lexer::tokenize(text, nullptr, true);
        for (const Token &tok : tokens)
            if (tok.kind == TokenKind::Keyword && tok.text != tok.upper)
                source.replace(tok.offset, tok.length, tok.upper);
        lines = splitLines(source);
    }

    std::u32string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::u32string line = lines[i];
        int level = i < levels.size() ? levels[i] : 0;
        if (level >= 0) {
            size_t first = 0;
            while (first < line.size() && (line[first] == U' ' || line[first] == U'\t'))
                ++first;
            line = line.substr(first);
            while (!line.empty() && (line.back() == U' ' || line.back() == U'\t'))
                line.pop_back();
            if (!line.empty())
                line = std::u32string(static_cast<size_t>(level * options.indentSize), U' ') + line;
        }
        out += line;
        if (i + 1 < lines.size())
            out += U'\n';
    }
    return out;
}

int indentForNewLine(const std::u32string &text, int line)
{
    // Depth after all tokens on lines 1..line.
    std::vector<Token> tokens = Lexer::tokenize(text, nullptr, false);
    Walker w;
    for (const Token &tok : tokens) {
        if (tok.isEof() || tok.line > line)
            break;
        w.apply(tok);
    }
    return static_cast<int>(w.stack.size());
}

namespace {

std::u32string replaceOperators(const std::u32string &text, bool toSymbols)
{
    std::vector<Token> tokens = Lexer::tokenize(text, nullptr, true);
    std::u32string out;
    int last = 0;
    for (const Token &tok : tokens) {
        if (tok.isEof())
            break;
        if (tok.kind != TokenKind::Operator)
            continue;
        std::u32string repl;
        if (toSymbols) {
            if (tok.text == U"<>") repl = U"≠";
            else if (tok.text == U"<=") repl = U"≤";
            else if (tok.text == U">=") repl = U"≥";
        } else {
            if (tok.text == U"≠") repl = U"<>";
            else if (tok.text == U"≤") repl = U"<=";
            else if (tok.text == U"≥") repl = U">=";
            else if (tok.text == U"−") repl = U"-";
            else if (tok.text == U"×") repl = U"*";
            else if (tok.text == U"÷") repl = U"/";
        }
        if (repl.empty())
            continue;
        out += text.substr(last, tok.offset - last);
        out += repl;
        last = tok.offset + tok.length;
    }
    out += text.substr(last);
    return out;
}

} // namespace

std::u32string toCalculatorSymbols(const std::u32string &text) { return replaceOperators(text, true); }
std::u32string toAscii(const std::u32string &text) { return replaceOperators(text, false); }

} // namespace Formatter
} // namespace ppl
