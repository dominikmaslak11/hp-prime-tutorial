#include "analyzer.h"

#include "lexer.h"
#include "utf8.h"

#include <algorithm>
#include <map>
#include <set>

namespace ppl {

int AnalysisResult::count(Severity s) const
{
    return static_cast<int>(std::count_if(diagnostics.begin(), diagnostics.end(),
                                          [s](const Diagnostic &d) { return d.severity == s; }));
}

int AnalysisResult::functionAt(int offset) const
{
    for (size_t i = 0; i < functions.size(); ++i) {
        const auto &f = functions[i];
        if (f.defined && f.bodyOffset >= 0 && offset >= f.offset && (f.endOffset < 0 || offset <= f.endOffset))
            return static_cast<int>(i);
    }
    return -1;
}

namespace {

std::string u8(const std::u32string &s) { return toUtf8(s); }

struct Expr {
    enum Kind { None, Number, Integer, String, List, Matrix, Ident, Call, Other };
    Kind kind = None;
    std::u32string topOp;
    int tokenIndex = -1;
    int usageIndex = -1;
};

class Parser {
public:
    Parser(const std::vector<Token> &tokens, AnalysisResult &result, const CommandDatabase &db)
        : t(tokens), r(result), db(db) {}

    void run()
    {
        parseFile();
        resolveUsages();
        finish();
    }

private:
    struct Local {
        std::u32string name;
        int offset, line, column;
        int uses = 0;
        bool isParam = false;
    };
    struct Usage {
        std::u32string name;
        int offset, length, line, column;
        bool call = false;
        int argc = 0;
        bool target = false;
    };

    const std::vector<Token> &t;
    AnalysisResult &r;
    const CommandDatabase &db;
    size_t p = 0;
    int loopDepth = 0;
    int currentFunction = -1;
    std::vector<Local> locals;
    std::vector<Usage> usages;
    std::map<std::u32string, size_t> functionIndex;
    std::map<std::u32string, size_t> fileVarIndex;

    // ---------------------------------------------------------------- helpers
    const Token &cur() const { return t[p]; }
    const Token &prev() const { return t[p > 0 ? p - 1 : 0]; }
    void adv()
    {
        if (!t[p].isEof())
            ++p;
    }
    bool kw(const char32_t *k) const { return cur().isKeyword(k); }
    bool punct(char32_t c) const { return cur().isPunct(c); }
    bool op(const char32_t *o) const { return cur().isOp(o); }
    bool isIdent() const { return cur().kind == TokenKind::Identifier; }

    void diag(Severity sev, const Token &at, std::string msg, const char *code)
    {
        Diagnostic d;
        d.severity = sev;
        d.offset = at.offset;
        d.length = std::max(1, at.length);
        d.line = at.line;
        d.column = at.column;
        d.message = std::move(msg);
        d.code = code;
        r.diagnostics.push_back(std::move(d));
    }
    void error(const Token &at, std::string msg, const char *code) { diag(Severity::Error, at, std::move(msg), code); }
    void warning(const Token &at, std::string msg, const char *code) { diag(Severity::Warning, at, std::move(msg), code); }

    // Diagnostic placed right after the given token (used for missing ';').
    void diagAfter(Severity sev, const Token &tok, std::string msg, const char *code)
    {
        Diagnostic d;
        d.severity = sev;
        d.offset = tok.offset + tok.length;
        d.length = 1;
        d.line = tok.endLine;
        d.column = tok.endLine == tok.line ? tok.column + tok.length : 1;
        d.message = std::move(msg);
        d.code = code;
        r.diagnostics.push_back(std::move(d));
    }

    std::string describe(const Token &tok) const
    {
        if (tok.isEof())
            return "koniec pliku";
        std::string s = u8(tok.text);
        if (s.size() > 24)
            s = s.substr(0, 24) + "…";
        return "'" + s + "'";
    }

    bool expectKw(const char32_t *k, const Token &opener, const std::string &what)
    {
        if (kw(k)) {
            adv();
            return true;
        }
        std::string msg = "Oczekiwano " + u8(k) + " (" + what + " z linii " + std::to_string(opener.line) + "), a jest "
                          + describe(cur()) + ".";
        if (std::u32string(k) == U"END" && cur().isEof())
            msg = "Brak END zamykającego " + what + " z linii " + std::to_string(opener.line) + ".";
        error(cur(), msg, std::u32string(k) == U"END" ? "missing-end" : "expected-keyword");
        return false;
    }

    bool isTerminator(const std::vector<const char32_t *> &terms) const
    {
        if (cur().isEof())
            return true;
        if (cur().kind != TokenKind::Keyword)
            return false;
        for (const char32_t *k : terms)
            if (cur().upper == k)
                return true;
        return false;
    }

    static bool isBlockKeyword(const Token &tok)
    {
        if (tok.kind != TokenKind::Keyword)
            return false;
        static const std::set<std::u32string> kws = {U"END", U"ELSE", U"UNTIL", U"DEFAULT", U"THEN"};
        return kws.count(tok.upper) > 0;
    }

    static bool isStatementStart(const Token &tok)
    {
        if (tok.kind != TokenKind::Keyword)
            return false;
        static const std::set<std::u32string> kws = {U"IF", U"FOR", U"WHILE", U"REPEAT", U"RETURN", U"LOCAL",
                                                     U"CASE", U"IFERR", U"BEGIN", U"BREAK", U"CONTINUE", U"KILL"};
        return kws.count(tok.upper) > 0;
    }

    // ---------------------------------------------------------------- names
    Local *findLocal(const std::u32string &name)
    {
        for (auto it = locals.rbegin(); it != locals.rend(); ++it)
            if (it->name == name)
                return &*it;
        return nullptr;
    }

    int reference(int tokenIndex, bool call, int argc, bool target)
    {
        const Token &tok = t[tokenIndex];
        const std::u32string &name = tok.text;
        if (name.find(U'.') != std::u32string::npos)
            return -1; // qualified: CAS.idivis, Function.Xmin, PROGRAM.var
        if (Local *l = findLocal(name)) {
            ++l->uses;
            return -1;
        }
        Usage u;
        u.name = name;
        u.offset = tok.offset;
        u.length = tok.length;
        u.line = tok.line;
        u.column = tok.column;
        u.call = call;
        u.argc = argc;
        u.target = target;
        usages.push_back(u);
        return static_cast<int>(usages.size()) - 1;
    }

    void declareLocal(const Token &tok, bool isParam)
    {
        if (!isParam) {
            for (const auto &l : locals) {
                if (l.name == tok.text) {
                    warning(tok, "Zmienna " + u8(tok.text) + " jest już zadeklarowana w tej funkcji.", "duplicate-local");
                    return;
                }
            }
        }
        Local l;
        l.name = tok.text;
        l.offset = tok.offset;
        l.line = tok.line;
        l.column = tok.column;
        l.isParam = isParam;
        locals.push_back(l);
    }

    void declareFileVar(const Token &tok, bool exported)
    {
        if (fileVarIndex.count(tok.text)) {
            if (exported)
                r.fileVariables[fileVarIndex[tok.text]].exported = true;
            return;
        }
        VariableInfo v;
        v.name = u8(tok.text);
        v.name32 = tok.text;
        v.exported = exported;
        v.line = tok.line;
        v.column = tok.column;
        v.offset = tok.offset;
        fileVarIndex[tok.text] = r.fileVariables.size();
        r.fileVariables.push_back(v);
    }

    void checkReservedFunctionName(const Token &tok)
    {
        if (const VariableGroup *g = db.findVariable(tok.text); g && g->fromHpList) {
            warning(tok, "Nazwa " + u8(tok.text) + " to zmienna aplikacji HP. Lepiej wybierz inną nazwę funkcji.",
                    "reserved-name");
            return;
        }
        if (db.isSystemVariable(tok.text)) {
            error(tok, "Nazwa " + u8(tok.text) + " jest zarezerwowana (zmienna systemowa). Wybierz inną nazwę funkcji.",
                  "reserved-name");
            return;
        }
        const CommandInfo *c = db.findCommand(tok.text);
        if (c && c->kind != "keyword" && c->name32 == tok.text)
            warning(tok, "Nazwa " + u8(tok.text) + " przesłania wbudowaną komendę " + c->name + ".", "shadows-command");
    }

    // ---------------------------------------------------------------- file level
    void parseFile()
    {
        while (!cur().isEof()) {
            if (punct(U';')) {
                adv();
                continue;
            }
            if (kw(U"EXPORT")) {
                adv();
                parseTopDecl(true);
            } else if (kw(U"VIEW")) {
                parseView();
            } else if (kw(U"KEY")) {
                parseKey();
            } else if (kw(U"LOCAL")) {
                adv();
                parseLocalList(true);
                if (punct(U';'))
                    adv();
            } else if (isIdent()) {
                parseTopDecl(false);
            } else if (kw(U"BEGIN")) {
                error(cur(), "Blok BEGIN bez nagłówka funkcji. Dodaj przed nim np. EXPORT NAZWA().", "begin-without-header");
                FunctionInfo f;
                f.name = "(bez nazwy)";
                f.offset = cur().offset;
                f.line = cur().line;
                r.functions.push_back(f);
                parseFunctionBody(r.functions.size() - 1, {});
            } else {
                error(cur(), "Nieoczekiwany element poza funkcją: " + describe(cur())
                                 + ". Poza funkcjami mogą stać tylko deklaracje (EXPORT, LOCAL, nazwy funkcji i zmiennych).",
                      "unexpected-toplevel");
                adv();
            }
        }
    }

    std::vector<Token> parseParamList()
    {
        std::vector<Token> params;
        const Token open = cur();
        adv(); // (
        if (punct(U')')) {
            adv();
            return params;
        }
        while (!cur().isEof()) {
            if (isIdent()) {
                for (const auto &q : params)
                    if (q.text == cur().text)
                        error(cur(), "Parametr " + u8(cur().text) + " powtarza się.", "duplicate-param");
                params.push_back(cur());
                adv();
            } else {
                error(cur(), "Oczekiwano nazwy parametru, a jest " + describe(cur()) + ".", "expected-param");
                if (!punct(U',') && !punct(U')'))
                    adv();
            }
            if (punct(U',')) {
                adv();
                continue;
            }
            if (punct(U')')) {
                adv();
                break;
            }
            error(cur(), "Niezamknięta lista parametrów (nawias z linii " + std::to_string(open.line) + ").", "unclosed-paren");
            break;
        }
        return params;
    }

    size_t getFunction(const Token &nameTok)
    {
        auto it = functionIndex.find(nameTok.text);
        if (it != functionIndex.end())
            return it->second;
        FunctionInfo f;
        f.name = u8(nameTok.text);
        f.name32 = nameTok.text;
        f.line = nameTok.line;
        f.column = nameTok.column;
        f.offset = nameTok.offset;
        r.functions.push_back(f);
        functionIndex[nameTok.text] = r.functions.size() - 1;
        return r.functions.size() - 1;
    }

    void parseTopDecl(bool exported)
    {
        if (!isIdent()) {
            error(cur(), "Oczekiwano nazwy funkcji lub zmiennej, a jest " + describe(cur()) + ".", "expected-name");
            adv();
            return;
        }
        const Token nameTok = cur();
        adv();
        if (punct(U'(')) {
            std::vector<Token> params = parseParamList();
            if (punct(U';')) {
                size_t fi = getFunction(nameTok);
                FunctionInfo &f = r.functions[fi];
                if (f.declOffset < 0)
                    f.declOffset = nameTok.offset;
                f.exported = f.exported || exported;
                adv();
                return;
            }
            if (kw(U"BEGIN")) {
                defineFunction(nameTok, params, exported);
                return;
            }
            error(cur(), "Po nagłówku funkcji " + u8(nameTok.text)
                             + " oczekiwano BEGIN (definicja) albo ';' (deklaracja), a jest " + describe(cur()) + ".",
                  "expected-begin");
            return;
        }
        // variable list
        Token vt = nameTok;
        while (true) {
            if (db.isSystemVariable(vt.text))
                warning(vt, "Deklaracja zmiennej o nazwie systemowej " + u8(vt.text) + " — wybierz inną nazwę.", "reserved-name");
            declareFileVar(vt, exported);
            if (op(U":=")) {
                adv();
                parseExpr();
            } else if (op(U"=")) {
                error(cur(), "W deklaracji użyj ':=' zamiast '='.", "equals-in-declaration");
                adv();
                parseExpr();
            }
            if (punct(U',')) {
                adv();
                if (isIdent()) {
                    vt = cur();
                    adv();
                    continue;
                }
                error(cur(), "Oczekiwano nazwy zmiennej po przecinku.", "expected-name");
            }
            break;
        }
        if (punct(U';'))
            adv();
        else
            error(cur(), "Oczekiwano ';' po deklaracji zmiennej, a jest " + describe(cur()) + ".", "missing-semicolon");
    }

    void parseView()
    {
        const Token vtok = cur();
        adv();
        std::string title;
        if (cur().kind == TokenKind::String) {
            title = u8(cur().text);
            adv();
        } else {
            error(cur(), "Po VIEW oczekiwano tekstu pozycji menu w cudzysłowie.", "expected-string");
        }
        if (punct(U','))
            adv();
        else
            error(cur(), "Po tekście VIEW oczekiwano przecinka.", "expected-comma");
        if (!isIdent()) {
            error(cur(), "Oczekiwano nazwy funkcji po VIEW.", "expected-name");
            return;
        }
        const Token nameTok = cur();
        adv();
        std::vector<Token> params;
        if (punct(U'('))
            params = parseParamList();
        if (!params.empty())
            error(params.front(), "Funkcja wywoływana z menu View nie może mieć parametrów.", "view-params");
        if (!kw(U"BEGIN")) {
            error(cur(), "Oczekiwano BEGIN po nagłówku VIEW.", "expected-begin");
            return;
        }
        size_t fi = defineFunction(nameTok, params, false);
        r.functions[fi].isView = true;
        r.functions[fi].viewTitle = title;
        (void)vtok;
    }

    void parseKey()
    {
        adv(); // KEY
        if (!isIdent()) {
            error(cur(), "Oczekiwano nazwy klawisza po KEY, np. K_Sin.", "expected-name");
            return;
        }
        const Token nameTok = cur();
        if (!db.isKeyName(nameTok.text))
            warning(nameTok,
                    "Nieznana nazwa klawisza " + u8(nameTok.text)
                        + ". Nazwy mają postać K_Sin, KS_Enter, KA_7, KSA_Plus (wielkość liter ma znaczenie).",
                    "unknown-key");
        adv();
        std::vector<Token> params;
        if (punct(U'('))
            params = parseParamList();
        if (!kw(U"BEGIN")) {
            error(cur(), "Oczekiwano BEGIN po nagłówku KEY.", "expected-begin");
            return;
        }
        size_t fi = defineFunction(nameTok, params, false);
        r.functions[fi].isKey = true;
    }

    size_t defineFunction(const Token &nameTok, const std::vector<Token> &params, bool exported)
    {
        size_t fi = getFunction(nameTok);
        FunctionInfo &f = r.functions[fi];
        if (f.defined) {
            error(nameTok, "Funkcja " + u8(nameTok.text) + " jest już zdefiniowana w linii " + std::to_string(f.line) + ".",
                  "duplicate-function");
        } else {
            f.line = nameTok.line;
            f.column = nameTok.column;
            f.offset = nameTok.offset;
        }
        checkReservedFunctionName(nameTok);
        f.defined = true;
        f.exported = f.exported || exported;
        f.params.clear();
        for (const auto &pt : params)
            f.params.push_back(u8(pt.text));
        parseFunctionBody(fi, params);
        return fi;
    }

    void parseFunctionBody(size_t fi, const std::vector<Token> &params)
    {
        currentFunction = static_cast<int>(fi);
        locals.clear();
        loopDepth = 0;
        for (const auto &pt : params)
            declareLocal(pt, true);
        const Token begin = cur();
        r.functions[fi].bodyOffset = begin.offset;
        adv(); // BEGIN
        parseStatements({U"END"});
        const Token endTok = cur();
        if (expectKw(U"END", begin, "BEGIN funkcji " + r.functions[fi].name)) {
            r.functions[fi].endOffset = endTok.offset;
            r.functions[fi].endLine = endTok.line;
            if (punct(U';'))
                adv();
            else
                diagAfter(Severity::Error, endTok, "Brak średnika po END kończącym funkcję — kalkulator nie skompiluje programu.",
                          "missing-semicolon");
        } else {
            r.functions[fi].endOffset = cur().offset;
            r.functions[fi].endLine = cur().line;
        }
        for (const auto &l : locals) {
            LocalInfo li;
            li.name = u8(l.name);
            li.name32 = l.name;
            li.isParam = l.isParam;
            li.functionIndex = static_cast<int>(fi);
            li.line = l.line;
            li.column = l.column;
            li.offset = l.offset;
            r.locals.push_back(li);
            if (!l.isParam && l.uses == 0) {
                Token fake;
                fake.offset = l.offset;
                fake.length = static_cast<int>(l.name.size());
                fake.line = l.line;
                fake.column = l.column;
                warning(fake, "Zmienna lokalna " + u8(l.name) + " nie jest używana.", "unused-local");
            }
        }
        locals.clear();
        currentFunction = -1;
    }

    // ---------------------------------------------------------------- statements
    void parseStatements(const std::vector<const char32_t *> &terms)
    {
        while (!cur().isEof()) {
            if (isTerminator(terms))
                break;
            if (punct(U';')) {
                adv();
                continue;
            }
            size_t before = p;
            parseStatement();
            if (p == before) {
                adv();
                continue;
            }
            if (punct(U';')) {
                adv();
                continue;
            }
            if (isTerminator(terms)) {
                if (!cur().isEof())
                    diagAfter(Severity::Warning, prev(), "Brak średnika przed " + u8(cur().upper) + ".", "missing-semicolon");
                continue;
            }
            const Token last = prev();
            diagAfter(Severity::Error, last, "Oczekiwano ';' po poleceniu, a jest " + describe(cur()) + ".",
                      "missing-semicolon");
            // recovery: continue on the next line or after the next ';'
            if (cur().line > last.endLine)
                continue;
            while (!cur().isEof() && !punct(U';') && !isTerminator(terms) && cur().line == last.endLine
                   && !isStatementStart(cur()))
                adv();
            if (punct(U';'))
                adv();
        }
    }

    void parseStatement()
    {
        const Token &tk = cur();
        if (tk.kind == TokenKind::Keyword) {
            const std::u32string &k = tk.upper;
            if (k == U"LOCAL") {
                adv();
                parseLocalList(false);
                return;
            }
            if (k == U"IF") return parseIf();
            if (k == U"CASE") return parseCase();
            if (k == U"IFERR") return parseIferr();
            if (k == U"FOR") return parseFor();
            if (k == U"WHILE") return parseWhile();
            if (k == U"REPEAT") return parseRepeat();
            if (k == U"BREAK" || k == U"CONTINUE") {
                const Token b = cur();
                adv();
                if (loopDepth == 0)
                    error(b, u8(k) + " poza pętlą.", "break-outside-loop");
                if (k == U"BREAK" && (punct(U'(') || cur().kind == TokenKind::Number))
                    parseExpr();
                return;
            }
            if (k == U"KILL") {
                adv();
                return;
            }
            if (k == U"RETURN") {
                adv();
                if (!punct(U';') && !isBlockKeyword(cur()) && !cur().isEof())
                    parseExpr();
                return;
            }
            if (k == U"BEGIN") {
                const Token b = cur();
                adv();
                parseStatements({U"END"});
                expectKw(U"END", b, "BEGIN");
                return;
            }
            if (k == U"EXPORT" || k == U"VIEW" || k == U"KEY") {
                error(tk, u8(k) + " może stać tylko poza funkcjami (przed nagłówkiem funkcji). Czy brakuje END?",
                      "export-inside-function");
                adv();
                return;
            }
            if (k != U"NOT") {
                error(tk, "Nieoczekiwane " + u8(k) + ".", "unexpected-keyword");
                adv();
                return;
            }
        }
        parseExpressionStatement();
    }

    void parseLocalList(bool fileLevel)
    {
        int count = 0;
        while (true) {
            if (!isIdent()) {
                error(cur(), "Oczekiwano nazwy zmiennej po LOCAL, a jest " + describe(cur()) + ".", "expected-name");
                return;
            }
            const Token nameTok = cur();
            if (++count == 9 && !fileLevel)
                error(nameTok, "Jedno polecenie LOCAL może zadeklarować najwyżej 8 zmiennych. Podziel je na dwa polecenia LOCAL.",
                      "too-many-locals");
            adv();
            if (op(U":=")) {
                adv();
                parseExpr();
            } else if (op(U"=")) {
                error(cur(), "W deklaracji LOCAL użyj ':=' zamiast '='.", "equals-in-declaration");
                adv();
                parseExpr();
            }
            if (fileLevel)
                declareFileVar(nameTok, false);
            else
                declareLocal(nameTok, false);
            if (punct(U',')) {
                adv();
                continue;
            }
            return;
        }
    }

    void checkCondition(const Expr &e, const Token &at)
    {
        if (e.topOp == U"=")
            warning(at, "Użyto '=' w warunku. Do porównania służy '=='.", "equals-in-condition");
    }

    void parseIf()
    {
        const Token tok = cur();
        adv();
        const Token condTok = cur();
        Expr c = parseExpr();
        checkCondition(c, condTok);
        if (!expectKw(U"THEN", tok, "IF"))
            if (!isTerminator({U"END", U"ELSE"}) && !isStatementStart(cur()))
                return;
        parseStatements({U"END", U"ELSE"});
        if (kw(U"ELSE")) {
            adv();
            parseStatements({U"END"});
        }
        expectKw(U"END", tok, "IF");
    }

    void parseCase()
    {
        const Token tok = cur();
        adv();
        int branches = 0;
        while (!cur().isEof()) {
            if (kw(U"IF")) {
                ++branches;
                const Token itok = cur();
                adv();
                const Token condTok = cur();
                Expr c = parseExpr();
                checkCondition(c, condTok);
                expectKw(U"THEN", itok, "IF w CASE");
                parseStatements({U"END"});
                expectKw(U"END", itok, "IF w CASE");
                if (punct(U';'))
                    adv();
            } else if (kw(U"DEFAULT")) {
                adv();
                parseStatements({U"END"});
                break;
            } else if (kw(U"END")) {
                break;
            } else if (punct(U';')) {
                adv();
            } else {
                error(cur(), "W bloku CASE oczekiwano 'IF warunek THEN … END;' albo 'DEFAULT', a jest " + describe(cur()) + ".",
                      "case-expected-if");
                while (!cur().isEof() && !punct(U';') && !kw(U"IF") && !kw(U"DEFAULT") && !kw(U"END"))
                    adv();
            }
        }
        if (branches > 127)
            error(tok, "CASE może mieć najwyżej 127 gałęzi.", "case-too-many");
        expectKw(U"END", tok, "CASE");
    }

    void parseIferr()
    {
        const Token tok = cur();
        adv();
        parseStatements({U"THEN"});
        expectKw(U"THEN", tok, "IFERR");
        parseStatements({U"END", U"ELSE"});
        if (kw(U"ELSE")) {
            adv();
            parseStatements({U"END"});
        }
        expectKw(U"END", tok, "IFERR");
    }

    void parseFor()
    {
        const Token tok = cur();
        adv();
        if (isIdent()) {
            reference(static_cast<int>(p), false, 0, true);
            adv();
        } else {
            error(cur(), "Po FOR oczekiwano nazwy zmiennej sterującej.", "expected-name");
        }
        if (kw(U"FROM") || op(U":=")) {
            adv();
            parseExpr();
        } else {
            error(cur(), "Oczekiwano FROM (FOR zmienna FROM start TO koniec DO … END).", "expected-keyword");
        }
        if (kw(U"TO") || kw(U"DOWNTO")) {
            adv();
            parseExpr();
        } else {
            error(cur(), "Oczekiwano TO lub DOWNTO w pętli FOR z linii " + std::to_string(tok.line) + ".", "expected-keyword");
        }
        if (kw(U"STEP")) {
            adv();
            parseExpr();
        }
        expectKw(U"DO", tok, "FOR");
        ++loopDepth;
        parseStatements({U"END"});
        --loopDepth;
        expectKw(U"END", tok, "FOR");
    }

    void parseWhile()
    {
        const Token tok = cur();
        adv();
        const Token condTok = cur();
        Expr c = parseExpr();
        checkCondition(c, condTok);
        expectKw(U"DO", tok, "WHILE");
        ++loopDepth;
        parseStatements({U"END"});
        --loopDepth;
        expectKw(U"END", tok, "WHILE");
    }

    void parseRepeat()
    {
        const Token tok = cur();
        adv();
        ++loopDepth;
        parseStatements({U"UNTIL"});
        --loopDepth;
        if (expectKw(U"UNTIL", tok, "REPEAT")) {
            const Token condTok = cur();
            Expr c = parseExpr();
            checkCondition(c, condTok);
        }
    }

    void parseExpressionStatement()
    {
        const Token startTok = cur();
        Expr lhs = parseExpr();
        if (op(U":=")) {
            adv();
            Expr rhs = parseExpr();
            handleAssign(lhs, rhs);
            return;
        }
        if (lhs.topOp == U"=" )
            warning(startTok, "'=' nie przypisuje wartości. Do przypisania służy ':=' (lub ▶), do porównania '=='.",
                    "equals-statement");
    }

    // ---------------------------------------------------------------- assignment checks
    void handleAssign(const Expr &lhs, const Expr &rhs)
    {
        if (lhs.kind != Expr::Ident && lhs.kind != Expr::Call) {
            if (lhs.tokenIndex >= 0)
                error(t[lhs.tokenIndex], "Nie można przypisać wartości do tego wyrażenia.", "bad-assign-target");
            return;
        }
        if (lhs.tokenIndex < 0)
            return;
        const Token &nameTok = t[lhs.tokenIndex];
        if (lhs.usageIndex >= 0)
            usages[lhs.usageIndex].target = true;
        if (lhs.kind != Expr::Ident || lhs.usageIndex < 0)
            return; // local variable or indexed element
        const std::u32string &n = nameTok.text;
        if (fileVarIndex.count(n))
            return;
        auto rk = rhs.kind;
        auto isDigitAt = [&](size_t k) { return n.size() > k && n[k] >= U'0' && n[k] <= U'9'; };
        if (n.size() == 2 && n[0] == U'L' && isDigitAt(1)) {
            if (rk == Expr::Number || rk == Expr::Integer || rk == Expr::String || rk == Expr::Matrix)
                error(nameTok, u8(n) + " przechowuje wyłącznie listy, np. " + u8(n) + " := {5};", "wrong-type");
        } else if (n.size() == 2 && n[0] == U'M' && isDigitAt(1)) {
            if (rk == Expr::Number || rk == Expr::Integer || rk == Expr::String || rk == Expr::List)
                error(nameTok, u8(n) + " przechowuje wyłącznie macierze i wektory, np. " + u8(n) + " := [[1,2],[3,4]];",
                      "wrong-type");
        } else if (n.size() == 2 && n[0] == U'Z' && isDigitAt(1)) {
            if (rk == Expr::String || rk == Expr::List || rk == Expr::Matrix)
                error(nameTok, u8(n) + " przechowuje wyłącznie liczby (zespolone).", "wrong-type");
        } else if (n.size() == 2 && n[0] == U'G' && isDigitAt(1)) {
            warning(nameTok, "Do zmiennych graficznych G0–G9 nie przypisuje się wartości przez ':='. Użyj DIMGROB_P, BLIT_P, SUBGROB_P.",
                    "grob-assign");
        } else if ((n.size() == 1 && n[0] >= U'A' && n[0] <= U'Z') || n == U"θ") {
            if (rk == Expr::String || rk == Expr::List || rk == Expr::Matrix)
                error(nameTok, "Zmienna " + u8(n) + " przechowuje wyłącznie liczby rzeczywiste. Zadeklaruj zmienną lokalną (LOCAL).",
                      "wrong-type");
        }
    }

    // ---------------------------------------------------------------- expressions
    Expr parseExpr() { return parseStore(); }

    Expr parseStore()
    {
        Expr e = parseWhere();
        while (op(U"▶")) {
            adv();
            Expr target = parsePostfix();
            handleAssign(target, e);
            e = Expr();
            e.kind = Expr::Other;
            e.topOp = U"▶";
        }
        return e;
    }

    Expr parseWhere()
    {
        Expr e = parseOr();
        while (op(U"|")) {
            adv();
            parseOr();
            e.kind = Expr::Other;
            e.topOp = U"|";
        }
        return e;
    }

    Expr parseOr()
    {
        Expr e = parseAnd();
        while (kw(U"OR") || kw(U"XOR")) {
            std::u32string o = cur().upper;
            adv();
            parseAnd();
            e.kind = Expr::Other;
            e.topOp = o;
        }
        return e;
    }

    Expr parseAnd()
    {
        Expr e = parseNot();
        while (kw(U"AND")) {
            adv();
            parseNot();
            e.kind = Expr::Other;
            e.topOp = U"AND";
        }
        return e;
    }

    Expr parseNot()
    {
        if (kw(U"NOT")) {
            adv();
            parseNot();
            Expr e;
            e.kind = Expr::Other;
            e.topOp = U"NOT";
            return e;
        }
        return parseCmp();
    }

    bool isComparison() const
    {
        static const std::vector<std::u32string> ops = {U"==", U"<>", U"<", U"<=", U">", U">=", U"=", U"≠", U"≤", U"≥"};
        if (cur().kind != TokenKind::Operator)
            return false;
        return std::find(ops.begin(), ops.end(), cur().text) != ops.end();
    }

    Expr parseCmp()
    {
        Expr e = parseAdd();
        while (isComparison()) {
            std::u32string o = cur().text;
            adv();
            parseAdd();
            e.kind = Expr::Other;
            e.topOp = o;
        }
        return e;
    }

    Expr parseAdd()
    {
        Expr e = parseMul();
        while (op(U"+") || op(U"-") || op(U"−")) {
            std::u32string o = cur().text;
            adv();
            parseMul();
            e.kind = Expr::Other;
            e.topOp = o;
        }
        return e;
    }

    bool startsPrimary(const Token &tok) const
    {
        switch (tok.kind) {
        case TokenKind::Number: case TokenKind::Integer: case TokenKind::String: case TokenKind::QuotedExpr:
        case TokenKind::Identifier:
            return true;
        case TokenKind::Punct:
            return tok.isPunct(U'(') || tok.isPunct(U'{') || tok.isPunct(U'[');
        case TokenKind::Operator:
            return tok.isOp(U"-") || tok.isOp(U"√") || tok.isOp(U"−");
        default:
            return false;
        }
    }

    Expr parseMul()
    {
        Expr e = parseUnary();
        while (true) {
            if (op(U"*") || op(U"/") || op(U"×") || op(U"÷") || op(U".*") || op(U"./") || kw(U"MOD")
                || (cur().kind == TokenKind::Identifier && cur().upper == U"NTHROOT")) {
                std::u32string o = cur().upper;
                adv();
                parseUnary();
                e.kind = Expr::Other;
                e.topOp = o;
                continue;
            }
            // implicit multiplication such as 2X or 3(X+1)
            if (p > 0 && prev().kind == TokenKind::Number && !cur().spaceBefore
                && (cur().kind == TokenKind::Identifier || punct(U'('))) {
                warning(cur(), "Mnożenie domyślne (np. 2X). W programach pisz jawnie 2*X.", "implicit-multiplication");
                parseUnary();
                e.kind = Expr::Other;
                e.topOp = U"*";
                continue;
            }
            break;
        }
        return e;
    }

    Expr parseUnary()
    {
        if (op(U"-") || op(U"+") || op(U"−") || op(U"√")) {
            std::u32string o = cur().text;
            adv();
            Expr inner = parseUnary();
            if ((o == U"-" || o == U"−" || o == U"+") && (inner.kind == Expr::Number || inner.kind == Expr::Integer))
                return inner;
            Expr e;
            e.kind = Expr::Other;
            e.topOp = o;
            return e;
        }
        return parsePower();
    }

    Expr parsePower()
    {
        Expr base = parsePostfix();
        if (op(U"^") || op(U".^")) {
            adv();
            parseUnary();
            base.kind = Expr::Other;
            base.topOp = U"^";
        }
        return base;
    }

    Expr parsePostfix()
    {
        Expr e = parsePrimary();
        if (e.kind == Expr::Ident) {
            int ti = e.tokenIndex;
            if (punct(U'(')) {
                int argc = parseArgs();
                e.usageIndex = reference(ti, true, argc, false);
                e.kind = Expr::Call;
            } else {
                e.usageIndex = reference(ti, false, 0, false);
            }
        }
        while (true) {
            if (punct(U'(')) {
                if (e.kind == Expr::Call && e.tokenIndex >= 0 && !findLocal(t[e.tokenIndex].text)
                    && !fileVarIndex.count(t[e.tokenIndex].text) && !db.findVariable(t[e.tokenIndex].text))
                    error(cur(), "Nie można indeksować wyniku wywołania funkcji. Zapisz wynik w zmiennej: t := "
                                     + u8(t[e.tokenIndex].text) + "(…); t(2).",
                          "call-index");
                parseArgs();
                if (e.kind != Expr::Call)
                    e.kind = Expr::Other;
                continue;
            }
            if (op(U"!") || op(U"°") || op(U"′") || op(U"″") || op(U"²") || op(U"³") || op(U"%")) {
                adv();
                e.kind = Expr::Other;
                continue;
            }
            if (op(U"⁻") ) {
                adv();
                if (op(U"¹"))
                    adv();
                e.kind = Expr::Other;
                continue;
            }
            break;
        }
        return e;
    }

    int parseArgs()
    {
        const Token open = cur();
        adv(); // (
        int argc = 0;
        if (punct(U')')) {
            adv();
            return 0;
        }
        while (true) {
            if (!punct(U',') && !punct(U')'))
                parseExpr();
            ++argc;
            if (punct(U',')) {
                adv();
                continue;
            }
            if (punct(U')')) {
                adv();
                break;
            }
            error(cur(), "Oczekiwano ',' lub ')' — niezamknięty nawias '(' z linii " + std::to_string(open.line) + ".",
                  "unclosed-paren");
            break;
        }
        return argc;
    }

    Expr parsePrimary()
    {
        Expr e;
        const Token &tk = cur();
        e.tokenIndex = static_cast<int>(p);
        switch (tk.kind) {
        case TokenKind::Number: e.kind = Expr::Number; adv(); return e;
        case TokenKind::Integer: e.kind = Expr::Integer; adv(); return e;
        case TokenKind::String: e.kind = Expr::String; adv(); return e;
        case TokenKind::QuotedExpr: e.kind = Expr::Other; adv(); return e;
        case TokenKind::Identifier: e.kind = Expr::Ident; adv(); return e;
        case TokenKind::Invalid: e.kind = Expr::Other; adv(); return e;
        case TokenKind::Punct:
            if (tk.isPunct(U'(')) {
                const Token open = tk;
                adv();
                if (punct(U')')) {
                    adv();
                    e.kind = Expr::Other;
                    return e;
                }
                Expr inner = parseExpr();
                int count = 1;
                while (punct(U',')) {
                    adv();
                    parseExpr();
                    ++count;
                }
                if (punct(U')'))
                    adv();
                else
                    error(cur(), "Niezamknięty nawias '(' z linii " + std::to_string(open.line) + ".", "unclosed-paren");
                if (count == 1) {
                    inner.topOp.clear();
                    return inner;
                }
                e.kind = Expr::Other;
                return e;
            }
            if (tk.isPunct(U'{')) {
                const Token open = tk;
                adv();
                e.kind = Expr::List;
                if (punct(U'}')) {
                    adv();
                    return e;
                }
                while (true) {
                    parseExpr();
                    if (punct(U',')) {
                        adv();
                        continue;
                    }
                    if (punct(U'}')) {
                        adv();
                        break;
                    }
                    error(cur(), "Oczekiwano ',' lub '}' — niezamknięta lista '{' z linii " + std::to_string(open.line) + ".",
                          "unclosed-brace");
                    break;
                }
                return e;
            }
            if (tk.isPunct(U'[')) {
                const Token open = tk;
                adv();
                e.kind = Expr::Matrix;
                if (punct(U']')) {
                    adv();
                    return e;
                }
                while (true) {
                    parseExpr();
                    if (punct(U',')) {
                        adv();
                        continue;
                    }
                    if (punct(U']')) {
                        adv();
                        break;
                    }
                    if (startsPrimary(cur()))
                        continue; // [1 2 3]
                    error(cur(), "Oczekiwano ',' lub ']' — niezamknięty nawias '[' z linii " + std::to_string(open.line) + ".",
                          "unclosed-bracket");
                    break;
                }
                return e;
            }
            break;
        default:
            break;
        }
        if (tk.isEof()) {
            error(tk, "Nieoczekiwany koniec pliku — oczekiwano wyrażenia.", "expected-expression");
            return e;
        }
        error(tk, "Oczekiwano wyrażenia, a jest " + describe(tk) + ".", "expected-expression");
        bool structural = isBlockKeyword(tk) || tk.isPunct(U';') || tk.isPunct(U')') || tk.isPunct(U'}')
                          || tk.isPunct(U']') || tk.isPunct(U',') || tk.isOp(U":=") || isStatementStart(tk);
        if (!structural)
            adv();
        e.kind = Expr::Other;
        return e;
    }

    // ---------------------------------------------------------------- resolution
    void resolveUsages()
    {
        std::set<std::u32string> reportedUnknown;
        for (const Usage &u : usages) {
            Token at;
            at.offset = u.offset;
            at.length = u.length;
            at.line = u.line;
            at.column = u.column;
            const std::string name = u8(u.name);

            auto fit = functionIndex.find(u.name);
            if (fit != functionIndex.end()) {
                const FunctionInfo &f = r.functions[fit->second];
                int first = f.offset;
                if (f.declOffset >= 0)
                    first = std::min(first, f.declOffset);
                if (u.offset < first) {
                    error(at, "Funkcja " + name + " jest użyta przed deklaracją. Dodaj '" + name
                                  + "();' na początku pliku albo przenieś jej definicję wyżej.",
                          "use-before-declaration");
                } else if (u.call && f.defined && !f.isView && static_cast<int>(f.params.size()) != u.argc) {
                    error(at, "Funkcja " + name + " oczekuje " + std::to_string(f.params.size()) + " argument(ów), podano "
                                  + std::to_string(u.argc) + ".",
                          "wrong-arg-count");
                }
                if (u.target)
                    error(at, "Nie można przypisać wartości do funkcji " + name + ".", "assign-to-function");
                continue;
            }
            if (fileVarIndex.count(u.name))
                continue;

            if (const CommandInfo *c = db.findCommand(u.name); c && c->kind != "keyword") {
                if (u.target && c->category != "System") {
                    // Notes("x") := …, Programs(…) :=, AVars(…) :=, HVars(…) := are valid
                    error(at, "Nie można przypisać wartości do komendy " + c->name + ".", "assign-to-command");
                    continue;
                }
                if (u.call && c->hasArgInfo()) {
                    bool tooFew = u.argc < c->minArgs;
                    bool tooMany = c->maxArgs >= 0 && u.argc > c->maxArgs;
                    if (tooFew || tooMany) {
                        std::string range;
                        if (c->maxArgs < 0)
                            range = "co najmniej " + std::to_string(c->minArgs);
                        else if (c->minArgs == c->maxArgs)
                            range = std::to_string(c->minArgs);
                        else
                            range = "od " + std::to_string(c->minArgs) + " do " + std::to_string(c->maxArgs);
                        error(at, c->name + " przyjmuje " + range + " argument(ów), podano " + std::to_string(u.argc)
                                      + ". Składnia: " + c->syntax,
                              "wrong-arg-count");
                    }
                }
                continue;
            }
            if (const VariableGroup *g = db.findVariable(u.name)) {
                if (u.target && g->group == "Stałe" && name != "COLOR")
                    error(at, name + " to stała (" + (name == "i" ? "jednostka urojona" : "stała matematyczna")
                                  + "), nie zmienna. Zadeklaruj zmienną lokalną: LOCAL " + name + ";",
                          "assign-to-constant");
                continue;
            }

            if (reportedUnknown.insert(u.name).second) {
                std::string msg;
                if (u.target)
                    msg = "Przypisanie do niezadeklarowanej zmiennej " + name
                          + ". Zadeklaruj ją przez LOCAL (lokalna) albo EXPORT (globalna), inaczej kalkulator zgłosi błąd składni.";
                else
                    msg = "Nieznana nazwa " + name
                          + ". Jeśli to nie zmienna ani funkcja wyeksportowana przez inny program, kalkulator zgłosi błąd składni.";
                warning(at, msg, "unknown-name");
            }
        }
    }

    void finish()
    {
        bool hasEntry = std::any_of(r.functions.begin(), r.functions.end(),
                                    [](const FunctionInfo &f) { return f.exported || f.isView || f.isKey; });
        bool hasAnyFunction = std::any_of(r.functions.begin(), r.functions.end(),
                                          [](const FunctionInfo &f) { return f.defined; });
        if (hasAnyFunction && !hasEntry) {
            Token at = t.front();
            diag(Severity::Info, at, "Plik nie zawiera funkcji z EXPORT, więc program nie pojawi się w menu User.",
                 "no-export");
        }
        auto &d = r.diagnostics;
        std::stable_sort(d.begin(), d.end(), [](const Diagnostic &a, const Diagnostic &b) { return a.offset < b.offset; });
        d.erase(std::unique(d.begin(), d.end(),
                            [](const Diagnostic &a, const Diagnostic &b) {
                                return a.offset == b.offset && a.code == b.code && a.message == b.message;
                            }),
                d.end());
    }
};

} // namespace

AnalysisResult Analyzer::analyze(const std::u32string &source) const
{
    AnalysisResult result;
    std::vector<Token> tokens = Lexer::tokenize(source, &result.diagnostics, false);
    Parser parser(tokens, result, m_db);
    parser.run();
    return result;
}

AnalysisResult Analyzer::analyze(std::string_view utf8Source) const
{
    return analyze(toU32(utf8Source));
}

} // namespace ppl
