#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ppl {

struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

enum class ExprKind {
    Number,     // num / text (with unit suffix kept in text)
    Integer,    // #FFh (text)
    String,     // text
    Quoted,     // 'expr' (text without quotes)
    Ident,      // text
    Call,       // text(args)  — function call or indexing of a variable
    Index,      // args[0](args[1..])  — indexing of an arbitrary expression
    List,       // {args}
    Matrix,     // [args]  (rows are nested Matrix nodes)
    Tuple,      // (a, b) — point / complex
    Unary,      // text = operator, args[0]
    Binary,     // text = operator, args[0], args[1]
    Postfix,    // text = operator, args[0]
    Where,      // args[0] | args[1]
    Store       // args[0] ▶ args[1]
};

struct Expr {
    ExprKind kind = ExprKind::Number;
    std::u32string text;
    double number = 0;
    std::vector<ExprPtr> args;
    int line = 0, column = 0, offset = 0;
};

enum class StmtKind { Expr, Assign, Local, If, Case, IfErr, For, While, Repeat, Break, Continue, Kill, Return, Block };

struct CaseBranch {
    ExprPtr condition;
    StmtList body;
    int line = 0;
};

struct Stmt {
    StmtKind kind = StmtKind::Expr;
    int line = 0, column = 0, offset = 0;
    int endLine = 0;                  // line of END / UNTIL (for "step out" display)
    ExprPtr expr;                     // Expr, Return, Break level, If/While/Repeat condition, For start
    ExprPtr target;                   // Assign target, For variable (Ident)
    ExprPtr value;                    // Assign value, For end
    ExprPtr step;                     // For step
    bool down = false;                // FOR … DOWNTO
    StmtList body;                    // then / loop body / try block
    StmtList elseBody;                // else / IFERR THEN
    StmtList thirdBody;               // IFERR ELSE
    std::vector<CaseBranch> branches; // CASE
    bool hasDefault = false;
    std::vector<std::u32string> names;   // LOCAL
    std::vector<ExprPtr> inits;          // LOCAL initialisers (may be null)
};

struct FunctionDef {
    std::u32string name;
    std::vector<std::u32string> params;
    StmtList body;
    bool exported = false;
    bool isView = false;
    bool isKey = false;
    std::u32string viewTitle;
    int line = 0;
    int endLine = 0;
};

struct VarDecl {
    std::u32string name;
    ExprPtr init;
    bool exported = false;
    int line = 0;
};

struct ProgramAst {
    std::u32string name;                    // program (file) name
    std::vector<std::shared_ptr<FunctionDef>> functions;
    std::vector<VarDecl> variables;
    const FunctionDef *find(const std::u32string &name) const;
};

struct ParseError {
    std::string message;
    int line = 0, column = 0;
};

// Parses a whole program. Throws ParseError on the first syntax error.
ProgramAst parseProgram(const std::u32string &source, const std::u32string &programName);
// Parses a single expression or a sequence of statements typed in Home / the debug console.
StmtList parseStatements(const std::u32string &source);
ExprPtr parseExpression(const std::u32string &source);

} // namespace ppl
