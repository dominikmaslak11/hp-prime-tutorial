#pragma once

#include "ast.h"
#include "commanddb.h"
#include "graphics.h"
#include "host.h"
#include "value.h"

#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace ppl {

class Interpreter;

// Arguments of a built-in command. Eager commands get evaluated values; lazy ones evaluate what they need.
struct CallArgs {
    Interpreter &in;
    const std::u32string &name;
    const std::vector<ExprPtr> &exprs;
    std::vector<Value> values;
    size_t size() const { return exprs.size(); }
    bool has(size_t i) const { return i < exprs.size() && exprs[i] != nullptr; }
    const Value &operator[](size_t i) const { return values.at(i); }
    double num(size_t i) const;
    int integer(size_t i) const;
    std::u32string text(size_t i) const;   // string argument
};

using BuiltinFn = std::function<Value(CallArgs &)>;

struct Builtin {
    int minArgs = 0;
    int maxArgs = -1;
    bool lazy = false;
    BuiltinFn fn;
};

// Control-flow signals
struct ReturnSignal { Value value; };
struct BreakSignal { int levels = 1; };
struct ContinueSignal {};
struct KillSignal {};

class Interpreter {
public:
    struct Frame {
        const FunctionDef *function = nullptr;   // nullptr = Home / console
        int unit = -1;
        std::vector<std::pair<std::u32string, Value>> locals;
        int line = 0;
        int column = 0;
    };
    struct Unit {
        std::u32string name;
        std::u32string source;
        ProgramAst ast;
        std::vector<std::pair<std::u32string, Value>> fileVars;
        bool initialized = false;
    };
    struct AppState {
        std::u32string current = U"Function";
        std::map<std::u32string, bool> checked;       // "F1" → true
        std::map<std::u32string, Value> colors;       // "F1" → color
        std::map<std::u32string, std::u32string> sample, freq;  // "H1" → "D1"
        std::map<std::u32string, std::u32string> indep, depend; // "S1" → "C1"
    };

    explicit Interpreter(Host &host, const CommandDatabase &db = CommandDatabase::instance());

    // Loads a program. The first loaded program is the main one; others act as libraries
    // (their EXPORTed functions and variables are visible, like other programs on the calculator).
    // Throws ParseError.
    void loadProgram(const std::u32string &source, const std::u32string &name);

    // Evaluates text typed in Home, e.g. "SQIN(5)" or "A:=2; F(A)". Returns the last value.
    // Throws RuntimeError, KillSignal.
    Value run(const std::u32string &text);
    // Evaluates an expression in the context of a stack frame (debugger watch / console).
    Value evaluateInFrame(const std::u32string &text, int frameIndex);

    // ---- state for tools (debugger, CLI)
    const std::vector<Frame> &frames() const { return m_frames; }
    const std::vector<Unit> &units() const { return m_units; }
    std::vector<std::pair<std::u32string, Value>> globalVariables() const;   // user/exported
    std::vector<std::pair<std::u32string, Value>> systemVariables(bool onlyChanged) const;
    bool setVariableInFrame(int frameIndex, const std::u32string &name, const Value &v);
    Graphics &graphics() { return m_graphics; }
    Host &host() { return m_host; }
    const CommandDatabase &db() const { return m_db; }
    Value *systemVariable(const std::u32string &name) { return findSystem(name); }
    uint64_t steps() const { return m_steps; }
    void setMaxSteps(uint64_t n) { m_maxSteps = n; }
    void setSeed(uint64_t seed) { m_rng.seed(seed); }
    std::u32string format(const Value &v, bool quoteStrings = true) const;
    FormatSettings formatSettings() const;
    int lastErrorLine() const { return m_errorLine; }
    int lastErrorColumn() const { return m_errorColumn; }
    std::u32string lastErrorProgram() const { return m_errorProgram; }
    std::u32string currentProgramName() const;

    // Called before every statement (debugger). May block.
    std::function<void(const Stmt &, int depth)> onStatement;

    // ---- API used by built-in commands
    Value eval(const Expr &e);
    Value evalText(const std::u32string &expressionText);
    // EXPR("…"): a statement sequence executed in the current context; returns the last value.
    Value execText(const std::u32string &text);
    void assign(const Expr &target, const Value &v);
    Value *findVariable(const std::u32string &name);
    Value getVariable(const std::u32string &name);
    void setVariable(const std::u32string &name, const Value &v, bool create);
    Value withBinding(const std::u32string &name, const Value &v, const std::function<Value()> &fn);
    double angleToRadians(double a) const;
    double radiansToAngle(double r) const;
    int angleMode() const;
    bool complexMode() const;
    std::mt19937_64 &rng() { return m_rng; }
    AppState &app() { return m_app; }
    std::map<std::u32string, std::u32string> &notes() { return m_notes; }
    std::map<std::u32string, Value> &avars() { return m_avars; }
    std::u32string programSource(const std::u32string &name) const;
    void setProgramSource(const std::u32string &name, const std::u32string &src);
    std::vector<std::u32string> programNames() const;
    double evalFunctionOf(const std::u32string &exprText, const std::u32string &var, double x);
    [[noreturn]] void unsupported(const std::u32string &what) const;

private:
    Host &m_host;
    const CommandDatabase &m_db;
    Graphics m_graphics;
    std::vector<Unit> m_units;
    std::vector<Frame> m_frames;
    std::vector<std::pair<std::u32string, Value>> m_bindings;
    std::map<std::u32string, Value> m_globals;     // user variables (exported / created in Home)
    std::map<std::u32string, Value> m_system;      // system & app variables
    std::map<std::u32string, Value> m_systemDefaults;
    std::map<std::u32string, Builtin> m_builtins;  // key: ASCII upper-case name
    std::map<std::u32string, ExprPtr> m_exprCache;
    std::map<std::u32string, std::u32string> m_notes;
    std::map<std::u32string, Value> m_avars;
    std::map<std::u32string, std::u32string> m_extraPrograms;
    AppState m_app;
    std::mt19937_64 m_rng;
    uint64_t m_steps = 0;
    uint64_t m_maxSteps = 0;
    Value m_last;
    int m_errorLine = 0, m_errorColumn = 0;
    std::u32string m_errorProgram;

    void initSystemVariables();
    void registerBuiltins();
    void initUnit(int unit);
    void exec(const Stmt &s);
    void execList(const StmtList &list);
    Value call(const Expr &call);
    Value callFunction(int unit, const FunctionDef &fn, std::vector<Value> args);
    Value callBuiltin(const std::u32string &name, const Builtin &b, const std::vector<ExprPtr> &args);
    Value index(const Value &target, const std::vector<Value> &indices, const std::u32string &what);
    void assignIndexed(Value &target, const std::vector<Value> &indices, const Value &v);
    Value binary(const std::u32string &op, const Value &a, const Value &b);
    Value unaryOp(const std::u32string &op, const Value &a);
    Value parseInteger(const std::u32string &text) const;
    bool findFunction(const std::u32string &name, int &unit, const FunctionDef *&fn) const;
    int currentUnit() const;
    Value *findSystem(const std::u32string &name);
    void checkSystemType(const std::u32string &name, const Value &v) const;
    void hook(const Stmt &s);
    void tick();

    friend void registerAllBuiltins(Interpreter &in, std::map<std::u32string, Builtin> &table);
    friend struct BuiltinAccess;
};

// Arithmetic helpers shared with built-ins
Value arith(const std::u32string &op, const Value &a, const Value &b, Interpreter &in);

} // namespace ppl
