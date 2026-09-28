#include "services.h"

#include "lexer.h"
#include "utf8.h"

#include <algorithm>
#include <map>
#include <set>

namespace ppl {

// ------------------------------------------------------------------ TextDocument

TextDocument::TextDocument(std::u32string text) : m_text(std::move(text))
{
    m_lineStarts.push_back(0);
    for (size_t i = 0; i < m_text.size(); ++i)
        if (m_text[i] == U'\n')
            m_lineStarts.push_back(static_cast<int>(i) + 1);
}

int TextDocument::offsetAt(int line, int column) const
{
    if (line < 0)
        return 0;
    if (line >= lineCount())
        return static_cast<int>(m_text.size());
    int start = m_lineStarts[line];
    int end = line + 1 < lineCount() ? m_lineStarts[line + 1] - 1 : static_cast<int>(m_text.size());
    return std::clamp(start + std::max(0, column), start, end);
}

std::pair<int, int> TextDocument::positionAt(int offset) const
{
    offset = std::clamp(offset, 0, static_cast<int>(m_text.size()));
    auto it = std::upper_bound(m_lineStarts.begin(), m_lineStarts.end(), offset);
    int line = static_cast<int>(it - m_lineStarts.begin()) - 1;
    return {line, offset - m_lineStarts[line]};
}

std::u32string TextDocument::lineText(int line) const
{
    if (line < 0 || line >= lineCount())
        return {};
    int start = m_lineStarts[line];
    int end = line + 1 < lineCount() ? m_lineStarts[line + 1] - 1 : static_cast<int>(m_text.size());
    std::u32string s = m_text.substr(start, end - start);
    if (!s.empty() && s.back() == U'\r')
        s.pop_back();
    return s;
}

int TextDocument::utf16Column(int line, int column) const
{
    std::u32string l = lineText(line);
    column = std::clamp(column, 0, static_cast<int>(l.size()));
    return utf16Length(std::u32string_view(l).substr(0, column));
}

int TextDocument::columnFromUtf16(int line, int utf16) const
{
    std::u32string l = lineText(line);
    int units = 0;
    for (int i = 0; i < static_cast<int>(l.size()); ++i) {
        if (units >= utf16)
            return i;
        units += l[i] > 0xFFFF ? 2 : 1;
    }
    return static_cast<int>(l.size());
}

namespace Services {

namespace {

bool wordChar(char32_t c) { return Lexer::isIdentifierChar(c) || c == U'.' || c == 0x2202 || c == 0x222B; }

// Identifier range around offset (may include dots of a qualified name).
std::pair<int, int> wordRange(const std::u32string &text, int offset)
{
    int n = static_cast<int>(text.size());
    int s = std::clamp(offset, 0, n);
    int e = s;
    while (s > 0 && wordChar(text[s - 1]))
        --s;
    while (e < n && wordChar(text[e]))
        ++e;
    // do not start with a digit (numbers are not identifiers)
    while (s < e && (isDigit(text[s]) || text[s] == U'.'))
        ++s;
    return {s, e - s};
}

// Kind of token at offset: used to disable completion in strings and comments.
bool insideStringOrComment(const std::u32string &text, int offset)
{
    std::vector<Token> tokens = Lexer::tokenize(text, nullptr, true);
    for (const Token &t : tokens) {
        if (t.isEof())
            break;
        if (t.offset >= offset)
            break;
        bool open = t.kind == TokenKind::String || t.kind == TokenKind::Comment || t.kind == TokenKind::CasBlock;
        if (open && offset > t.offset && offset < t.offset + t.length)
            return true;
        if (t.kind == TokenKind::Comment && offset == t.offset + t.length)
            return true; // cursor at the end of a comment
        if (t.kind == TokenKind::String && offset == t.offset + t.length && (t.length < 2 || t.text.back() != U'"'))
            return true; // unterminated string
    }
    return false;
}

std::string escapeSnippet(const std::string &s)
{
    std::string out;
    for (char c : s) {
        if (c == '$' || c == '}' || c == '\\')
            out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

std::string lower(std::string s)
{
    for (auto &c : s)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return s;
}

} // namespace

std::vector<std::string> syntaxParameters(const std::string &syntax)
{
    std::vector<std::string> params;
    size_t open = syntax.find('(');
    if (open == std::string::npos)
        return params;
    int depth = 0;
    bool inString = false;
    std::string cur;
    for (size_t i = open + 1; i < syntax.size(); ++i) {
        char c = syntax[i];
        if (c == '"')
            inString = !inString;
        if (!inString) {
            if (c == '(' || c == '[' || c == '{')
                ++depth;
            else if (c == ']' || c == '}')
                --depth;
            else if (c == ')') {
                if (depth == 0) {
                    break;
                }
                --depth;
            } else if (c == ',' && depth == 0) {
                params.push_back(cur);
                cur.clear();
                continue;
            }
        }
        cur.push_back(c);
    }
    params.push_back(cur);
    for (auto &p : params) {
        size_t b = p.find_first_not_of(' ');
        size_t e = p.find_last_not_of(' ');
        p = b == std::string::npos ? std::string() : p.substr(b, e - b + 1);
    }
    if (params.size() == 1 && params[0].empty())
        params.clear();
    return params;
}

std::string commandMarkdown(const CommandInfo &c)
{
    std::string md = "```ppl\n" + c.syntax + "\n```\n\n" + c.description + "\n";
    if (!c.example.empty())
        md += "\n**Przykład:**\n```ppl\n" + c.example + "\n```\n";
    std::string meta = "*" + c.category + "*";
    if (c.kind == "cas")
        meta += " · funkcja CAS (w programie wywołuj z prefiksem `CAS.`)";
    if (c.hasArgInfo()) {
        if (c.maxArgs < 0)
            meta += " · argumenty: co najmniej " + std::to_string(c.minArgs);
        else if (c.minArgs == c.maxArgs)
            meta += " · argumenty: " + std::to_string(c.minArgs);
        else
            meta += " · argumenty: " + std::to_string(c.minArgs) + "–" + std::to_string(c.maxArgs);
    }
    md += "\n" + meta;
    std::string url = CommandDatabase::chapterUrl(c.chapter);
    if (!url.empty())
        md += " · [rozdział kursu](" + url + ")";
    return md;
}

std::string variableMarkdown(const std::string &name, const VariableGroup &g)
{
    std::string md = "**" + name + "** — " + g.group + "\n\n" + g.description;
    std::string url = CommandDatabase::chapterUrl(g.chapter);
    if (!url.empty())
        md += "\n\n[rozdział kursu](" + url + ")";
    return md;
}

const std::vector<Snippet> &snippets()
{
    static const std::vector<Snippet> list = {
        {"EXPORT … BEGIN … END (program)", "EXPORT", "EXPORT ${1:NAZWA}(${2})\nBEGIN\n\t${0}\nEND;", "Nowa funkcja eksportowana (program)."},
        {"IF … THEN … END", "IF", "IF ${1:warunek} THEN\n\t${0}\nEND;", "Instrukcja warunkowa."},
        {"IF … THEN … ELSE … END", "IFELSE", "IF ${1:warunek} THEN\n\t${2}\nELSE\n\t${0}\nEND;", "Instrukcja warunkowa z gałęzią ELSE."},
        {"CASE … DEFAULT … END", "CASE", "CASE\n\tIF ${1:warunek1} THEN ${2} END;\n\tIF ${3:warunek2} THEN ${4} END;\n\tDEFAULT ${0}\nEND;", "Wybór spośród wielu przypadków."},
        {"IFERR … THEN … END", "IFERR", "IFERR\n\t${1}\nTHEN\n\t${0}\nEND;", "Przechwytywanie błędów."},
        {"FOR … FROM … TO … DO", "FOR", "FOR ${1:i} FROM ${2:1} TO ${3:n} DO\n\t${0}\nEND;", "Pętla licząca."},
        {"FOR … STEP", "FORSTEP", "FOR ${1:i} FROM ${2:1} TO ${3:n} STEP ${4:2} DO\n\t${0}\nEND;", "Pętla z krokiem."},
        {"FOR … DOWNTO", "FORDOWN", "FOR ${1:i} FROM ${2:10} DOWNTO ${3:1} DO\n\t${0}\nEND;", "Pętla licząca w dół."},
        {"WHILE … DO … END", "WHILE", "WHILE ${1:warunek} DO\n\t${0}\nEND;", "Pętla z warunkiem na początku."},
        {"REPEAT … UNTIL", "REPEAT", "REPEAT\n\t${0}\nUNTIL ${1:warunek};", "Pętla z warunkiem na końcu."},
        {"Podprogram z deklaracją", "SUB", "${1:POMOC}();\n\nEXPORT ${2:GLOWNA}()\nBEGIN\n\tRETURN ${1:POMOC}(${3:1});\nEND;\n\n${1:POMOC}(x)\nBEGIN\n\tRETURN ${0:x^2};\nEND;", "Program z prywatną funkcją pomocniczą."},
        {"INPUT z obsługą Cancel", "INPUTOK", "IF NOT INPUT({${1:a},${2:b}}, \"${3:Tytuł}\", {\"${1:a}=\",\"${2:b}=\"}) THEN\n\tRETURN \"anulowano\";\nEND;\n${0}", "Formularz z obsługą przycisku Cancel."},
        {"CHOOSE menu w pętli", "MENU", "LOCAL w;\nREPEAT\n\tCHOOSE(w, \"${1:Menu}\", {\"${2:Opcja 1}\",\"${3:Opcja 2}\",\"Koniec\"});\n\tCASE\n\t\tIF w==1 THEN ${4} END;\n\t\tIF w==2 THEN ${5} END;\n\tEND;\nUNTIL w==0 OR w==3;", "Menu programu w pętli."},
        {"Czekaj na klawisz", "WAITKEY", "REPEAT\n\t${1:k} := GETKEY;\nUNTIL ${1:k} <> -1;", "Pętla oczekiwania na naciśnięcie klawisza."},
        {"Pętla animacji (bufor G1)", "ANIM", "DIMGROB_P(G1, 320, 240);\nREPEAT\n\tRECT_P(G1, 0, 0, 319, 239, #FFFFFFh, #FFFFFFh);\n\t${0}\n\tBLIT_P(G0, G1);\n\tWAIT(0.02);\nUNTIL GETKEY == 4;", "Animacja bez migotania; [Esc] kończy."},
        {"Zachowaj i przywróć HAngle", "HANGLE", "LOCAL stary := HAngle;\nHAngle := ${1:1};\n${0}\nHAngle := stary;", "Zmiana trybu kątów z przywróceniem ustawienia użytkownika."},
        {"VIEW (pozycja menu aplikacji)", "VIEW", "VIEW \"${1:Tekst}\", ${2:FUNKCJA}()\nBEGIN\n\t${0}\n\tSTARTVIEW(7,1);\nEND;", "Pozycja menu View własnej aplikacji."},
        {"KEY (klawisz użytkownika)", "KEY", "KEY ${1:K_Sin}()\nBEGIN\n\tRETURN \"${0}\";\nEND;", "Przeprogramowanie klawisza."},
        {"Obsługa dotyku (MOUSE)", "TOUCH", "LOCAL m := MOUSE;\nIF SIZE(m(1)) > 0 THEN\n\t${1:x} := m(1,1);\n\t${2:y} := m(1,2);\n\t${0}\nEND;", "Odczyt pierwszego punktu dotyku."}};
    return list;
}

std::vector<CompletionItem> completions(const TextDocument &doc, const AnalysisResult &analysis, int offset,
                                        const CommandDatabase &db)
{
    std::vector<CompletionItem> items;
    const std::u32string &text = doc.text();
    offset = std::clamp(offset, 0, static_cast<int>(text.size()));
    if (insideStringOrComment(text, offset))
        return items;

    std::set<std::string> seen;
    auto add = [&](CompletionItem it) {
        if (seen.insert(it.label).second)
            items.push_back(std::move(it));
    };

    // Locals and parameters of the function at the cursor
    int fi = analysis.functionAt(offset);
    for (const auto &l : analysis.locals) {
        if (l.functionIndex != fi || l.offset > offset)
            continue;
        CompletionItem it;
        it.label = l.name;
        it.kind = ItemKind::Variable;
        it.detail = l.isParam ? "parametr" : "zmienna lokalna";
        it.insertText = l.name;
        it.sortText = "0" + l.name;
        add(it);
    }
    for (const auto &f : analysis.functions) {
        if (!f.defined && f.declOffset < 0)
            continue;
        CompletionItem it;
        it.label = f.name;
        it.kind = ItemKind::Function;
        std::string params;
        std::string snippetParams;
        for (size_t k = 0; k < f.params.size(); ++k) {
            params += (k ? ", " : "") + f.params[k];
            snippetParams += (k ? ", " : "") + std::string("${") + std::to_string(k + 1) + ":" + escapeSnippet(f.params[k]) + "}";
        }
        it.detail = (f.exported ? "EXPORT " : "") + f.name + "(" + params + ")";
        it.documentation = f.isView ? "Pozycja menu View: " + f.viewTitle : "Funkcja z tego pliku (linia " + std::to_string(f.line) + ").";
        it.insertText = f.name + "(" + snippetParams + ")$0";
        it.snippet = true;
        it.triggerSignatureHelp = !f.params.empty();
        it.sortText = "1" + f.name;
        add(it);
    }
    for (const auto &v : analysis.fileVariables) {
        CompletionItem it;
        it.label = v.name;
        it.kind = ItemKind::Variable;
        it.detail = v.exported ? "zmienna eksportowana" : "zmienna pliku";
        it.insertText = v.name;
        it.sortText = "1" + v.name;
        add(it);
    }

    // Key names after KEY
    std::u32string line = doc.lineText(doc.positionAt(offset).first);
    std::string lineUtf8 = asciiUpper(toUtf8(line));
    bool keyLine = lineUtf8.find("KEY ") != std::string::npos;
    if (keyLine) {
        for (const auto &k : db.allKeyNames()) {
            CompletionItem it;
            it.label = k;
            it.kind = ItemKind::Constant;
            it.detail = "nazwa klawisza";
            it.insertText = k + "()";
            it.sortText = "0" + k;
            add(it);
        }
    }

    for (const auto &s : snippets()) {
        CompletionItem it;
        it.label = s.label;
        it.filterText = s.prefix;
        it.kind = ItemKind::Snippet;
        it.detail = "szablon";
        it.documentation = s.description + "\n\n```ppl\n" + s.body + "\n```";
        it.insertText = s.body;
        it.snippet = true;
        it.sortText = "2" + s.prefix;
        add(it);
    }

    for (const auto &c : db.commands()) {
        CompletionItem it;
        it.label = c.kind == "cas" ? "CAS." + c.name : c.name;
        it.documentation = commandMarkdown(c);
        it.detail = c.syntax;
        if (c.kind == "keyword") {
            it.kind = ItemKind::Keyword;
            it.insertText = c.name;
            it.sortText = "3" + c.name;
        } else {
            it.kind = c.kind == "appfunc" ? ItemKind::Method : ItemKind::Function;
            bool noArgs = c.maxArgs == 0 || c.syntax.find('(') == std::string::npos;
            if (noArgs) {
                it.insertText = it.label;
            } else {
                it.insertText = escapeSnippet(it.label) + "($1)$0";
                it.snippet = true;
                it.triggerSignatureHelp = true;
            }
            it.sortText = "4" + c.name;
        }
        add(it);
    }

    for (const auto &g : db.variableGroups()) {
        bool constants = g.group == "Stałe";
        for (const auto &n : g.names) {
            CompletionItem it;
            it.label = n;
            it.kind = constants ? ItemKind::Constant : ItemKind::Field;
            it.detail = g.group;
            it.documentation = variableMarkdown(n, g);
            it.insertText = n;
            it.sortText = "5" + n;
            add(it);
        }
    }
    return items;
}

std::optional<std::string> hover(const TextDocument &doc, const AnalysisResult &analysis, int offset, int *start,
                                 int *length, const CommandDatabase &db)
{
    const std::u32string &text = doc.text();
    auto [s, len] = wordRange(text, offset);
    if (len <= 0)
        return std::nullopt;
    if (start)
        *start = s;
    if (length)
        *length = len;
    std::u32string word = text.substr(s, len);

    // qualified names: CAS.idivis, Function.ROOT
    size_t dot = word.rfind(U'.');
    if (dot != std::u32string::npos) {
        std::u32string last = word.substr(dot + 1);
        if (const CommandInfo *c = db.findCommand(last))
            return commandMarkdown(*c);
        if (const VariableGroup *g = db.findVariable(last))
            return variableMarkdown(toUtf8(last), *g);
        return "**" + toUtf8(word) + "** — nazwa kwalifikowana (aplikacja lub program `" + toUtf8(word.substr(0, dot)) + "`).";
    }

    int fi = analysis.functionAt(s);
    for (const auto &l : analysis.locals) {
        if (l.functionIndex == fi && l.name32 == word) {
            std::string fname = fi >= 0 ? analysis.functions[fi].name : "";
            return "**" + l.name + "** — " + (l.isParam ? "parametr" : "zmienna lokalna") + " funkcji " + fname
                   + " (linia " + std::to_string(l.line) + ")";
        }
    }
    for (const auto &f : analysis.functions) {
        if (f.name32 == word) {
            std::string params;
            for (size_t k = 0; k < f.params.size(); ++k)
                params += (k ? ", " : "") + f.params[k];
            std::string head = f.isKey ? "KEY " : f.isView ? "VIEW \"" + f.viewTitle + "\", " : f.exported ? "EXPORT " : "";
            return "```ppl\n" + head + f.name + "(" + params + ")\n```\nFunkcja z tego pliku, linia " + std::to_string(f.line)
                   + (f.defined ? "." : " (tylko deklaracja — definicja w innym programie).");
        }
    }
    for (const auto &v : analysis.fileVariables)
        if (v.name32 == word)
            return "**" + v.name + "** — " + (v.exported ? "zmienna eksportowana (globalna)" : "zmienna wspólna dla pliku")
                   + ", linia " + std::to_string(v.line);
    if (const CommandInfo *c = db.findCommand(word))
        return commandMarkdown(*c);
    if (const VariableGroup *g = db.findVariable(word))
        return variableMarkdown(toUtf8(word), *g);
    return std::nullopt;
}

std::optional<SignatureInfo> signatureHelp(const TextDocument &doc, const AnalysisResult &analysis, int offset,
                                           const CommandDatabase &db)
{
    const std::u32string &text = doc.text();
    std::vector<Token> tokens = Lexer::tokenize(text, nullptr, false);
    // tokens before the cursor
    int last = -1;
    for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
        if (tokens[i].isEof() || tokens[i].offset >= offset)
            break;
        last = i;
    }
    int depth = 0;
    int commas = 0;
    int nameIndex = -1;
    for (int i = last; i >= 0; --i) {
        const Token &t = tokens[i];
        if (t.isPunct(U')') || t.isPunct(U']') || t.isPunct(U'}')) {
            ++depth;
        } else if (t.isPunct(U'(') || t.isPunct(U'[') || t.isPunct(U'{')) {
            if (depth == 0) {
                if (t.isPunct(U'(') && i > 0 && tokens[i - 1].kind == TokenKind::Identifier)
                    nameIndex = i - 1;
                break;
            }
            --depth;
        } else if (t.isPunct(U',') && depth == 0) {
            ++commas;
        } else if (t.isPunct(U';') || (t.kind == TokenKind::Keyword && (t.upper == U"BEGIN" || t.upper == U"END"))) {
            break;
        }
    }
    if (nameIndex < 0)
        return std::nullopt;
    std::u32string name = tokens[nameIndex].text;
    if (name.rfind(U"CAS.", 0) == 0)
        name = name.substr(4);
    else if (size_t dot = name.rfind(U'.'); dot != std::u32string::npos)
        name = name.substr(dot + 1);

    SignatureInfo sig;
    std::vector<std::string> params;
    bool found = false;
    for (const auto &f : analysis.functions) {
        if (f.name32 == name) {
            sig.label = f.name + "(";
            for (size_t k = 0; k < f.params.size(); ++k) {
                if (k)
                    sig.label += ", ";
                int st = utf16Length(toU32(sig.label));
                sig.label += f.params[k];
                sig.parameters.push_back({st, utf16Length(toU32(sig.label))});
            }
            sig.label += ")";
            sig.documentation = "Funkcja z tego pliku (linia " + std::to_string(f.line) + ").";
            found = true;
            break;
        }
    }
    if (!found) {
        const CommandInfo *c = db.findCommand(name);
        if (!c || c->kind == "keyword")
            return std::nullopt;
        sig.label = c->syntax;
        sig.documentation = c->description;
        params = syntaxParameters(c->syntax);
        // locate each parameter inside the label
        std::u32string label32 = toU32(sig.label);
        size_t searchFrom = label32.find(U'(');
        if (searchFrom == std::u32string::npos)
            searchFrom = 0;
        for (const auto &prm : params) {
            std::u32string p32 = toU32(prm);
            size_t pos = label32.find(p32, searchFrom);
            if (pos == std::u32string::npos)
                break;
            int st = utf16Length(std::u32string_view(label32).substr(0, pos));
            int en = st + utf16Length(p32);
            sig.parameters.push_back({st, en});
            searchFrom = pos + p32.size();
        }
    }
    int count = static_cast<int>(sig.parameters.size());
    sig.activeParameter = count == 0 ? 0 : std::min(commas, count - 1);
    return sig;
}

std::optional<int> definition(const TextDocument &doc, const AnalysisResult &analysis, int offset)
{
    auto [s, len] = wordRange(doc.text(), offset);
    if (len <= 0)
        return std::nullopt;
    std::u32string word = doc.text().substr(s, len);
    int fi = analysis.functionAt(s);
    for (const auto &l : analysis.locals)
        if (l.functionIndex == fi && l.name32 == word)
            return l.offset;
    for (const auto &f : analysis.functions)
        if (f.name32 == word && f.offset >= 0)
            return f.offset;
    for (const auto &v : analysis.fileVariables)
        if (v.name32 == word)
            return v.offset;
    return std::nullopt;
}

std::vector<const CommandInfo *> searchCommands(const std::string &query, const CommandDatabase &db)
{
    std::vector<const CommandInfo *> out;
    std::string q = lower(query);
    for (const auto &c : db.commands()) {
        std::string hay = lower(c.name + " " + c.category + " " + c.description + " " + c.syntax);
        if (q.empty() || hay.find(q) != std::string::npos)
            out.push_back(&c);
    }
    // exact name matches first
    std::stable_sort(out.begin(), out.end(), [&](const CommandInfo *a, const CommandInfo *b) {
        return (lower(a->name) == q) > (lower(b->name) == q);
    });
    return out;
}

std::string languageGuide(const CommandDatabase &db)
{
    std::string g = R"GUIDE(# HP PPL — przewodnik dla asystenta AI

HP PPL (HP Prime Programming Language) to język kalkulatora graficznego HP Prime, podobny do Pascala.
Kod sprawdzaj narzędziem `ppl_validate` przed pokazaniem go użytkownikowi. Nie wymyślaj komend: jeśli nie masz pewności, użyj `ppl_search_commands` / `ppl_command_help`.

## Struktura programu
```ppl
POMOC();                 // deklaracja zapowiadająca funkcji zdefiniowanej niżej

EXPORT GLOWNA(a, b)      // EXPORT = funkcja widoczna w Home i w menu User
BEGIN
  LOCAL x := 0, lista := {};
  x := POMOC(a) + b;
  RETURN x;
END;

POMOC(n)                 // funkcja prywatna pliku
BEGIN
  RETURN n^2;
END;
```

## Najważniejsze zasady
- Każde polecenie kończy się średnikiem `;`, także `END;` i `UNTIL warunek;`.
- Przypisanie `:=` (albo `wartość ▶ zmienna`). Porównanie `==`. Różne `<>` (lub `≠`), `<=`, `>=`. Operatory logiczne `AND OR XOR NOT`, reszta `MOD`.
- Bloki: `IF w THEN … [ELSE …] END;` · `CASE IF w THEN … END; … DEFAULT … END;` · `IFERR … THEN … [ELSE …] END;`
  · `FOR i FROM a TO b [STEP k] DO … END;` · `FOR i FROM a DOWNTO b DO … END;` · `WHILE w DO … END;` · `REPEAT … UNTIL w;` · `BREAK;` `CONTINUE;`
- Nie ma `ELSEIF` — zagnieżdżaj `IF` albo używaj `CASE`. Wyrażenie warunkowe: `when(w, a, b)`.
- Funkcję trzeba zadeklarować przed użyciem (`NAZWA();` na początku pliku) albo zdefiniować wyżej.
- Zmienne robocze deklaruj przez `LOCAL` (domyślna wartość 0). Wielkość liter ma znaczenie.
- Zmienne systemowe mają stały typ: `A`–`Z` liczby rzeczywiste, `Z0`–`Z9` zespolone, `L0`–`L9` listy, `M0`–`M9` macierze, `G0`–`G9` grafika (G0 = ekran). Nie nadpisuj ich bez potrzeby.
- Funkcje CAS wywołuj z prefiksem: `CAS.idivis(12)`, `CAS.isprime(n)`.
- Listy `{1,2,3}` (indeks od 1: `L(1)`), macierze `[[1,2],[3,4]]`, teksty `"…"` (łączenie `+`, znak `"` jako `""`).
- Kolory: `RGB(r,g,b)` albo `#RRGGBBh`. Liczby całkowite w systemach: `#FFh`, `#1101b`.
- Komentarze: `// …`.

## Wejście i wyjście
- `RETURN w;` zwraca wynik. `MSGBOX(tekst[, 1])` okno (zwraca 1 przy OK, 0 przy Cancel). `PRINT(x)` terminal, `PRINT()` czyści.
- `INPUT(zm, "tytuł", "etykieta", "pomoc")` lub z listami: `INPUT({a,b}, "Tytuł", {"a=","b="})`. Zwraca 0 przy Cancel — obsługuj to.
- `CHOOSE(zm, "tytuł", "op1", "op2")` → numer opcji albo 0.
- `GETKEY` zwraca kod klawisza 0–50 albo -1 (nie czeka). Pętla: `REPEAT k := GETKEY; UNTIL k <> -1;`. Kody: Esc 4, Enter 30, strzałki ▲2 ◀7 ▶8 ▼12, cyfry 0→47 1→42 2→43 3→44 4→37 5→38 6→39 7→32 8→33 9→34.
- `MOUSE` → `{{x,y,x0,y0,typ},{…}}`, pusta lista gdy brak dotyku.
- `TICKS` milisekundy, `WAIT(s)` pauza.

## Grafika
- Ekran 320×240 pikseli, (0,0) w lewym górnym rogu; pasek menu y≈220–239. Komendy z `_P` używają pikseli, bez `_P` współrzędnych kartezjańskich aplikacji.
- `RECT()` czyści ekran; `RECT_P(x1,y1,x2,y2,krawędź,wypełnienie)`, `LINE_P`, `PIXON_P`, `ARC_P(x,y,r,kolor)`, `FILLPOLY_P({(x,y),…},kolor)`, `TEXTOUT_P(tekst,x,y,czcionka,kolor)`.
- Na końcu programu graficznego `FREEZE;` albo pętla oczekiwania na klawisz.
- Animacja: rysuj w `G1` (`DIMGROB_P(G1,320,240)`), potem `BLIT_P(G0,G1)`.

## Aplikacje
- `STARTAPP("Function")`, `STARTVIEW(1,1)` (0 Symb, 1 Plot, 2 Num, -1 Home). Funkcje w aplikacji przypisuj jako tekst: `F1 := "SIN(X)"; CHECK(1);`
- Okno wykresu: `Xmin Xmax Ymin Ymax`. Tryb kątów: `HAngle` (0 rad, 1 stopnie) — zapamiętaj i przywróć ustawienie użytkownika.

## Komendy według kategorii
)GUIDE";
    std::map<std::string, std::vector<std::string>> byCat;
    for (const auto &c : db.commands())
        if (c.kind != "keyword")
            byCat[c.category].push_back(c.kind == "cas" ? "CAS." + c.name : c.name);
    for (const auto &[cat, names] : byCat) {
        g += "- **" + cat + "**: ";
        for (size_t i = 0; i < names.size(); ++i)
            g += (i ? ", " : "") + std::string("`") + names[i] + "`";
        g += "\n";
    }
    g += "\nPełny kurs po polsku: https://github.com/dominikmaslak11/hp-prime-tutorial\n";
    return g;
}

} // namespace Services
} // namespace ppl
