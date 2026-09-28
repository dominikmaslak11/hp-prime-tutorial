#include "generators.h"

#include "cli.h"
#include "commanddb.h"
#include "json.h"
#include "lexer.h"
#include "services.h"
#include "utf8.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace ppl::gen {

namespace {

std::string regexEscape(const std::string &s)
{
    std::string out;
    for (char c : s) {
        if (std::string("\\^$.|?*+()[]{}/%#&-").find(c) != std::string::npos)
            out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

std::string alternation(std::vector<std::string> names)
{
    // longest first so that e.g. TEXTOUT_P wins over TEXTOUT
    std::sort(names.begin(), names.end(), [](const std::string &a, const std::string &b) {
        return a.size() != b.size() ? a.size() > b.size() : a < b;
    });
    names.erase(std::unique(names.begin(), names.end()), names.end());
    std::string alt;
    for (const auto &n : names)
        alt += (alt.empty() ? "" : "|") + regexEscape(n);
    return alt;
}

// Word-boundary pattern that also works for names with → Σ ∂ and similar characters.
std::string wordPattern(const std::string &alt, bool ignoreCase)
{
    return std::string(ignoreCase ? "(?i)" : "") + "(?<![\\w→.])(?:" + alt + ")(?![\\w→])";
}

std::string xmlEscape(const std::string &s)
{
    std::string out;
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\n': out += "&#x0A;"; break;
        default: out.push_back(c);
        }
    }
    return out;
}

std::string join(const std::vector<std::string> &v, const char *sep = " ")
{
    std::string s;
    for (const auto &x : v)
        s += (s.empty() ? "" : sep) + x;
    return s;
}

// Names only known from HP's list: color those written in capitals (NORMALD_CDF), but not the
// lower-case CAS/geometry names (point, radius, angle…) that often collide with user variables.
bool colorCommand(const CommandInfo &c)
{
    return !c.fromHpList || std::none_of(c.name.begin(), c.name.end(), [](char ch) { return ch >= 'a' && ch <= 'z'; });
}

bool colorVariable(const VariableGroup &g, const std::string &n)
{
    return !g.fromHpList || n.empty() || !(n[0] >= 'a' && n[0] <= 'z');
}

} // namespace

std::string textMateGrammar()
{
    const auto &db = CommandDatabase::instance();
    std::vector<std::string> control, declaration, logical, commands, casFns, appFns, variables, constants;
    for (const auto &c : db.commands()) {
        if (!colorCommand(c))
            continue;
        if (c.kind == "keyword") {
            if (c.name == "AND" || c.name == "OR" || c.name == "XOR" || c.name == "NOT" || c.name == "MOD")
                logical.push_back(c.name);
            else if (c.name == "LOCAL" || c.name == "EXPORT" || c.name == "VIEW" || c.name == "KEY")
                declaration.push_back(c.name);
            else
                control.push_back(c.name);
        } else if (c.kind == "cas") {
            casFns.push_back(c.name);
        } else if (c.kind == "appfunc") {
            appFns.push_back(c.name);
        } else {
            commands.push_back(c.name);
        }
    }
    for (const auto &g : db.variableGroups())
        for (const auto &n : g.names)
            if (colorVariable(g, n))
                (g.group == "Stałe" ? constants : variables).push_back(n);

    auto match = [](const std::string &name, const std::string &re) {
        return Json::object({{"name", name}, {"match", re}});
    };

    Json patterns = Json::array();
    patterns.push(Json::object({{"include", "#comment"}}));
    patterns.push(Json::object({{"include", "#cas-block"}}));
    patterns.push(Json::object({{"include", "#pragma"}}));
    patterns.push(Json::object({{"include", "#string"}}));
    patterns.push(Json::object({{"include", "#function-definition"}}));
    patterns.push(Json::object({{"include", "#keywords"}}));
    patterns.push(Json::object({{"include", "#numbers"}}));
    patterns.push(Json::object({{"include", "#builtins"}}));
    patterns.push(Json::object({{"include", "#operators"}}));

    Json repo = Json::object();
    repo.set("comment", match("comment.line.double-slash.hpppl", "//.*$"));
    repo.set("pragma", match("meta.preprocessor.pragma.hpppl", "(?i)^\\s*#pragma\\b.*$"));
    repo.set("cas-block", Json::object({{"name", "meta.embedded.cas.hpppl"},
                                        {"begin", "(?i)^\\s*(#cas)\\b"},
                                        {"beginCaptures", Json::object({{"1", Json::object({{"name", "keyword.control.directive.hpppl"}})}})},
                                        {"end", "(?i)^\\s*(#end)\\b"},
                                        {"endCaptures", Json::object({{"1", Json::object({{"name", "keyword.control.directive.hpppl"}})}})},
                                        {"patterns", Json::array().push(Json::object({{"include", "#comment"}}))
                                                                  .push(Json::object({{"include", "#string"}}))}}));
    repo.set("string", Json::object({{"name", "string.quoted.double.hpppl"},
                                     {"begin", "\""},
                                     {"end", "\"(?!\")"},
                                     {"patterns", Json::array()
                                                      .push(match("constant.character.escape.hpppl", "\"\""))
                                                      .push(match("constant.character.escape.hpppl", "\\\\."))}}));
    repo.set("function-definition",
             Json::object({{"match", "(?i)^\\s*(?:(EXPORT)\\s+)?([A-Za-z_\\p{L}][\\w→]*)\\s*(?=\\([^)]*\\)\\s*(?:$|//))"},
                           {"captures", Json::object({{"1", Json::object({{"name", "storage.modifier.export.hpppl"}})},
                                                      {"2", Json::object({{"name", "entity.name.function.hpppl"}})}})}}));
    Json kw = Json::array();
    kw.push(match("keyword.control.hpppl", wordPattern(alternation(control), true)));
    kw.push(match("storage.type.hpppl", wordPattern(alternation(declaration), true)));
    kw.push(match("keyword.operator.logical.hpppl", wordPattern(alternation(logical), true)));
    repo.set("keywords", Json::object({{"patterns", kw}}));
    Json nums = Json::array();
    nums.push(match("constant.numeric.integer.based.hpppl", "#[0-9A-Fa-f]+(?::\\d+)?[bodhBODH]?\\b"));
    nums.push(match("constant.numeric.hpppl", "(?<![\\w→])(?:\\d+\\.?\\d*|\\.\\d+)(?:[eEᴇ][-+−]?\\d+)?(?:_(?:\\([^)]*\\)|[\\p{L}_][\\w^]*))?"));
    repo.set("numbers", Json::object({{"patterns", nums}}));
    Json bi = Json::array();
    bi.push(Json::object({{"match", "(?<![\\w→])(CAS)\\.(" + alternation(casFns) + "|[A-Za-z_]\\w*)"},
                          {"captures", Json::object({{"1", Json::object({{"name", "support.class.cas.hpppl"}})},
                                                     {"2", Json::object({{"name", "support.function.cas.hpppl"}})}})}}));
    bi.push(match("support.function.builtin.hpppl", wordPattern(alternation(commands), false)));
    bi.push(match("support.function.app.hpppl", wordPattern(alternation(appFns), false)));
    bi.push(match("support.function.cas.hpppl", wordPattern(alternation(casFns), false)));
    bi.push(match("constant.language.hpppl", wordPattern(alternation(constants), false)));
    bi.push(match("variable.language.system.hpppl", wordPattern(alternation(variables), false)));
    repo.set("builtins", Json::object({{"patterns", bi}}));
    Json ops = Json::array();
    ops.push(match("keyword.operator.assignment.hpppl", ":=|▶"));
    ops.push(match("keyword.operator.comparison.hpppl", "==|<>|<=|>=|≠|≤|≥|<|>|="));
    ops.push(match("keyword.operator.arithmetic.hpppl", "\\.\\*|\\./|\\.\\^|[-+*/^−×÷√!%|]"));
    ops.push(match("punctuation.terminator.hpppl", ";"));
    repo.set("operators", Json::object({{"patterns", ops}}));

    Json g = Json::object({{"$schema", "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json"},
                           {"name", "HP PPL"},
                           {"scopeName", "source.hpppl"},
                           {"patterns", patterns},
                           {"repository", repo}});
    return g.dump();
}

std::string notepadUdl()
{
    const auto &db = CommandDatabase::instance();
    std::vector<std::string> kw1, kw2, kw3, kw4, kw5, kw6;
    for (const auto &c : db.commands()) {
        if (!colorCommand(c))
            continue;
        if (c.kind == "keyword") {
            if (c.name == "AND" || c.name == "OR" || c.name == "XOR" || c.name == "NOT" || c.name == "MOD")
                continue; // operators2
            kw1.push_back(c.name);
        } else if (c.kind == "cas") {
            kw3.push_back(c.name);
        } else if (c.kind == "appfunc") {
            kw5.push_back(c.name);
        } else if (c.name != "∂" && c.name != "∫") {
            kw2.push_back(c.name);
        }
    }
    for (const auto &g : db.variableGroups())
        for (const auto &n : g.names)
            if (colorVariable(g, n))
                (g.group == "Stałe" ? kw6 : kw4).push_back(n);
    kw3.push_back("CAS");

    std::string x;
    x += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    x += "<!-- Wygenerowane przez: ppl gen notepadpp. HP PPL dla Notepad++ -->\n";
    x += "<NotepadPlus>\n";
    x += "    <UserLang name=\"HP PPL\" ext=\"hpppl ppl hpprgm.txt\" udlVersion=\"2.1\">\n";
    x += "        <Settings>\n";
    x += "            <Global caseIgnored=\"yes\" allowFoldOfComments=\"no\" foldCompact=\"no\" forcePureLC=\"0\" decimalSeparator=\"0\" />\n";
    x += "            <Prefix Keywords1=\"no\" Keywords2=\"no\" Keywords3=\"no\" Keywords4=\"no\" Keywords5=\"no\" Keywords6=\"no\" Keywords7=\"no\" Keywords8=\"no\" />\n";
    x += "        </Settings>\n";
    x += "        <KeywordLists>\n";
    auto list = [&](const std::string &name, const std::string &value) {
        x += "            <Keywords name=\"" + name + "\">" + xmlEscape(value) + "</Keywords>\n";
    };
    list("Comments", "00// 01 02 03 04");
    list("Numbers, prefix1", "#");
    list("Numbers, prefix2", "");
    list("Numbers, extras1", "A B C D E F a b c d e f");
    list("Numbers, extras2", "E e ᴇ");
    list("Numbers, suffix1", "h b o d H B O D");
    list("Numbers, suffix2", "");
    list("Numbers, range", "");
    list("Operators1", "( ) { } [ ] , ; + - * / ^ = < > | ! : ▶ ≠ ≤ ≥ √ −");
    list("Operators2", "AND OR XOR NOT MOD");
    list("Folders in code1, open", "BEGIN IF CASE IFERR FOR WHILE REPEAT");
    list("Folders in code1, middle", "ELSE");
    list("Folders in code1, close", "END UNTIL");
    list("Folders in code2, open", "");
    list("Folders in code2, middle", "");
    list("Folders in code2, close", "");
    list("Folders in comment, open", "");
    list("Folders in comment, middle", "");
    list("Folders in comment, close", "");
    list("Keywords1", join(kw1));
    list("Keywords2", join(kw2));
    list("Keywords3", join(kw3));
    list("Keywords4", join(kw4));
    list("Keywords5", join(kw5));
    list("Keywords6", join(kw6));
    list("Keywords7", "");
    list("Keywords8", "");
    list("Delimiters", "00\" 01\\ 02\" 03' 04 05' 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23");
    x += "        </KeywordLists>\n";
    x += "        <Styles>\n";
    auto style = [&](const std::string &name, const std::string &fg, int fontStyle) {
        x += "            <WordsStyle name=\"" + name + "\" fgColor=\"" + fg
             + "\" bgColor=\"FFFFFF\" colorStyle=\"1\" fontName=\"\" fontStyle=\"" + std::to_string(fontStyle)
             + "\" nesting=\"0\" />\n";
    };
    style("DEFAULT", "000000", 0);
    style("COMMENTS", "008000", 2);
    style("LINE COMMENTS", "008000", 2);
    style("NUMBERS", "C05000", 0);
    style("KEYWORDS1", "0000C0", 1);   // structure
    style("KEYWORDS2", "007080", 0);   // commands
    style("KEYWORDS3", "8000A0", 0);   // CAS
    style("KEYWORDS4", "A01060", 0);   // system variables
    style("KEYWORDS5", "005090", 0);   // app functions
    style("KEYWORDS6", "C05000", 1);   // constants
    style("KEYWORDS7", "000000", 0);
    style("KEYWORDS8", "000000", 0);
    style("OPERATORS", "404040", 1);
    style("FOLDER IN CODE1", "0000C0", 1);
    style("FOLDER IN CODE2", "000000", 0);
    style("FOLDER IN COMMENT", "000000", 0);
    style("DELIMITERS1", "A31515", 0); // strings
    style("DELIMITERS2", "7F007F", 0); // 'quoted'
    for (int i = 3; i <= 8; ++i)
        style("DELIMITERS" + std::to_string(i), "000000", 0);
    x += "        </Styles>\n";
    x += "    </UserLang>\n";
    x += "</NotepadPlus>\n";
    return x;
}

std::string notepadApi()
{
    const auto &db = CommandDatabase::instance();
    struct Entry {
        std::string name;
        const CommandInfo *cmd;
    };
    std::vector<Entry> entries;
    for (const auto &c : db.commands())
        entries.push_back({c.kind == "cas" ? "CAS." + c.name : c.name, &c});
    for (const auto &g : db.variableGroups())
        for (const auto &n : g.names)
            entries.push_back({n, nullptr});
    auto key = [](const std::string &s) { return asciiUpper(s); };
    std::sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b) { return key(a.name) < key(b.name); });
    entries.erase(std::unique(entries.begin(), entries.end(),
                              [&](const Entry &a, const Entry &b) { return key(a.name) == key(b.name); }),
                  entries.end());

    std::string x;
    x += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    x += "<!-- Wygenerowane przez: ppl gen notepadpp. Autouzupełnianie HP PPL dla Notepad++ -->\n";
    x += "<NotepadPlus>\n";
    x += "    <AutoComplete language=\"HP PPL\">\n";
    x += "        <Environment ignoreCase=\"yes\" startFunc=\"(\" stopFunc=\")\" paramSeparator=\",\" terminal=\";\" additionalWordChar=\"_.→\" />\n";
    for (const auto &e : entries) {
        const CommandInfo *c = e.cmd;
        bool func = c && c->kind != "keyword" && c->syntax.find('(') != std::string::npos;
        if (!func) {
            x += "        <KeyWord name=\"" + xmlEscape(e.name) + "\" />\n";
            continue;
        }
        x += "        <KeyWord name=\"" + xmlEscape(e.name) + "\" func=\"yes\">\n";
        x += "            <Overload retVal=\"\" descr=\"" + xmlEscape(c->description) + "\">\n";
        for (const auto &p : Services::syntaxParameters(c->syntax))
            x += "                <Param name=\"" + xmlEscape(p) + "\" />\n";
        x += "            </Overload>\n";
        x += "        </KeyWord>\n";
    }
    x += "    </AutoComplete>\n";
    x += "</NotepadPlus>\n";
    return x;
}

int run(const std::vector<std::string> &args)
{
    if (args.size() < 2) {
        std::fprintf(stderr, "Użycie: ppl gen tmgrammar PLIK.json | ppl gen notepadpp KATALOG\n");
        return 2;
    }
    if (args[0] == "tmgrammar") {
        if (!cli::writeTextFile(args[1], textMateGrammar()))
            return 2;
        std::printf("Zapisano %s\n", args[1].c_str());
        return 0;
    }
    if (args[0] == "notepadpp") {
        namespace fs = std::filesystem;
        fs::path dir = fs::path(reinterpret_cast<const char8_t *>(args[1].c_str()));
        std::error_code ec;
        fs::create_directories(dir, ec);
        fs::path udl = dir / "HP_PPL_udl.xml";
        fs::path api = dir / "HP PPL.xml";
        if (!cli::writeTextFile(udl.string(), notepadUdl()) || !cli::writeTextFile(api.string(), notepadApi()))
            return 2;
        std::printf("Zapisano %s i %s\n", udl.string().c_str(), api.string().c_str());
        return 0;
    }
    std::fprintf(stderr, "Nieznany generator: %s\n", args[0].c_str());
    return 2;
}

} // namespace ppl::gen
