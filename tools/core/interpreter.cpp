#include "interpreter.h"

#include "utf8.h"

#include <algorithm>
#include <cmath>

namespace ppl {

void registerAllBuiltins(Interpreter &in, std::map<std::u32string, Builtin> &table);

namespace {

std::u32string u32(const std::string &s) { return toU32(s); }
std::string u8(const std::u32string &s) { return toUtf8(s); }

bool isDigitAt(const std::u32string &n, size_t k) { return n.size() > k && n[k] >= U'0' && n[k] <= U'9'; }

// Category of a system variable for type checks
enum class SysKind { Real, Complex, List, Matrix, Graphic, Equation, Any };

SysKind sysKind(const std::u32string &n)
{
    if ((n.size() == 1 && n[0] >= U'A' && n[0] <= U'Z') || n == U"θ")
        return SysKind::Real;
    if (n.size() == 2 && isDigitAt(n, 1)) {
        switch (n[0]) {
        case U'Z': return SysKind::Complex;
        case U'L': case U'D': case U'C': return SysKind::List;
        case U'M': return SysKind::Matrix;
        case U'G': return SysKind::Graphic;
        case U'F': case U'R': case U'X': case U'Y': case U'U': case U'V': case U'E': return SysKind::Equation;
        default: break;
        }
    }
    if (n.size() == 3 && n[0] == U'F' && n[1] == U'Z' && isDigitAt(n, 2))
        return SysKind::Equation;
    return SysKind::Any;
}

} // namespace

// ---------------------------------------------------------------- CallArgs

double CallArgs::num(size_t i) const
{
    if (i >= values.size())
        throw RuntimeError(u8(name) + ": brak argumentu " + std::to_string(i + 1) + ".");
    return values[i].toReal(u8(name).c_str());
}

int CallArgs::integer(size_t i) const
{
    if (i >= values.size())
        throw RuntimeError(u8(name) + ": brak argumentu " + std::to_string(i + 1) + ".");
    return values[i].toInt(u8(name).c_str());
}

std::u32string CallArgs::text(size_t i) const
{
    if (i >= values.size())
        throw RuntimeError(u8(name) + ": brak argumentu " + std::to_string(i + 1) + ".");
    if (values[i].type == Value::Type::String || values[i].type == Value::Type::Symbolic)
        return values[i].str;
    throw RuntimeError(u8(name) + ": argument " + std::to_string(i + 1) + " musi być tekstem, a jest "
                       + typeName(values[i]) + ".");
}

// ---------------------------------------------------------------- construction

Interpreter::Interpreter(Host &host, const CommandDatabase &db) : m_host(host), m_db(db), m_rng(std::random_device{}())
{
    initSystemVariables();
    registerAllBuiltins(*this, m_builtins);
}

void Interpreter::initSystemVariables()
{
    auto set = [&](const std::u32string &n, Value v) { m_system[n] = v; };
    for (char32_t c = U'A'; c <= U'Z'; ++c)
        set(std::u32string(1, c), Value::real(0));
    set(U"θ", Value::real(0));
    for (int i = 0; i <= 9; ++i) {
        std::u32string d = u32(std::to_string(i));
        set(U"Z" + d, Value::real(0));
        set(U"L" + d, Value::makeList());
        set(U"M" + d, Value::matrix(0, 0));
        set(U"D" + d, Value::makeList());
        set(U"C" + d, Value::makeList());
        set(U"G" + d, Value::graphic(i));
        for (const char32_t *p : {U"F", U"R", U"X", U"Y", U"U", U"V", U"E", U"FZ"})
            set(std::u32string(p) + d, Value::string(U""));
    }
    // settings
    set(U"HAngle", Value::real(0));
    set(U"HFormat", Value::real(0));
    set(U"HDigits", Value::real(4));
    set(U"HComplex", Value::real(0));
    set(U"Entry", Value::real(0));
    set(U"Base", Value::real(3));
    set(U"Bits", Value::real(32));
    set(U"Signed", Value::real(0));
    set(U"Language", Value::real(1));
    set(U"TOff", Value::integer(300000, 'h'));
    set(U"Ans", Value::real(0));
    set(U"AAngle", Value::real(0));
    set(U"AComplex", Value::real(0));
    set(U"AFormat", Value::real(0));
    set(U"ADigits", Value::real(4));
    // Function app plot window (defaults of the calculator)
    set(U"Xmin", Value::real(-15.9));
    set(U"Xmax", Value::real(15.9));
    set(U"Ymin", Value::real(-10.9));
    set(U"Ymax", Value::real(10.9));
    set(U"Xtick", Value::real(1));
    set(U"Ytick", Value::real(1));
    set(U"Tmin", Value::real(0));
    set(U"Tmax", Value::real(2 * M_PI));
    set(U"Tstep", Value::real(0.1));
    set(U"θmin", Value::real(0));
    set(U"θmax", Value::real(2 * M_PI));
    set(U"θstep", Value::real(0.1));
    // everything else from the command database defaults to 0
    for (const auto &g : m_db.variableGroups())
        for (const auto &n : g.names) {
            std::u32string k = u32(n);
            if (!m_system.count(k) && g.group != "Stałe")
                m_system[k] = Value::real(0);
        }
    // computed on access by built-ins
    m_system.erase(U"Date");
    m_system.erase(U"Time");
    m_systemDefaults = m_system;
}

void Interpreter::loadProgram(const std::u32string &source, const std::u32string &name)
{
    Unit u;
    u.name = name;
    u.source = source;
    u.ast = parseProgram(source, name);
    m_units.push_back(std::move(u));
}

// ---------------------------------------------------------------- formatting / settings

FormatSettings Interpreter::formatSettings() const
{
    FormatSettings fs;
    auto get = [&](const char32_t *n, int def) {
        auto it = m_system.find(n);
        return it != m_system.end() && it->second.isNumber() ? static_cast<int>(it->second.toReal()) : def;
    };
    fs.format = get(U"HFormat", 0);
    fs.digits = get(U"HDigits", 4);
    fs.bitsDefault = get(U"Bits", 32);
    return fs;
}

std::u32string Interpreter::format(const Value &v, bool quoteStrings) const
{
    return formatValue(v, formatSettings(), quoteStrings);
}

int Interpreter::angleMode() const
{
    auto it = m_system.find(U"HAngle");
    return it == m_system.end() ? 0 : static_cast<int>(it->second.toReal());
}

bool Interpreter::complexMode() const
{
    auto it = m_system.find(U"HComplex");
    return it != m_system.end() && it->second.toReal() != 0;
}

double Interpreter::angleToRadians(double a) const
{
    switch (angleMode()) {
    case 1: return a * M_PI / 180.0;
    case 2: return a * M_PI / 200.0;
    default: return a;
    }
}

double Interpreter::radiansToAngle(double r) const
{
    switch (angleMode()) {
    case 1: return r * 180.0 / M_PI;
    case 2: return r * 200.0 / M_PI;
    default: return r;
    }
}

std::u32string Interpreter::currentProgramName() const
{
    int u = currentUnit();
    return u >= 0 ? m_units[u].name : U"Home";
}

void Interpreter::unsupported(const std::u32string &what) const
{
    throw RuntimeError(u8(what) + " — nieobsługiwane w symulatorze (sprawdź na kalkulatorze).", 99);
}

// ---------------------------------------------------------------- variables

int Interpreter::currentUnit() const
{
    for (auto it = m_frames.rbegin(); it != m_frames.rend(); ++it)
        if (it->unit >= 0)
            return it->unit;
    return m_units.empty() ? -1 : 0;
}

Value *Interpreter::findSystem(const std::u32string &name)
{
    auto it = m_system.find(name);
    return it == m_system.end() ? nullptr : &it->second;
}

Value *Interpreter::findVariable(const std::u32string &name)
{
    for (auto it = m_bindings.rbegin(); it != m_bindings.rend(); ++it)
        if (it->first == name)
            return &it->second;
    if (!m_frames.empty()) {
        auto &locals = m_frames.back().locals;
        for (auto it = locals.rbegin(); it != locals.rend(); ++it)
            if (it->first == name)
                return &it->second;
    }
    int u = currentUnit();
    if (u >= 0) {
        initUnit(u);
        for (auto &fv : m_units[u].fileVars)
            if (fv.first == name)
                return &fv.second;
    }
    auto g = m_globals.find(name);
    if (g != m_globals.end())
        return &g->second;
    // qualified: Function.Xmin, PROGRAM.var
    size_t dot = name.rfind(U'.');
    if (dot != std::u32string::npos) {
        std::u32string prefix = name.substr(0, dot), last = name.substr(dot + 1);
        for (size_t k = 0; k < m_units.size(); ++k)
            if (m_units[k].name == prefix) {
                initUnit(static_cast<int>(k));
                for (auto &fv : m_units[k].fileVars)
                    if (fv.first == last)
                        return &fv.second;
            }
        return findSystem(last);
    }
    return findSystem(name);
}

Value Interpreter::getVariable(const std::u32string &name)
{
    if (Value *v = findVariable(name))
        return *v;
    throw RuntimeError("Nieznana nazwa " + u8(name) + ".", 2);
}

void Interpreter::checkSystemType(const std::u32string &name, const Value &v) const
{
    switch (sysKind(name)) {
    case SysKind::Real:
        if (v.type != Value::Type::Real && v.type != Value::Type::Integer)
            throw RuntimeError("Zmienna " + u8(name) + " przechowuje tylko liczby rzeczywiste (próba zapisu: " + typeName(v) + ").");
        break;
    case SysKind::Complex:
        if (!v.isNumber())
            throw RuntimeError("Zmienna " + u8(name) + " przechowuje tylko liczby.");
        break;
    case SysKind::List:
        if (v.type != Value::Type::List)
            throw RuntimeError("Zmienna " + u8(name) + " przechowuje tylko listy (próba zapisu: " + typeName(v) + ").");
        break;
    case SysKind::Matrix:
        if (v.type != Value::Type::Matrix)
            throw RuntimeError("Zmienna " + u8(name) + " przechowuje tylko macierze i wektory.");
        break;
    case SysKind::Graphic:
        throw RuntimeError("Do " + u8(name) + " nie można przypisać wartości; użyj DIMGROB_P, BLIT_P lub SUBGROB_P.");
    default: break;
    }
}

void Interpreter::setVariable(const std::u32string &name, const Value &v, bool create)
{
    for (auto it = m_bindings.rbegin(); it != m_bindings.rend(); ++it)
        if (it->first == name) {
            it->second = v;
            return;
        }
    if (!m_frames.empty()) {
        auto &locals = m_frames.back().locals;
        for (auto it = locals.rbegin(); it != locals.rend(); ++it)
            if (it->first == name) {
                it->second = v;
                return;
            }
    }
    int u = currentUnit();
    if (u >= 0) {
        initUnit(u);
        for (auto &fv : m_units[u].fileVars)
            if (fv.first == name) {
                fv.second = v;
                return;
            }
    }
    auto g = m_globals.find(name);
    if (g != m_globals.end()) {
        g->second = v;
        return;
    }
    std::u32string sysName = name;
    size_t dot = name.rfind(U'.');
    if (dot != std::u32string::npos)
        sysName = name.substr(dot + 1);
    if (Value *s = findSystem(sysName)) {
        checkSystemType(sysName, v);
        Value stored = v;
        if (sysKind(sysName) == SysKind::Equation && v.isNumber())
            stored = Value::string(format(v, false));
        *s = stored;
        return;
    }
    const CommandInfo *c = m_db.findCommand(name);
    if (c && c->kind != "keyword")
        throw RuntimeError("Nie można przypisać wartości do komendy " + c->name + ".");
    if (!create)
        throw RuntimeError("Przypisanie do niezadeklarowanej zmiennej " + u8(name)
                               + ". Zadeklaruj ją przez LOCAL albo EXPORT.",
                           2);
    m_globals[name] = v;
}

Value Interpreter::withBinding(const std::u32string &name, const Value &v, const std::function<Value()> &fn)
{
    m_bindings.push_back({name, v});
    struct Pop {
        std::vector<std::pair<std::u32string, Value>> &b;
        ~Pop() { b.pop_back(); }
    } pop{m_bindings};
    return fn();
}

void Interpreter::initUnit(int unit)
{
    Unit &u = m_units[unit];
    if (u.initialized)
        return;
    u.initialized = true;
    for (const auto &v : u.ast.variables) {
        Value val = Value::real(0);
        if (v.exported) {
            if (!m_globals.count(v.name))
                m_globals[v.name] = val;
        } else {
            u.fileVars.push_back({v.name, val});
        }
    }
    // initialisers are evaluated in the program's context
    Frame f;
    f.unit = unit;
    m_frames.push_back(f);
    try {
        for (const auto &v : u.ast.variables) {
            if (!v.init)
                continue;
            Value val = eval(*v.init);
            setVariable(v.name, val, false);
        }
    } catch (...) {
        m_frames.pop_back();
        throw;
    }
    m_frames.pop_back();
}

std::vector<std::pair<std::u32string, Value>> Interpreter::globalVariables() const
{
    return {m_globals.begin(), m_globals.end()};
}

std::vector<std::pair<std::u32string, Value>> Interpreter::systemVariables(bool onlyChanged) const
{
    std::vector<std::pair<std::u32string, Value>> out;
    for (const auto &[k, v] : m_system) {
        if (onlyChanged) {
            auto d = m_systemDefaults.find(k);
            if (d != m_systemDefaults.end() && valuesEqual(d->second, v))
                continue;
        }
        out.push_back({k, v});
    }
    return out;
}

bool Interpreter::setVariableInFrame(int frameIndex, const std::u32string &name, const Value &v)
{
    if (frameIndex < 0 || frameIndex >= static_cast<int>(m_frames.size()))
        return false;
    for (auto &l : m_frames[frameIndex].locals)
        if (l.first == name) {
            l.second = v;
            return true;
        }
    return false;
}

std::u32string Interpreter::programSource(const std::u32string &name) const
{
    for (const auto &u : m_units)
        if (u.name == name)
            return u.source;
    auto it = m_extraPrograms.find(name);
    return it == m_extraPrograms.end() ? std::u32string() : it->second;
}

void Interpreter::setProgramSource(const std::u32string &name, const std::u32string &src)
{
    if (src.empty())
        m_extraPrograms.erase(name);
    else
        m_extraPrograms[name] = src;
}

std::vector<std::u32string> Interpreter::programNames() const
{
    std::vector<std::u32string> out;
    for (const auto &u : m_units)
        out.push_back(u.name);
    for (const auto &[k, v] : m_extraPrograms)
        out.push_back(k);
    return out;
}

// ---------------------------------------------------------------- running

Value Interpreter::run(const std::u32string &text)
{
    StmtList stmts;
    try {
        stmts = parseStatements(text);
    } catch (const ParseError &e) {
        throw RuntimeError("Błąd składni w wywołaniu: " + e.message);
    }
    m_frames.clear();
    Frame home;
    home.unit = -1;
    m_frames.push_back(home);
    m_last = Value::real(0);
    m_errorLine = m_errorColumn = 0;
    m_errorProgram.clear();
    try {
        execList(stmts);
    } catch (const ReturnSignal &r) {
        m_last = r.value;
    } catch (const RuntimeError &) {
        if (!m_frames.empty() && m_errorLine == 0) {
            m_errorLine = m_frames.back().line;
            m_errorColumn = m_frames.back().column;
            m_errorProgram = currentProgramName();
        }
        m_frames.clear();
        throw;
    } catch (...) {
        m_frames.clear();
        throw;
    }
    m_frames.clear();
    m_system[U"Ans"] = m_last;
    return m_last;
}

Value Interpreter::evaluateInFrame(const std::u32string &text, int frameIndex)
{
    // Temporarily make the requested frame the innermost one.
    if (frameIndex < 0 || frameIndex >= static_cast<int>(m_frames.size()))
        frameIndex = static_cast<int>(m_frames.size()) - 1;
    std::vector<Frame> saved;
    while (static_cast<int>(m_frames.size()) > frameIndex + 1) {
        saved.push_back(std::move(m_frames.back()));
        m_frames.pop_back();
    }
    auto restore = [&]() {
        while (!saved.empty()) {
            m_frames.push_back(std::move(saved.back()));
            saved.pop_back();
        }
    };
    auto savedHook = onStatement;
    onStatement = nullptr;
    Value result;
    try {
        StmtList stmts = parseStatements(text);
        Value lastSaved = m_last;
        for (const auto &s : stmts)
            exec(*s);
        result = m_last;
        m_last = lastSaved;
    } catch (const ParseError &e) {
        onStatement = savedHook;
        restore();
        throw RuntimeError("Błąd składni: " + e.message);
    } catch (...) {
        onStatement = savedHook;
        restore();
        throw;
    }
    onStatement = savedHook;
    restore();
    return result;
}

void Interpreter::tick()
{
    ++m_steps;
    if (m_maxSteps && m_steps > m_maxSteps)
        throw RuntimeError("Przekroczono limit " + std::to_string(m_maxSteps)
                               + " kroków — prawdopodobnie nieskończona pętla (np. czekanie na klawisz, którego nie podano).",
                           98);
    if ((m_steps & 1023) == 0 && m_host.stopRequested())
        throw KillSignal{};
}

void Interpreter::hook(const Stmt &s)
{
    tick();
    if (!m_frames.empty()) {
        m_frames.back().line = s.line;
        m_frames.back().column = s.column;
    }
    if (onStatement)
        onStatement(s, static_cast<int>(m_frames.size()));
}

void Interpreter::execList(const StmtList &list)
{
    for (const auto &s : list)
        exec(*s);
}

void Interpreter::exec(const Stmt &s)
{
    if (s.kind == StmtKind::Block && s.body.empty())
        return; // placeholder of an empty function
    hook(s);
    switch (s.kind) {
    case StmtKind::Expr:
        m_last = eval(*s.expr);
        return;
    case StmtKind::Assign: {
        Value v = eval(*s.value);
        assign(*s.target, v);
        m_last = v;
        return;
    }
    case StmtKind::Local:
        for (size_t i = 0; i < s.names.size(); ++i) {
            Value v = s.inits[i] ? eval(*s.inits[i]) : Value::real(0);
            auto &locals = m_frames.back().locals;
            bool found = false;
            for (auto &l : locals)
                if (l.first == s.names[i]) {
                    l.second = v;
                    found = true;
                }
            if (!found)
                locals.push_back({s.names[i], v});
        }
        return;
    case StmtKind::If:
        if (eval(*s.expr).truthy())
            execList(s.body);
        else
            execList(s.elseBody);
        return;
    case StmtKind::Case:
        for (const auto &b : s.branches) {
            if (eval(*b.condition).truthy()) {
                execList(b.body);
                return;
            }
        }
        if (s.hasDefault)
            execList(s.elseBody);
        return;
    case StmtKind::IfErr: {
        bool failed = false;
        size_t depth = m_frames.size();
        size_t bindings = m_bindings.size();
        try {
            execList(s.body);
        } catch (const RuntimeError &e) {
            failed = true;
            m_frames.resize(depth);
            m_bindings.resize(bindings);
            m_system[U"Ans"] = Value::real(e.code);
        }
        if (failed)
            execList(s.elseBody);
        else
            execList(s.thirdBody);
        return;
    }
    case StmtKind::For: {
        const std::u32string &var = s.target->text;
        Value start = eval(*s.expr);
        double end = eval(*s.value).toReal("FOR");
        double step = s.step ? eval(*s.step).toReal("STEP") : 1;
        if (step == 0)
            throw RuntimeError("Krok pętli FOR nie może być zerem.");
        setVariable(var, start, false);
        while (true) {
            tick();
            double cur = getVariable(var).toReal("FOR");
            if (s.down ? cur < end : cur > end)
                break;
            try {
                execList(s.body);
            } catch (BreakSignal &b) {
                if (--b.levels > 0)
                    throw;
                break;
            } catch (ContinueSignal &) {
            }
            double next = getVariable(var).toReal("FOR") + (s.down ? -step : step);
            setVariable(var, Value::real(next), false);
        }
        return;
    }
    case StmtKind::While:
        while ((tick(), eval(*s.expr).truthy())) {
            try {
                execList(s.body);
            } catch (BreakSignal &b) {
                if (--b.levels > 0)
                    throw;
                break;
            } catch (ContinueSignal &) {
            }
        }
        return;
    case StmtKind::Repeat:
        while (true) {
            tick();
            try {
                execList(s.body);
            } catch (BreakSignal &b) {
                if (--b.levels > 0)
                    throw;
                break;
            } catch (ContinueSignal &) {
            }
            if (eval(*s.expr).truthy())
                break;
        }
        return;
    case StmtKind::Break: {
        int levels = s.expr ? eval(*s.expr).toInt("BREAK") : 1;
        throw BreakSignal{std::max(1, levels)};
    }
    case StmtKind::Continue:
        throw ContinueSignal{};
    case StmtKind::Kill:
        throw KillSignal{};
    case StmtKind::Return:
        throw ReturnSignal{s.expr ? eval(*s.expr) : m_last};
    case StmtKind::Block:
        execList(s.body);
        return;
    }
}

// ---------------------------------------------------------------- functions

bool Interpreter::findFunction(const std::u32string &name, int &unit, const FunctionDef *&fn) const
{
    int cu = currentUnit();
    std::u32string n = name;
    int onlyUnit = -1;
    size_t dot = name.rfind(U'.');
    if (dot != std::u32string::npos) {
        std::u32string prefix = name.substr(0, dot);
        n = name.substr(dot + 1);
        for (size_t k = 0; k < m_units.size(); ++k)
            if (m_units[k].name == prefix)
                onlyUnit = static_cast<int>(k);
        if (onlyUnit < 0)
            return false;
    }
    auto search = [&](int k, bool requireExport) -> bool {
        for (const auto &f : m_units[k].ast.functions) {
            if (f->name == n && !f->body.empty() && (!requireExport || f->exported)) {
                unit = k;
                fn = f.get();
                return true;
            }
        }
        return false;
    };
    if (onlyUnit >= 0)
        return search(onlyUnit, false);
    if (cu >= 0 && search(cu, false))
        return true;
    for (size_t k = 0; k < m_units.size(); ++k)
        if (static_cast<int>(k) != cu && search(static_cast<int>(k), true))
            return true;
    return false;
}

Value Interpreter::callFunction(int unit, const FunctionDef &fn, std::vector<Value> args)
{
    if (args.size() != fn.params.size())
        throw RuntimeError("Funkcja " + u8(fn.name) + " oczekuje " + std::to_string(fn.params.size())
                           + " argument(ów), podano " + std::to_string(args.size()) + ".");
    if (m_frames.size() > 900)
        throw RuntimeError("Zbyt głębokie wywołania (rekurencja bez końca?).");
    initUnit(unit);
    Frame f;
    f.function = &fn;
    f.unit = unit;
    f.line = fn.line;
    for (size_t i = 0; i < args.size(); ++i)
        f.locals.push_back({fn.params[i], args[i]});
    m_frames.push_back(std::move(f));
    size_t bindings = m_bindings.size();
    Value result;
    Value savedLast = m_last;
    m_last = Value::real(0);
    try {
        execList(fn.body);
        result = m_last;
    } catch (ReturnSignal &r) {
        result = r.value;
    } catch (BreakSignal &) {
        m_frames.pop_back();
        throw RuntimeError("BREAK poza pętlą.");
    } catch (ContinueSignal &) {
        m_frames.pop_back();
        throw RuntimeError("CONTINUE poza pętlą.");
    } catch (const RuntimeError &) {
        // record the innermost location only
        if (m_errorLine == 0) {
            m_errorLine = m_frames.back().line;
            m_errorColumn = m_frames.back().column;
            m_errorProgram = m_units[unit].name;
        }
        m_frames.pop_back();
        m_bindings.resize(bindings);
        throw;
    } catch (...) {
        m_frames.pop_back();
        m_bindings.resize(bindings);
        throw;
    }
    m_frames.pop_back();
    m_bindings.resize(bindings);
    m_last = savedLast;
    return result;
}

Value Interpreter::callBuiltin(const std::u32string &name, const Builtin &b, const std::vector<ExprPtr> &args)
{
    int n = static_cast<int>(args.size());
    if (n < b.minArgs || (b.maxArgs >= 0 && n > b.maxArgs)) {
        std::string range = b.maxArgs < 0 ? "co najmniej " + std::to_string(b.minArgs)
                            : b.minArgs == b.maxArgs ? std::to_string(b.minArgs)
                                                     : std::to_string(b.minArgs) + "–" + std::to_string(b.maxArgs);
        throw RuntimeError(u8(name) + " przyjmuje " + range + " argument(ów), podano " + std::to_string(n) + ".");
    }
    CallArgs ca{*this, name, args, {}};
    if (!b.lazy) {
        ca.values.reserve(args.size());
        for (const auto &a : args)
            ca.values.push_back(a ? eval(*a) : Value::real(0));
    }
    return b.fn(ca);
}

Value Interpreter::call(const Expr &e)
{
    const std::u32string &name = e.text;

    // CAS.name(...) and App.name(...)
    size_t dot = name.find(U'.');
    std::u32string plain = name;
    if (dot != std::u32string::npos) {
        std::u32string prefix = name.substr(0, dot);
        std::u32string rest = name.substr(dot + 1);
        int u;
        const FunctionDef *fn;
        if (asciiUpper(prefix) == U"CAS") {
            auto it = m_builtins.find(U"CAS." + asciiUpper(rest));
            if (it == m_builtins.end())
                it = m_builtins.find(asciiUpper(rest));
            if (it == m_builtins.end())
                unsupported(U"CAS." + rest);
            return callBuiltin(rest, it->second, e.args);
        }
        if (findFunction(name, u, fn)) {
            std::vector<Value> args;
            for (const auto &a : e.args)
                args.push_back(a ? eval(*a) : Value::real(0));
            return callFunction(u, *fn, std::move(args));
        }
        plain = rest; // App.function / App.variable
        std::u32string appName = prefix;
        std::replace(appName.begin(), appName.end(), U'_', U' ');
        if (m_db.isAppName(u8(appName))) {
            auto it = m_builtins.find(asciiUpper(rest));
            if (it != m_builtins.end()) {
                std::u32string saved = m_app.current;
                m_app.current = appName;
                try {
                    Value r = callBuiltin(rest, it->second, e.args);
                    m_app.current = saved;
                    return r;
                } catch (...) {
                    m_app.current = saved;
                    throw;
                }
            }
        }
    }

    // variable being indexed (list, matrix, string) or an app function like F1(2)
    if (Value *v = findVariable(name)) {
        Value target = *v;
        if (target.type == Value::Type::String && sysKind(plain) == SysKind::Equation && !e.args.empty()) {
            if (e.args.size() == 1 && e.args[0] && e.args[0]->kind == ExprKind::Ident && e.args[0]->text == U"COLOR") {
                auto it = m_app.colors.find(plain);
                return it == m_app.colors.end() ? Value::integer(0, 'h') : it->second;
            }
            std::u32string var = plain[0] == U'R' ? U"θ" : (plain[0] == U'X' || plain[0] == U'Y') ? U"T"
                                 : plain[0] == U'U' ? U"N" : U"X";
            Value x = eval(*e.args[0]);
            if (e.args.size() == 2 && (plain[0] == U'V' || (plain.size() > 1 && plain[1] == U'Z'))) {
                Value y = eval(*e.args[1]);
                return withBinding(U"X", x, [&] { return withBinding(U"Y", y, [&] { return evalText(target.str); }); });
            }
            return withBinding(var, x, [&] { return evalText(target.str); });
        }
        if (target.type == Value::Type::List || target.type == Value::Type::Matrix || target.type == Value::Type::String) {
            std::vector<Value> idx;
            for (const auto &a : e.args)
                idx.push_back(a ? eval(*a) : Value::real(0));
            return index(target, idx, name);
        }
    }

    int u;
    const FunctionDef *fn;
    if (findFunction(plain, u, fn)) {
        std::vector<Value> args;
        for (const auto &a : e.args)
            args.push_back(a ? eval(*a) : Value::real(0));
        return callFunction(u, *fn, std::move(args));
    }
    auto it = m_builtins.find(asciiUpper(plain));
    if (it != m_builtins.end())
        return callBuiltin(plain, it->second, e.args);
    if (m_db.findCommand(plain))
        unsupported(plain);
    throw RuntimeError("Nieznana funkcja " + u8(name)
                           + ". Jeśli pochodzi z innego programu, umieść jego plik w tym samym folderze.",
                       2);
}

// ---------------------------------------------------------------- indexing

Value Interpreter::index(const Value &target, const std::vector<Value> &idx, const std::u32string &what)
{
    if (idx.empty())
        return target;
    const Value &first = idx[0];
    std::vector<Value> rest(idx.begin() + 1, idx.end());
    auto badIndex = [&](int i, int n) {
        return RuntimeError("Indeks " + std::to_string(i) + " poza zakresem (" + u8(what) + " ma " + std::to_string(n)
                            + " element(ów)).");
    };
    switch (target.type) {
    case Value::Type::List: {
        const auto &l = target.items();
        if (first.type == Value::Type::List) {
            const auto &r = first.items();
            if (r.size() != 2)
                throw RuntimeError("Podlista wymaga {początek, koniec}.");
            int a = r[0].toInt(), b = r[1].toInt();
            ValueList sub;
            for (int k = std::max(1, a); k <= std::min<int>(b, static_cast<int>(l.size())); ++k)
                sub.push_back(l[k - 1]);
            return index(Value::makeList(sub), rest, what);
        }
        int i = first.toInt("indeks");
        if (i < 1 || i > static_cast<int>(l.size()))
            throw badIndex(i, static_cast<int>(l.size()));
        return index(l[i - 1], rest, what);
    }
    case Value::Type::Matrix: {
        const Matrix &m = *target.mat;
        if (m.isVector) {
            int i = first.toInt("indeks");
            if (i < 1 || i > m.cols)
                throw badIndex(i, m.cols);
            return index(m.at(0, i - 1), rest, what);
        }
        int r = first.toInt("wiersz");
        if (r < 1 || r > m.rows)
            throw badIndex(r, m.rows);
        if (rest.empty()) {
            ValueList row;
            for (int c = 0; c < m.cols; ++c)
                row.push_back(m.at(r - 1, c));
            return Value::vector(row);
        }
        int c = rest[0].toInt("kolumna");
        if (c < 1 || c > m.cols)
            throw badIndex(c, m.cols);
        std::vector<Value> more(rest.begin() + 1, rest.end());
        return index(m.at(r - 1, c - 1), more, what);
    }
    case Value::Type::String: {
        int i = first.toInt("indeks");
        if (i < 1 || i > static_cast<int>(target.str.size()))
            throw badIndex(i, static_cast<int>(target.str.size()));
        return Value::real(static_cast<double>(target.str[i - 1]));
    }
    default:
        throw RuntimeError("Nie można indeksować wartości typu " + typeName(target) + " (" + u8(what) + ").");
    }
}

void Interpreter::assignIndexed(Value &target, const std::vector<Value> &idx, const Value &v)
{
    if (idx.empty()) {
        target = v;
        return;
    }
    std::vector<Value> rest(idx.begin() + 1, idx.end());
    if (target.type == Value::Type::List) {
        ValueList &l = target.mutableList();
        int i = idx[0].toInt("indeks");
        if (i == static_cast<int>(l.size()) + 1 || (i == 0 && rest.empty())) {
            l.push_back(Value::real(0));
            i = static_cast<int>(l.size());
        }
        if (i < 1 || i > static_cast<int>(l.size()))
            throw RuntimeError("Indeks " + std::to_string(i) + " poza zakresem listy (" + std::to_string(l.size()) + " elementów).");
        assignIndexed(l[i - 1], rest, v);
        return;
    }
    if (target.type == Value::Type::Matrix) {
        Matrix &m = target.mutableMatrix();
        if (m.isVector) {
            int i = idx[0].toInt("indeks");
            if (i == m.cols + 1) {
                m.data.push_back(Value::real(0));
                m.cols++;
            }
            if (i < 1 || i > m.cols)
                throw RuntimeError("Indeks " + std::to_string(i) + " poza zakresem wektora.");
            m.at(0, i - 1) = v;
            return;
        }
        int r = idx[0].toInt("wiersz");
        if (r < 1 || r > m.rows)
            throw RuntimeError("Wiersz " + std::to_string(r) + " poza zakresem macierzy.");
        if (rest.empty()) {
            if (v.type != Value::Type::Matrix || v.mat->rows != 1 || v.mat->cols != m.cols)
                throw RuntimeError("Zapis całego wiersza wymaga wektora o " + std::to_string(m.cols) + " elementach.");
            for (int c = 0; c < m.cols; ++c)
                m.at(r - 1, c) = v.mat->at(0, c);
            return;
        }
        int c = rest[0].toInt("kolumna");
        if (c < 1 || c > m.cols)
            throw RuntimeError("Kolumna " + std::to_string(c) + " poza zakresem macierzy.");
        if (!v.isNumber())
            throw RuntimeError("Elementem macierzy może być tylko liczba.");
        m.at(r - 1, c - 1) = v;
        return;
    }
    throw RuntimeError("Nie można zapisać elementu w wartości typu " + typeName(target) + ".");
}

void Interpreter::assign(const Expr &target, const Value &v)
{
    if (target.kind == ExprKind::Ident) {
        bool home = !m_frames.empty() && m_frames.back().function == nullptr && m_frames.back().unit < 0;
        setVariable(target.text, v, home);
        return;
    }
    if (target.kind == ExprKind::Call) {
        const std::u32string &name = target.text;
        std::u32string up = asciiUpper(name);
        std::vector<Value> idx;
        for (const auto &a : target.args)
            idx.push_back(a ? eval(*a) : Value::real(0));
        // accessor variables: Notes("x") := …, Programs(…), HVars(…), AVars(…)
        if (up == U"NOTES" || up == U"PROGRAMS" || up == U"HVARS" || up == U"AVARS" || up == U"AFILES") {
            if (idx.empty())
                throw RuntimeError(u8(name) + ": podaj nazwę.");
            std::u32string key = idx[0].type == Value::Type::String ? idx[0].str : format(idx[0], false);
            if (up == U"NOTES") {
                if (v.type != Value::Type::String)
                    throw RuntimeError("Notatka musi być tekstem.");
                if (v.str.empty())
                    m_notes.erase(key);
                else
                    m_notes[key] = v.str;
            } else if (up == U"PROGRAMS") {
                if (v.type != Value::Type::String)
                    throw RuntimeError("Kod programu musi być tekstem.");
                setProgramSource(key, v.str);
            } else if (up == U"HVARS") {
                m_globals[key] = v;
            } else {
                m_avars[key] = v;
            }
            return;
        }
        // F1(COLOR) := …
        if (idx.size() == 1 && target.args[0] && target.args[0]->kind == ExprKind::Ident && target.args[0]->text == U"COLOR") {
            std::u32string key = name.substr(name.rfind(U'.') == std::u32string::npos ? 0 : name.rfind(U'.') + 1);
            m_app.colors[key] = v;
            return;
        }
        Value *var = findVariable(name);
        if (!var)
            throw RuntimeError("Nieznana zmienna " + u8(name) + ".", 2);
        Value copy = *var;
        assignIndexed(copy, idx, v);
        setVariable(name, copy, false);
        return;
    }
    throw RuntimeError("Nie można przypisać wartości do tego wyrażenia.");
}

// ---------------------------------------------------------------- expressions

Value Interpreter::parseInteger(const std::u32string &text) const
{
    std::u32string body = text.substr(1);
    int bits = 0;
    char base = 0;
    size_t colon = body.find(U':');
    std::u32string digits = body;
    if (colon != std::u32string::npos) {
        digits = body.substr(0, colon);
        std::u32string b = body.substr(colon + 1);
        if (!b.empty() && !isDigit(b.back())) {
            base = static_cast<char>(std::tolower(static_cast<int>(b.back())));
            b.pop_back();
        }
        bits = std::atoi(u8(b).c_str());
    }
    if (!base && !digits.empty()) {
        char32_t last = digits.back();
        char32_t l = last >= U'A' && last <= U'Z' ? last + 32 : last;
        if (l == U'h' || l == U'o' || (l == U'b' && digits.size() > 1) || (l == U'd' && digits.size() > 1)) {
            // #1101b → binary; #FFh → hex; a trailing b/d that is a valid hex digit is treated as a suffix
            base = static_cast<char>(l);
            digits.pop_back();
        }
    }
    if (!base) {
        auto it = m_system.find(U"Base");
        int b = it == m_system.end() ? 3 : static_cast<int>(it->second.toReal());
        base = b == 0 ? 'b' : b == 1 ? 'o' : b == 2 ? 'd' : 'h';
    }
    int radix = base == 'b' ? 2 : base == 'o' ? 8 : base == 'd' ? 10 : 16;
    uint64_t v = 0;
    for (char32_t c : digits) {
        int d = c >= U'0' && c <= U'9' ? c - U'0' : (c | 32) >= U'a' && (c | 32) <= U'f' ? (c | 32) - U'a' + 10 : 99;
        if (d >= radix)
            throw RuntimeError("Niepoprawna cyfra w liczbie " + u8(text) + ".");
        v = v * radix + d;
    }
    return Value::integer(static_cast<int64_t>(v), base, bits);
}

Value Interpreter::evalText(const std::u32string &text)
{
    auto it = m_exprCache.find(text);
    ExprPtr e;
    if (it != m_exprCache.end()) {
        e = it->second;
    } else {
        try {
            e = parseExpression(text);
        } catch (const ParseError &pe) {
            throw RuntimeError("Błąd składni w wyrażeniu \"" + u8(text) + "\": " + pe.message);
        }
        if (m_exprCache.size() > 4096)
            m_exprCache.clear();
        m_exprCache[text] = e;
    }
    return eval(*e);
}

double Interpreter::evalFunctionOf(const std::u32string &exprText, const std::u32string &var, double x)
{
    FullPrecision fp;
    return withBinding(var, Value::real(x), [&] { return evalText(exprText); }).toReal("funkcja");
}

Value Interpreter::eval(const Expr &e)
{
    switch (e.kind) {
    case ExprKind::Number:
        if (e.text.find(U'_') != std::u32string::npos)
            unsupported(U"Jednostki (" + e.text + U")");
        return Value::real(e.number);
    case ExprKind::Integer: return parseInteger(e.text);
    case ExprKind::String: return Value::string(e.text);
    case ExprKind::Quoted: return Value::symbolic(e.text);
    case ExprKind::Ident: {
        const std::u32string &n = e.text;
        if (Value *v = findVariable(n)) {
            if (v->type == Value::Type::Symbolic)
                return *v;
            return *v;
        }
        if (n == U"π" || n == U"PI") return Value::real(M_PI);
        if (n == U"e") return Value::real(M_E);
        if (n == U"i") return Value::complex(0, 1);
        if (n == U"∞") return Value::real(INFINITY);
        if (n == U"MAXREAL") return Value::real(9.99999999999e499 > 1e308 ? 1.79769313486e308 : 9.99999999999e499);
        if (n == U"MINREAL") return Value::real(1e-307);
        // command without parentheses: GETKEY, TICKS, RANDOM, FREEZE, WAIT, MOUSE …
        Expr callExpr = e;
        callExpr.kind = ExprKind::Call;
        int u;
        const FunctionDef *fn;
        std::u32string plain = n.substr(n.rfind(U'.') == std::u32string::npos ? 0 : n.rfind(U'.') + 1);
        if (findFunction(n, u, fn) || m_builtins.count(asciiUpper(plain)) || asciiUpper(n).rfind(U"CAS.", 0) == 0)
            return call(callExpr);
        throw RuntimeError("Nieznana nazwa " + u8(n) + ".", 2);
    }
    case ExprKind::Call: return call(e);
    case ExprKind::Index: {
        Value target = eval(*e.args[0]);
        std::vector<Value> idx;
        for (size_t i = 1; i < e.args.size(); ++i)
            idx.push_back(e.args[i] ? eval(*e.args[i]) : Value::real(0));
        return index(target, idx, U"wyrażenie");
    }
    case ExprKind::List: {
        ValueList items;
        for (const auto &a : e.args)
            items.push_back(eval(*a));
        return Value::makeList(std::move(items));
    }
    case ExprKind::Matrix: {
        std::vector<Value> items;
        for (const auto &a : e.args)
            items.push_back(eval(*a));
        bool rows = !items.empty() && std::all_of(items.begin(), items.end(), [](const Value &v) {
            return v.type == Value::Type::Matrix && v.mat->isVector;
        });
        if (rows) {
            int cols = items[0].mat->cols;
            Value m = Value::matrix(static_cast<int>(items.size()), cols);
            for (size_t r = 0; r < items.size(); ++r) {
                if (items[r].mat->cols != cols)
                    throw RuntimeError("Wiersze macierzy mają różne długości.");
                for (int c = 0; c < cols; ++c)
                    m.mat->at(static_cast<int>(r), c) = items[r].mat->at(0, c);
            }
            return m;
        }
        for (const auto &v : items)
            if (!v.isNumber())
                throw RuntimeError("Elementami wektora mogą być tylko liczby.");
        return Value::vector(items);
    }
    case ExprKind::Tuple: {
        if (e.args.size() != 2)
            throw RuntimeError("Nawias z " + std::to_string(e.args.size()) + " elementami: oczekiwano punktu (x,y).");
        double x = eval(*e.args[0]).toReal("punkt"), y = eval(*e.args[1]).toReal("punkt");
        return normalizeComplex({x, y});
    }
    case ExprKind::Unary: return unaryOp(e.text, eval(*e.args[0]));
    case ExprKind::Binary: {
        const std::u32string &op = e.text;
        if (op == U"AND") {
            if (!eval(*e.args[0]).truthy())
                return Value::real(0);
            return Value::real(eval(*e.args[1]).truthy() ? 1 : 0);
        }
        if (op == U"OR") {
            if (eval(*e.args[0]).truthy())
                return Value::real(1);
            return Value::real(eval(*e.args[1]).truthy() ? 1 : 0);
        }
        return binary(op, eval(*e.args[0]), eval(*e.args[1]));
    }
    case ExprKind::Postfix: {
        Value a = eval(*e.args[0]);
        const std::u32string &op = e.text;
        if (op == U"!") {
            double x = a.toReal("!");
            if (x < 0 || x != std::floor(x))
                return Value::real(std::tgamma(x + 1));
            double r = 1;
            for (int k = 2; k <= static_cast<int>(x); ++k)
                r *= k;
            return Value::real(r);
        }
        if (op == U"%") return binary(U"/", a, Value::real(100));
        if (op == U"²") return binary(U"^", a, Value::real(2));
        if (op == U"³") return binary(U"^", a, Value::real(3));
        if (op == U"⁻¹") return binary(U"^", a, Value::real(-1));
        if (op == U"°") return Value::real(radiansToAngle(a.toReal("°") * M_PI / 180.0));
        unsupported(U"Operator " + op);
    }
    case ExprKind::Where: {
        // expr | X=value  or  expr | {X=a, Y=b}
        std::vector<std::pair<std::u32string, Value>> binds;
        auto addBinding = [&](const Expr &eq) {
            if (eq.kind != ExprKind::Binary || (eq.text != U"=" && eq.text != U"==") || eq.args[0]->kind != ExprKind::Ident)
                throw RuntimeError("Po '|' oczekiwano podstawienia w postaci zmienna=wartość.");
            binds.push_back({eq.args[0]->text, eval(*eq.args[1])});
        };
        const Expr &rhs = *e.args[1];
        if (rhs.kind == ExprKind::List)
            for (const auto &a : rhs.args)
                addBinding(*a);
        else
            addBinding(rhs);
        size_t base = m_bindings.size();
        for (auto &b : binds)
            m_bindings.push_back(b);
        Value r;
        try {
            r = eval(*e.args[0]);
        } catch (...) {
            m_bindings.resize(base);
            throw;
        }
        m_bindings.resize(base);
        return r;
    }
    case ExprKind::Store: {
        Value v = eval(*e.args[0]);
        assign(*e.args[1], v);
        return v;
    }
    }
    return Value::real(0);
}

Value Interpreter::unaryOp(const std::u32string &op, const Value &a)
{
    if (op == U"NOT")
        return Value::real(a.truthy() ? 0 : 1);
    if (op == U"-") {
        switch (a.type) {
        case Value::Type::Real: return Value::real(-a.re);
        case Value::Type::Integer: return Value::integer(-a.ival, a.base, a.bits);
        case Value::Type::Complex: return Value::complex(-a.re, -a.im);
        default: return arith(U"*", Value::real(-1), a, *this);
        }
    }
    if (op == U"√") {
        if (a.type == Value::Type::List || a.type == Value::Type::Matrix)
            return arith(U"^", a, Value::real(0.5), *this);
        if (a.type == Value::Type::Complex)
            return normalizeComplex(std::sqrt(a.toComplex()));
        double x = a.toReal("√");
        if (x < 0) {
            if (!complexMode())
                throw RuntimeError("√ z liczby ujemnej: wynik zespolony. Włącz HComplex:=1.");
            return normalizeComplex(std::sqrt(std::complex<double>(x, 0)));
        }
        return Value::real(std::sqrt(x));
    }
    unsupported(U"Operator " + op);
}

Value Interpreter::binary(const std::u32string &op, const Value &a, const Value &b)
{
    return arith(op, a, b, *this);
}

} // namespace ppl
