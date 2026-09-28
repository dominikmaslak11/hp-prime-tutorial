#pragma once

#include "analyzer.h"
#include "commanddb.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ppl {

// Text with line index. Lines and columns are 0-based and counted in code points.
class TextDocument {
public:
    TextDocument() = default;
    explicit TextDocument(std::u32string text);

    const std::u32string &text() const { return m_text; }
    int lineCount() const { return static_cast<int>(m_lineStarts.size()); }
    int offsetAt(int line, int column) const;
    std::pair<int, int> positionAt(int offset) const;
    std::u32string lineText(int line) const;
    // Conversions between code point columns and UTF-16 columns (LSP default encoding).
    int utf16Column(int line, int column) const;
    int columnFromUtf16(int line, int utf16) const;

private:
    std::u32string m_text;
    std::vector<int> m_lineStarts;
};

// LSP CompletionItemKind values
enum class ItemKind { Method = 2, Function = 3, Field = 5, Variable = 6, Module = 9, Keyword = 14, Snippet = 15, Constant = 21 };

struct CompletionItem {
    std::string label;
    ItemKind kind = ItemKind::Keyword;
    std::string detail;
    std::string documentation;   // markdown
    std::string insertText;
    bool snippet = false;
    bool triggerSignatureHelp = false;
    std::string sortText;
    std::string filterText;
};

struct SignatureInfo {
    std::string label;
    std::vector<std::pair<int, int>> parameters; // [start, end) offsets in label (UTF-16 units)
    std::string documentation;
    int activeParameter = 0;
};

struct Snippet {
    std::string label;
    std::string prefix;
    std::string body;          // VS Code / LSP snippet syntax, \t = one indent
    std::string description;
};

namespace Services {

std::vector<CompletionItem> completions(const TextDocument &doc, const AnalysisResult &analysis, int offset,
                                        const CommandDatabase &db = CommandDatabase::instance());

// Markdown for the word at offset; start/length receive the word range (code points).
std::optional<std::string> hover(const TextDocument &doc, const AnalysisResult &analysis, int offset, int *start = nullptr,
                                 int *length = nullptr, const CommandDatabase &db = CommandDatabase::instance());

std::optional<SignatureInfo> signatureHelp(const TextDocument &doc, const AnalysisResult &analysis, int offset,
                                           const CommandDatabase &db = CommandDatabase::instance());

// Offset of the declaration of the symbol at offset.
std::optional<int> definition(const TextDocument &doc, const AnalysisResult &analysis, int offset);

std::string commandMarkdown(const CommandInfo &info);
std::string variableMarkdown(const std::string &name, const VariableGroup &group);

// Parameters of a syntax string such as "INPUT(zm, [\"tytuł\"])".
std::vector<std::string> syntaxParameters(const std::string &syntax);

const std::vector<Snippet> &snippets();

// Markdown guide to PPL for AI assistants.
std::string languageGuide(const CommandDatabase &db = CommandDatabase::instance());

// Commands whose name, category or description contains the query (case-insensitive).
std::vector<const CommandInfo *> searchCommands(const std::string &query, const CommandDatabase &db = CommandDatabase::instance());

} // namespace Services
} // namespace ppl
