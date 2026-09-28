// AST parser for the interpreter. The analyzer (analyzer.cpp) reports friendly diagnostics;
// this parser assumes mostly valid code and stops at the first error.
#include "ast.h"

#include "lexer.h"
#include "utf8.h"

#include <set>

namespace ppl {

const FunctionDef *ProgramAst::find(const std::u32string &n) const
{
    for (const auto &f : functions)
        if (f->name == n && !f->body.empty())
            return f.get();
    for (const auto &f : functions)
        if (f->name == n)
            return f.get();
    return nullptr;
}

namespace {

class AstParser {
public:
    explicit AstParser(const std::u32string &src)
    {
        std::vector<Diagnostic> diags;
        t = Lexer::tokenize(src, &diags, false);
        for (const auto &d : diags)
            if (d.severity == Severity::Error)
                throw ParseError{d.message, d.line, d.column};
    }

    ProgramAst program(const std::u32string &name)
    {
        ProgramAst prog;
        prog.name = name;
        while (!cur().isEof()) {
            if (punct(U';')) {
                adv();
                continue;
            }
            if (kw(U"EXPORT")) {
                adv();
                topDecl(prog, true);
            } else if (kw(U"VIEW")) {
                adv();
                std::u32string title;
                if (cur().kind == TokenKind::String) {
                    title = unquote(cur().text);
                    adv();
                }
                expectPunct(U',');
                auto f = functionDef(prog);
                f->isView = true;
                f->viewTitle = title;
            } else if (kw(U"KEY")) {
                adv();
                auto f = functionDef(prog);
                f->isKey = true;
            } else if (kw(U"LOCAL")) {
                adv();
                varList(prog, false);
            } else if (cur().kind == TokenKind::Identifier) {
                topDecl(prog, false);
            } else {
                fail("Nieoczekiwany element poza funkcją");
            }
        }
        return prog;
    }

    StmtList statementsOnly()
    {
        StmtList list = statements({});
        if (!cur().isEof())
            fail("Nieoczekiwany element");
        return list;
    }

    ExprPtr expressionOnly()
    {
        ExprPtr e = expr();
        if (!cur().isEof())
            fail("Nieoczekiwany element w wyrażeniu");
        return e;
    }

private:
    std::vector<Token> t;
    size_t p = 0;

    const Token &cur() const { return t[p]; }
    void adv()
    {
        if (!t[p].isEof())
            ++p;
    }
    bool kw(const char32_t *k) const { return cur().isKeyword(k); }
    bool punct(char32_t c) const { return cur().isPunct(c); }
    bool op(const char32_t *o) const { return cur().isOp(o); }

    [[noreturn]] void fail(const std::string &msg) const
    {
        std::string text = cur().isEof() ? "koniec pliku" : "'" + toUtf8(cur().text) + "'";
        throw ParseError{msg + " (" + text + ")", cur().line, cur().column};
    }
    void expectKw(const char32_t *k)
    {
        if (!kw(k))
            fail("Oczekiwano " + toUtf8(k));
        adv();
    }
    void expectPunct(char32_t c)
    {
        if (!punct(c))
            fail(std::string("Oczekiwano '") + toUtf8(c) + "'");
        adv();
    }
    void optSemicolon()
    {
        if (punct(U';'))
            adv();
    }

    static std::u32string unquote(const std::u32string &s)
    {
        // "abc""d\n" -> abc"d<newline>
        std::u32string out;
        size_t end = s.size() >= 2 && s.back() == U'"' ? s.size() - 1 : s.size();
        for (size_t i = 1; i < end; ++i) {
            char32_t c = s[i];
            if (c == U'"' && i + 1 < end && s[i + 1] == U'"') {
                out += U'"';
                ++i;
            } else if (c == U'\\' && i + 1 < end) {
                char32_t n = s[++i];
                if (n == U'n') out += U'\n';
                else if (n == U't') out += U'\t';
                else out += n;
            } else {
                out += c;
            }
        }
        return out;
    }

    ExprPtr node(ExprKind k, const Token &at, std::u32string text = {})
    {
        auto e = std::make_shared<Expr>();
        e->kind = k;
        e->text = std::move(text);
        e->line = at.line;
        e->column = at.column;
        e->offset = at.offset;
        return e;
    }
    StmtPtr snode(StmtKind k, const Token &at)
    {
        auto s = std::make_shared<Stmt>();
        s->kind = k;
        s->line = at.line;
        s->column = at.column;
        s->offset = at.offset;
        return s;
    }

    // ------------------------------------------------------------ declarations
    std::shared_ptr<FunctionDef> getFunction(ProgramAst &prog, const std::u32string &name)
    {
        for (auto &f : prog.functions)
            if (f->name == name && f->body.empty())
                return f;
        auto f = std::make_shared<FunctionDef>();
        f->name = name;
        prog.functions.push_back(f);
        return f;
    }

    std::vector<std::u32string> params()
    {
        std::vector<std::u32string> ps;
        expectPunct(U'(');
        if (punct(U')')) {
            adv();
            return ps;
        }
        while (true) {
            if (cur().kind != TokenKind::Identifier)
                fail("Oczekiwano nazwy parametru");
            ps.push_back(cur().text);
            adv();
            if (punct(U',')) {
                adv();
                continue;
            }
            expectPunct(U')');
            break;
        }
        return ps;
    }

    std::shared_ptr<FunctionDef> functionDef(ProgramAst &prog)
    {
        if (cur().kind != TokenKind::Identifier)
            fail("Oczekiwano nazwy funkcji");
        const Token nameTok = cur();
        adv();
        std::vector<std::u32string> ps;
        if (punct(U'('))
            ps = params();
        auto f = getFunction(prog, nameTok.text);
        f->params = ps;
        f->line = nameTok.line;
        body(*f);
        return f;
    }

    void body(FunctionDef &f)
    {
        expectKw(U"BEGIN");
        f.body = statements({U"END"});
        f.endLine = cur().line;
        expectKw(U"END");
        optSemicolon();
        if (f.body.empty()) {
            // keep a no-op so that "defined" functions are distinguishable from declarations
            auto s = std::make_shared<Stmt>();
            s->kind = StmtKind::Block;
            s->line = f.endLine;
            f.body.push_back(s);
        }
    }

    void topDecl(ProgramAst &prog, bool exported)
    {
        if (cur().kind != TokenKind::Identifier)
            fail("Oczekiwano nazwy");
        const Token nameTok = cur();
        adv();
        if (punct(U'(')) {
            auto ps = params();
            if (punct(U';')) {
                adv();
                auto f = getFunction(prog, nameTok.text);
                f->exported = f->exported || exported;
                if (f->line == 0)
                    f->line = nameTok.line;
                return;
            }
            auto f = getFunction(prog, nameTok.text);
            f->params = ps;
            f->exported = f->exported || exported;
            f->line = nameTok.line;
            body(*f);
            return;
        }
        p--; // back to the name
        varList(prog, exported);
    }

    void varList(ProgramAst &prog, bool exported)
    {
        while (true) {
            if (cur().kind != TokenKind::Identifier)
                fail("Oczekiwano nazwy zmiennej");
            VarDecl v;
            v.name = cur().text;
            v.exported = exported;
            v.line = cur().line;
            adv();
            if (op(U":=")) {
                adv();
                v.init = expr();
            }
            prog.variables.push_back(v);
            if (punct(U',')) {
                adv();
                continue;
            }
            break;
        }
        optSemicolon();
    }

    // ------------------------------------------------------------ statements
    bool atTerminator(const std::vector<const char32_t *> &terms) const
    {
        if (cur().isEof())
            return true;
        if (cur().kind != TokenKind::Keyword)
            return false;
        for (auto k : terms)
            if (cur().upper == k)
                return true;
        return false;
    }

    StmtList statements(const std::vector<const char32_t *> &terms)
    {
        StmtList list;
        while (!atTerminator(terms)) {
            if (punct(U';')) {
                adv();
                continue;
            }
            list.push_back(statement());
            if (punct(U';'))
                adv();
            else if (!atTerminator(terms) && !cur().isEof())
                fail("Oczekiwano ';'");
        }
        return list;
    }

    StmtPtr statement()
    {
        const Token tk = cur();
        if (tk.kind == TokenKind::Keyword) {
            const auto &k = tk.upper;
            if (k == U"LOCAL") {
                adv();
                auto s = snode(StmtKind::Local, tk);
                while (true) {
                    if (cur().kind != TokenKind::Identifier)
                        fail("Oczekiwano nazwy zmiennej po LOCAL");
                    s->names.push_back(cur().text);
                    adv();
                    if (op(U":=")) {
                        adv();
                        s->inits.push_back(expr());
                    } else {
                        s->inits.push_back(nullptr);
                    }
                    if (punct(U',')) {
                        adv();
                        continue;
                    }
                    break;
                }
                return s;
            }
            if (k == U"IF") {
                adv();
                auto s = snode(StmtKind::If, tk);
                s->expr = expr();
                expectKw(U"THEN");
                s->body = statements({U"END", U"ELSE"});
                if (kw(U"ELSE")) {
                    adv();
                    s->elseBody = statements({U"END"});
                }
                s->endLine = cur().line;
                expectKw(U"END");
                return s;
            }
            if (k == U"CASE") {
                adv();
                auto s = snode(StmtKind::Case, tk);
                while (!cur().isEof()) {
                    if (punct(U';')) {
                        adv();
                        continue;
                    }
                    if (kw(U"IF")) {
                        CaseBranch b;
                        b.line = cur().line;
                        adv();
                        b.condition = expr();
                        expectKw(U"THEN");
                        b.body = statements({U"END"});
                        expectKw(U"END");
                        s->branches.push_back(std::move(b));
                        continue;
                    }
                    if (kw(U"DEFAULT")) {
                        adv();
                        s->hasDefault = true;
                        s->elseBody = statements({U"END"});
                        break;
                    }
                    break;
                }
                s->endLine = cur().line;
                expectKw(U"END");
                return s;
            }
            if (k == U"IFERR") {
                adv();
                auto s = snode(StmtKind::IfErr, tk);
                s->body = statements({U"THEN"});
                expectKw(U"THEN");
                s->elseBody = statements({U"END", U"ELSE"});
                if (kw(U"ELSE")) {
                    adv();
                    s->thirdBody = statements({U"END"});
                }
                s->endLine = cur().line;
                expectKw(U"END");
                return s;
            }
            if (k == U"FOR") {
                adv();
                auto s = snode(StmtKind::For, tk);
                if (cur().kind != TokenKind::Identifier)
                    fail("Oczekiwano zmiennej po FOR");
                s->target = node(ExprKind::Ident, cur(), cur().text);
                adv();
                if (kw(U"FROM") || op(U":="))
                    adv();
                else
                    fail("Oczekiwano FROM");
                s->expr = expr();
                if (kw(U"DOWNTO"))
                    s->down = true;
                else if (!kw(U"TO"))
                    fail("Oczekiwano TO lub DOWNTO");
                adv();
                s->value = expr();
                if (kw(U"STEP")) {
                    adv();
                    s->step = expr();
                }
                expectKw(U"DO");
                s->body = statements({U"END"});
                s->endLine = cur().line;
                expectKw(U"END");
                return s;
            }
            if (k == U"WHILE") {
                adv();
                auto s = snode(StmtKind::While, tk);
                s->expr = expr();
                expectKw(U"DO");
                s->body = statements({U"END"});
                s->endLine = cur().line;
                expectKw(U"END");
                return s;
            }
            if (k == U"REPEAT") {
                adv();
                auto s = snode(StmtKind::Repeat, tk);
                s->body = statements({U"UNTIL"});
                s->endLine = cur().line;
                expectKw(U"UNTIL");
                s->expr = expr();
                return s;
            }
            if (k == U"BREAK") {
                adv();
                auto s = snode(StmtKind::Break, tk);
                if (punct(U'(') || cur().kind == TokenKind::Number)
                    s->expr = expr();
                return s;
            }
            if (k == U"CONTINUE") {
                adv();
                return snode(StmtKind::Continue, tk);
            }
            if (k == U"KILL") {
                adv();
                return snode(StmtKind::Kill, tk);
            }
            if (k == U"RETURN") {
                adv();
                auto s = snode(StmtKind::Return, tk);
                if (!punct(U';') && !cur().isEof() && !(cur().kind == TokenKind::Keyword && cur().upper != U"NOT"))
                    s->expr = expr();
                return s;
            }
            if (k == U"BEGIN") {
                adv();
                auto s = snode(StmtKind::Block, tk);
                s->body = statements({U"END"});
                expectKw(U"END");
                return s;
            }
            if (k != U"NOT")
                fail("Nieoczekiwane słowo kluczowe");
        }
        auto s = snode(StmtKind::Expr, tk);
        ExprPtr e = expr();
        if (op(U":=")) {
            adv();
            s->kind = StmtKind::Assign;
            s->target = e;
            s->value = expr();
            return s;
        }
        s->expr = e;
        return s;
    }

    // ------------------------------------------------------------ expressions
    ExprPtr expr() { return store(); }

    ExprPtr binary(const Token &at, const std::u32string &opText, ExprPtr a, ExprPtr b)
    {
        auto e = node(ExprKind::Binary, at, opText);
        e->args = {std::move(a), std::move(b)};
        return e;
    }

    ExprPtr store()
    {
        ExprPtr e = where();
        while (op(U"▶")) {
            const Token at = cur();
            adv();
            ExprPtr target = postfix();
            auto s = node(ExprKind::Store, at);
            s->args = {e, target};
            e = s;
        }
        return e;
    }

    ExprPtr where()
    {
        ExprPtr e = orExpr();
        while (op(U"|")) {
            const Token at = cur();
            adv();
            auto w = node(ExprKind::Where, at);
            w->args = {e, orExpr()};
            e = w;
        }
        return e;
    }

    ExprPtr orExpr()
    {
        ExprPtr e = andExpr();
        while (kw(U"OR") || kw(U"XOR")) {
            const Token at = cur();
            adv();
            e = binary(at, at.upper, e, andExpr());
        }
        return e;
    }

    ExprPtr andExpr()
    {
        ExprPtr e = notExpr();
        while (kw(U"AND")) {
            const Token at = cur();
            adv();
            e = binary(at, U"AND", e, notExpr());
        }
        return e;
    }

    ExprPtr notExpr()
    {
        if (kw(U"NOT")) {
            const Token at = cur();
            adv();
            auto u = node(ExprKind::Unary, at, U"NOT");
            u->args = {notExpr()};
            return u;
        }
        return cmp();
    }

    ExprPtr cmp()
    {
        static const std::set<std::u32string> ops = {U"==", U"<>", U"<", U"<=", U">", U">=", U"=", U"≠", U"≤", U"≥"};
        ExprPtr e = add();
        while (cur().kind == TokenKind::Operator && ops.count(cur().text)) {
            const Token at = cur();
            std::u32string o = at.text;
            if (o == U"≠") o = U"<>";
            if (o == U"≤") o = U"<=";
            if (o == U"≥") o = U">=";
            adv();
            e = binary(at, o, e, add());
        }
        return e;
    }

    ExprPtr add()
    {
        ExprPtr e = mul();
        while (op(U"+") || op(U"-") || op(U"−")) {
            const Token at = cur();
            adv();
            e = binary(at, at.text == U"+" ? U"+" : U"-", e, mul());
        }
        return e;
    }

    bool startsPrimary() const
    {
        const Token &tk = cur();
        switch (tk.kind) {
        case TokenKind::Number: case TokenKind::Integer: case TokenKind::String: case TokenKind::QuotedExpr:
        case TokenKind::Identifier: return true;
        case TokenKind::Punct: return tk.isPunct(U'(') || tk.isPunct(U'{') || tk.isPunct(U'[');
        case TokenKind::Operator: return tk.isOp(U"-") || tk.isOp(U"√") || tk.isOp(U"−");
        default: return false;
        }
    }

    ExprPtr mul()
    {
        ExprPtr e = unary();
        while (true) {
            if (op(U"*") || op(U"/") || op(U"×") || op(U"÷") || op(U".*") || op(U"./") || kw(U"MOD")) {
                const Token at = cur();
                std::u32string o = at.kind == TokenKind::Keyword ? U"MOD" : at.text;
                if (o == U"×") o = U"*";
                if (o == U"÷") o = U"/";
                adv();
                e = binary(at, o, e, unary());
                continue;
            }
            // implicit multiplication 2X
            if (p > 0 && t[p - 1].kind == TokenKind::Number && !cur().spaceBefore
                && (cur().kind == TokenKind::Identifier || punct(U'('))) {
                const Token at = cur();
                e = binary(at, U"*", e, unary());
                continue;
            }
            break;
        }
        return e;
    }

    ExprPtr unary()
    {
        if (op(U"-") || op(U"+") || op(U"−") || op(U"√")) {
            const Token at = cur();
            std::u32string o = at.text == U"−" ? U"-" : at.text;
            adv();
            if (o == U"+")
                return unary();
            auto u = node(ExprKind::Unary, at, o);
            u->args = {unary()};
            return u;
        }
        return power();
    }

    ExprPtr power()
    {
        ExprPtr base = postfix();
        if (op(U"^") || op(U".^")) {
            const Token at = cur();
            adv();
            return binary(at, at.text, base, unary());
        }
        // infix n NTHROOT x (binds tighter than * and unary minus, left to right)
        while (cur().kind == TokenKind::Identifier && asciiUpper(cur().text) == U"NTHROOT") {
            const Token at = cur();
            adv();
            base = binary(at, U"NTHROOT", base, postfix());
        }
        return base;
    }

    std::vector<ExprPtr> args()
    {
        std::vector<ExprPtr> list;
        expectPunct(U'(');
        if (punct(U')')) {
            adv();
            return list;
        }
        while (true) {
            if (punct(U',') || punct(U')'))
                list.push_back(nullptr); // empty argument ARC(0,0,60,0,,c)
            else
                list.push_back(expr());
            if (punct(U',')) {
                adv();
                continue;
            }
            expectPunct(U')');
            break;
        }
        return list;
    }

    ExprPtr postfix()
    {
        ExprPtr e = primary();
        while (true) {
            if (punct(U'(')) {
                const Token at = cur();
                auto a = args();
                if (e->kind == ExprKind::Ident) {
                    e->kind = ExprKind::Call;
                    e->args = std::move(a);
                } else {
                    auto ix = node(ExprKind::Index, at);
                    ix->args.push_back(e);
                    for (auto &x : a)
                        ix->args.push_back(x);
                    e = ix;
                }
                continue;
            }
            if (op(U"!") || op(U"°") || op(U"′") || op(U"″") || op(U"²") || op(U"³") || op(U"%")) {
                const Token at = cur();
                adv();
                auto pf = node(ExprKind::Postfix, at, at.text);
                pf->args = {e};
                e = pf;
                continue;
            }
            if (op(U"⁻")) {
                const Token at = cur();
                adv();
                if (op(U"¹"))
                    adv();
                auto pf = node(ExprKind::Postfix, at, U"⁻¹");
                pf->args = {e};
                e = pf;
                continue;
            }
            break;
        }
        return e;
    }

    ExprPtr primary()
    {
        const Token tk = cur();
        switch (tk.kind) {
        case TokenKind::Number: {
            adv();
            auto e = node(ExprKind::Number, tk, tk.text);
            std::string s;
            for (char32_t c : tk.text) {
                if (c == U'_')
                    break;
                if (c == 0x1D07 || c == U'e')
                    s += 'E';
                else if (c == 0x2212)
                    s += '-';
                else
                    s += static_cast<char>(c);
            }
            e->number = std::strtod(s.c_str(), nullptr);
            return e;
        }
        case TokenKind::Integer: adv(); return node(ExprKind::Integer, tk, tk.text);
        case TokenKind::String: adv(); return node(ExprKind::String, tk, unquote(tk.text));
        case TokenKind::QuotedExpr: {
            adv();
            std::u32string s = tk.text.substr(1);
            if (!s.empty() && s.back() == U'\'')
                s.pop_back();
            return node(ExprKind::Quoted, tk, s);
        }
        case TokenKind::Identifier: adv(); return node(ExprKind::Ident, tk, tk.text);
        case TokenKind::Punct:
            if (tk.isPunct(U'(')) {
                adv();
                ExprPtr first = expr();
                if (punct(U',')) {
                    auto tup = node(ExprKind::Tuple, tk);
                    tup->args.push_back(first);
                    while (punct(U',')) {
                        adv();
                        tup->args.push_back(expr());
                    }
                    expectPunct(U')');
                    return tup;
                }
                expectPunct(U')');
                return first;
            }
            if (tk.isPunct(U'{')) {
                adv();
                auto l = node(ExprKind::List, tk);
                if (punct(U'}')) {
                    adv();
                    return l;
                }
                while (true) {
                    l->args.push_back(expr());
                    if (punct(U',')) {
                        adv();
                        continue;
                    }
                    expectPunct(U'}');
                    break;
                }
                return l;
            }
            if (tk.isPunct(U'[')) {
                adv();
                auto m = node(ExprKind::Matrix, tk);
                if (punct(U']')) {
                    adv();
                    return m;
                }
                while (true) {
                    m->args.push_back(expr());
                    if (punct(U',')) {
                        adv();
                        continue;
                    }
                    if (punct(U']')) {
                        adv();
                        break;
                    }
                    if (startsPrimary())
                        continue;
                    fail("Oczekiwano ',' lub ']'");
                }
                return m;
            }
            break;
        default: break;
        }
        fail("Oczekiwano wyrażenia");
    }
};

} // namespace

ProgramAst parseProgram(const std::u32string &source, const std::u32string &programName)
{
    AstParser p(source);
    return p.program(programName);
}

StmtList parseStatements(const std::u32string &source)
{
    AstParser p(source);
    return p.statementsOnly();
}

ExprPtr parseExpression(const std::u32string &source)
{
    AstParser p(source);
    return p.expressionOnly();
}

} // namespace ppl
