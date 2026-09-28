// Built-in commands: HP apps (Function, Solve, Statistics, Finance, Explorer…), system accessors, plotting.
#include "builtins.h"
#include "interpreter.h"
#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <ctime>

namespace ppl {

namespace {

std::string u8(const std::u32string &s) { return toUtf8(s); }

std::u32string equationPrefix(const std::u32string &app)
{
    if (app == U"Polar") return U"R";
    if (app == U"Parametric") return U"X";
    if (app == U"Sequence") return U"U";
    if (app == U"Advanced Graphing") return U"V";
    if (app == U"Graph 3D") return U"FZ";
    if (app == U"Solve") return U"E";
    if (app == U"Statistics 1Var") return U"H";
    if (app == U"Statistics 2Var") return U"S";
    return U"F";
}

// Name of an equation variable passed as argument (F1, E2 …) and its expression text.
std::pair<std::u32string, std::u32string> equationArg(CallArgs &a, size_t i)
{
    const Expr &e = *a.exprs[i];
    if (e.kind == ExprKind::Ident) {
        Value v = a.in.getVariable(e.text);
        if (v.type == Value::Type::String || v.type == Value::Type::Symbolic) {
            if (v.str.empty())
                throw RuntimeError(u8(a.name) + ": " + u8(e.text) + " nie jest zdefiniowane (przypisz np. " + u8(e.text)
                                   + " := \"X^2-2\").");
            return {e.text, v.str};
        }
    }
    // expression given directly, e.g. SOLVE(X^2-X-2, X, 3)
    if (e.kind == ExprKind::String || e.kind == ExprKind::Quoted)
        return {U"", e.text};
    return {U"", U""};
}

// lhs=rhs → lhs-(rhs)
std::u32string toZeroForm(const std::u32string &eq)
{
    int depth = 0;
    for (size_t i = 0; i < eq.size(); ++i) {
        char32_t c = eq[i];
        if (c == U'(' || c == U'[' || c == U'{') ++depth;
        if (c == U')' || c == U']' || c == U'}') --depth;
        if (c == U'=' && depth == 0 && (i == 0 || (eq[i - 1] != U'<' && eq[i - 1] != U'>' && eq[i - 1] != U'='))
            && (i + 1 >= eq.size() || eq[i + 1] != U'='))
            return U"(" + eq.substr(0, i) + U")-(" + eq.substr(i + 1) + U")";
    }
    return eq;
}

ValueList listOrEmpty(Interpreter &in, const std::u32string &name)
{
    Value *v = in.findVariable(name);
    if (!v || !v->isList())
        return {};
    return v->items();
}

void setSys(Interpreter &in, const char32_t *name, double v) { in.setVariable(name, Value::real(v), true); }

// ---- TVM: PV·(1+i)^n + PMT·(1+i·b)·((1+i)^n − 1)/i + FV = 0
struct Tvm {
    double n, ipyr, pv, pmt, fv, ppyr = 12, cpyr = 12;
    bool beg = false;
    double rate() const
    {
        double nominal = ipyr / 100.0;
        return std::pow(1 + nominal / cpyr, cpyr / ppyr) - 1;
    }
    double residual() const
    {
        double i = rate();
        if (std::fabs(i) < 1e-14)
            return pv + pmt * n + fv;
        double g = std::pow(1 + i, n);
        return pv * g + pmt * (1 + i * (beg ? 1 : 0)) * (g - 1) / i + fv;
    }
};

Tvm tvmFrom(CallArgs &a, const char *order)
{
    // order: letters for the first four arguments, e.g. "NIVF" = NbPmt, IPYR, PV, FV
    Tvm t{0, 0, 0, 0, 0};
    for (int k = 0; k < 4; ++k) {
        double v = a.num(k);
        switch (order[k]) {
        case 'N': t.n = v; break;
        case 'I': t.ipyr = v; break;
        case 'V': t.pv = v; break;
        case 'P': t.pmt = v; break;
        case 'F': t.fv = v; break;
        }
    }
    if (a.size() > 4) t.ppyr = a.num(4);
    t.cpyr = a.size() > 5 ? a.num(5) : t.ppyr;
    if (a.size() > 6) t.beg = a.num(6) != 0;
    return t;
}

double solveTvm(Tvm t, char unknown)
{
    auto f = [&](double x) {
        Tvm c = t;
        switch (unknown) {
        case 'N': c.n = x; break;
        case 'I': c.ipyr = x; break;
        case 'V': c.pv = x; break;
        case 'P': c.pmt = x; break;
        case 'F': c.fv = x; break;
        }
        return c.residual();
    };
    if (unknown == 'V' || unknown == 'P' || unknown == 'F') {
        // linear in these variables
        double f0 = f(0), f1 = f(1);
        return -f0 / (f1 - f0);
    }
    return findRoot(f, unknown == 'I' ? 5 : 12);
}

long long daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

void dateParts(double v, int &y, int &m, int &d)
{
    y = static_cast<int>(std::floor(v));
    double frac = std::round((v - y) * 10000);
    m = static_cast<int>(frac) / 100;
    d = static_cast<int>(frac) % 100;
}

void plotApp(Interpreter &in)
{
    Graphics &g = in.graphics();
    const std::u32string app = in.app().current;
    double xmin = in.getVariable(U"Xmin").toReal(), xmax = in.getVariable(U"Xmax").toReal();
    double ymin = in.getVariable(U"Ymin").toReal(), ymax = in.getVariable(U"Ymax").toReal();
    auto px = [&](double x) { return (x - xmin) / (xmax - xmin) * 318; };
    auto py = [&](double y) { return (ymax - y) / (ymax - ymin) * 218; };
    g.rect(0, 0, 0, 319, 239, 0xFFFFFF, 0xFFFFFF, true);
    if (xmin < 0 && xmax > 0)
        g.line(0, px(0), 0, px(0), 218, 0x808080);
    if (ymin < 0 && ymax > 0)
        g.line(0, 0, py(0), 318, py(0), 0x808080);
    static const uint32_t palette[10] = {0xE00000, 0x0000E0, 0x00A000, 0xFF8000, 0x8000A0, 0x008080, 0x804000, 0x606060, 0xC000C0, 0x000000};
    int drawn = 0;
    for (int k = 0; k <= 9; ++k) {
        std::u32string d = toU32(std::to_string(k));
        std::u32string prefix = equationPrefix(app);
        std::u32string key = prefix + d;
        if (!in.app().checked[key])
            continue;
        uint32_t color = palette[k];
        auto cit = in.app().colors.find(key);
        if (cit != in.app().colors.end() && cit->second.isNumber())
            color = static_cast<uint32_t>(static_cast<int64_t>(cit->second.toReal())) & 0xFFFFFF;
        auto evalAt = [&](const std::u32string &name, const std::u32string &var, double t) -> double {
            Value e = in.getVariable(name);
            if (!e.isString() || e.str.empty())
                throw RuntimeError("");
            return in.evalFunctionOf(e.str, var, t);
        };
        try {
            if (prefix == U"F") {
                double prevY = NAN;
                for (int sx = 0; sx <= 318; ++sx) {
                    double x = xmin + sx * (xmax - xmin) / 318;
                    double y;
                    try { y = evalAt(key, U"X", x); } catch (...) { prevY = NAN; continue; }
                    if (std::isfinite(prevY) && std::fabs(py(y) - py(prevY)) < 400)
                        g.line(0, sx - 1, py(prevY), sx, py(y), color);
                    prevY = y;
                }
            } else if (prefix == U"X" || prefix == U"R") {
                double tmin = in.getVariable(prefix == U"X" ? U"Tmin" : U"θmin").toReal();
                double tmax = in.getVariable(prefix == U"X" ? U"Tmax" : U"θmax").toReal();
                int steps = 600;
                double lx = NAN, ly = NAN;
                for (int s = 0; s <= steps; ++s) {
                    double t = tmin + (tmax - tmin) * s / steps, x, y;
                    try {
                        if (prefix == U"X") {
                            x = evalAt(U"X" + d, U"T", t);
                            y = evalAt(U"Y" + d, U"T", t);
                        } else {
                            double r = evalAt(key, U"θ", t), ang = in.angleToRadians(t);
                            x = r * std::cos(ang);
                            y = r * std::sin(ang);
                        }
                    } catch (...) { lx = NAN; continue; }
                    if (std::isfinite(lx))
                        g.line(0, px(lx), py(ly), px(x), py(y), color);
                    lx = x;
                    ly = y;
                }
            } else {
                continue;
            }
            ++drawn;
        } catch (const RuntimeError &) {
        }
    }
    if (drawn == 0)
        in.host().notice(U"[STARTVIEW] Wykres aplikacji " + app + U": brak zaznaczonych funkcji albo ten typ wykresu nie jest symulowany.");
    in.host().screenChanged();
}

} // namespace

void registerAppBuiltins(Interpreter &, BuiltinTable &t)
{
    // ------------------------------------------------------------------ app control
    t[U"STARTAPP"] = {1, 1, false, [](CallArgs &a) {
                          std::u32string name = a.text(0);
                          if (!a.in.db().isAppName(u8(name)))
                              throw RuntimeError("Nieznana aplikacja \"" + u8(name) + "\".");
                          a.in.app().current = name;
                          return Value::real(0);
                      }};
    t[U"STARTVIEW"] = {1, 2, false, [](CallArgs &a) {
                           int v = a.integer(0);
                           if (v == 1) {
                               plotApp(a.in);
                           } else if (v != -1) {
                               a.in.host().notice(U"[STARTVIEW(" + toU32(std::to_string(v)) + U")] Widok aplikacji "
                                                  + a.in.app().current + U" nie jest symulowany.");
                           }
                           return Value::real(0);
                       }};
    auto check = [](bool on) {
        return Builtin{1, 1, false, [on](CallArgs &a) {
                           int n = a.integer(0);
                           std::u32string key = equationPrefix(a.in.app().current) + toU32(std::to_string(n));
                           a.in.app().checked[key] = on;
                           return Value::real(0);
                       }};
    };
    t[U"CHECK"] = check(true);
    t[U"UNCHECK"] = check(false);
    t[U"ISCHECK"] = {1, 1, false, [](CallArgs &a) {
                         std::u32string key = equationPrefix(a.in.app().current) + toU32(std::to_string(a.integer(0)));
                         return Value::real(a.in.app().checked[key] ? 1 : 0);
                     }};

    // ------------------------------------------------------------------ Function app
    auto fnOf = [](CallArgs &a, size_t i) {
        auto [name, text] = equationArg(a, i);
        if (text.empty())
            throw RuntimeError(u8(a.name) + ": pierwszy argument musi być funkcją F0–F9 (tekst wyrażenia w X).");
        std::u32string expr = text;
        Interpreter *in = &a.in;
        return std::function<double(double)>([in, expr](double x) { return in->evalFunctionOf(expr, U"X", x); });
    };
    t[U"ROOT"] = {2, 2, true, [fnOf](CallArgs &a) {
                      double r = findRoot(fnOf(a, 0), a.in.eval(*a.exprs[1]).toReal());
                      setSys(a.in, U"Root", r);
                      return Value::real(r);
                  }};
    t[U"SLOPE"] = {2, 2, true, [fnOf](CallArgs &a) {
                       double s = numericDerivative(fnOf(a, 0), a.in.eval(*a.exprs[1]).toReal());
                       setSys(a.in, U"Slope", s);
                       return Value::real(s);
                   }};
    t[U"EXTREMUM"] = {2, 2, true, [fnOf](CallArgs &a) {
                          auto f = fnOf(a, 0);
                          double x = findRoot([&](double v) { return numericDerivative(f, v); }, a.in.eval(*a.exprs[1]).toReal());
                          setSys(a.in, U"Extremum", x);
                          return Value::real(x);
                      }};
    t[U"ISECT"] = {3, 3, true, [fnOf](CallArgs &a) {
                       auto f = fnOf(a, 0), g = fnOf(a, 1);
                       double x = findRoot([&](double v) { return f(v) - g(v); }, a.in.eval(*a.exprs[2]).toReal());
                       setSys(a.in, U"Isect", x);
                       return Value::real(x);
                   }};
    t[U"AREA"] = {3, 4, true, [fnOf](CallArgs &a) {
                      auto f = fnOf(a, 0);
                      std::function<double(double)> g = [](double) { return 0.0; };
                      size_t k = 1;
                      if (a.size() == 4) {
                          g = fnOf(a, 1);
                          k = 2;
                      }
                      double lo = a.in.eval(*a.exprs[k]).toReal(), hi = a.in.eval(*a.exprs[k + 1]).toReal();
                      double r = integrate([&](double x) { return f(x) - g(x); }, lo, hi);
                      setSys(a.in, U"SignedArea", r);
                      return Value::real(r);
                  }};
    t[U"SOLVE"] = {2, 3, true, [](CallArgs &a) {
                       Interpreter &in = a.in;
                       auto [name, text] = equationArg(a, 0);
                       std::u32string var = U"X";
                       if (a.size() >= 2 && a.exprs[1]->kind == ExprKind::Ident)
                           var = a.exprs[1]->text;
                       double guess = a.size() >= 3 ? in.eval(*a.exprs[2]).toReal() : 0;
                       std::function<double(double)> f;
                       if (!text.empty()) {
                           std::u32string expr = toZeroForm(text);
                           f = [&in, expr, var](double x) { return in.evalFunctionOf(expr, var, x); };
                       } else {
                           const Expr *e = a.exprs[0].get();
                           if (e->kind == ExprKind::Binary && e->text == U"=") {
                               const Expr *l = e->args[0].get(), *r = e->args[1].get();
                               f = [&in, l, r, var](double x) {
                                   return in.withBinding(var, Value::real(x), [&] {
                                               return Value::real(in.eval(*l).toReal() - in.eval(*r).toReal());
                                           }).toReal();
                               };
                           } else {
                               f = exprFunction(in, *e, var);
                           }
                       }
                       return Value::real(findRoot(f, guess));
                   }};

    // ------------------------------------------------------------------ Explorer, Linear Solver
    t[U"LINEARSLOPE"] = {4, 4, false, [](CallArgs &a) { return Value::real((a.num(3) - a.num(1)) / (a.num(2) - a.num(0))); }};
    t[U"SOLVEFORSLOPE"] = t[U"LINEARSLOPE"];
    t[U"LINEARYINTERCEPT"] = {3, 3, false, [](CallArgs &a) { return Value::real(a.num(1) - a.num(2) * a.num(0)); }};
    t[U"SOLVEFORYINTERCEPT"] = t[U"LINEARYINTERCEPT"];
    t[U"QUADDELTA"] = {3, 3, false, [](CallArgs &a) { return Value::real(a.num(1) * a.num(1) - 4 * a.num(0) * a.num(2)); }};
    t[U"DELTA"] = t[U"QUADDELTA"];
    t[U"QUADSOLVE"] = {3, 3, false, [](CallArgs &a) {
                           double A = a.num(0), B = a.num(1), C = a.num(2), D = B * B - 4 * A * C;
                           if (A == 0)
                               return Value::makeList({Value::real(-C / B)});
                           if (D < 0)
                               return Value::makeList();
                           if (D == 0)
                               return Value::makeList({Value::real(-B / (2 * A))});
                           double s = std::sqrt(D);
                           return Value::makeList({Value::real((-B - s) / (2 * A)), Value::real((-B + s) / (2 * A))});
                       }};
    auto linsolve = [](std::vector<std::vector<double>> m) {
        int n = static_cast<int>(m.size());
        for (int c = 0; c < n; ++c) {
            int p = c;
            for (int r = c + 1; r < n; ++r)
                if (std::fabs(m[r][c]) > std::fabs(m[p][c]))
                    p = r;
            if (std::fabs(m[p][c]) < 1e-14)
                throw RuntimeError("Układ nie ma jednoznacznego rozwiązania.");
            std::swap(m[p], m[c]);
            for (int r = 0; r < n; ++r) {
                if (r == c)
                    continue;
                double f = m[r][c] / m[c][c];
                for (int k = c; k <= n; ++k)
                    m[r][k] -= f * m[c][k];
            }
        }
        ValueList out;
        for (int r = 0; r < n; ++r)
            out.push_back(Value::real(m[r][n] / m[r][r]));
        return Value::vector(out);
    };
    t[U"SOLVE2X2"] = {6, 6, false, [linsolve](CallArgs &a) {
                          return linsolve({{a.num(0), a.num(1), a.num(2)}, {a.num(3), a.num(4), a.num(5)}});
                      }};
    t[U"SOLVE3X3"] = {12, 12, false, [linsolve](CallArgs &a) {
                          return linsolve({{a.num(0), a.num(1), a.num(2), a.num(3)},
                                           {a.num(4), a.num(5), a.num(6), a.num(7)},
                                           {a.num(8), a.num(9), a.num(10), a.num(11)}});
                      }};
    t[U"LINSOLVE"] = {1, 1, false, [linsolve](CallArgs &a) {
                          if (!a[0].isMatrix())
                              throw RuntimeError("LinSolve: oczekiwano macierzy rozszerzonej [A|b].");
                          std::vector<std::vector<double>> m(a[0].mat->rows, std::vector<double>(a[0].mat->cols));
                          for (int i = 0; i < a[0].mat->rows; ++i)
                              for (int j = 0; j < a[0].mat->cols; ++j)
                                  m[i][j] = a[0].mat->at(i, j).toReal();
                          return linsolve(m);
                      }};

    // ------------------------------------------------------------------ Statistics
    auto setPair = [](std::map<std::u32string, std::u32string> &(*sel)(Interpreter &)) {
        return Builtin{2, 2, true, [sel](CallArgs &a) {
                           if (a.exprs[0]->kind != ExprKind::Ident)
                               throw RuntimeError(u8(a.name) + ": pierwszy argument to nazwa analizy (H1–H5 / S1–S5).");
                           std::u32string col = a.exprs[1]->kind == ExprKind::Ident ? a.exprs[1]->text
                                                                                      : a.in.format(a.in.eval(*a.exprs[1]), false);
                           sel(a.in)[a.exprs[0]->text] = col;
                           return Value::real(0);
                       }};
    };
    t[U"SETSAMPLE"] = setPair([](Interpreter &in) -> std::map<std::u32string, std::u32string> & { return in.app().sample; });
    t[U"SETFREQ"] = setPair([](Interpreter &in) -> std::map<std::u32string, std::u32string> & { return in.app().freq; });
    t[U"SETINDEP"] = setPair([](Interpreter &in) -> std::map<std::u32string, std::u32string> & { return in.app().indep; });
    t[U"SETDEPEND"] = setPair([](Interpreter &in) -> std::map<std::u32string, std::u32string> & { return in.app().depend; });
    t[U"DO1VSTATS"] = {1, 1, true, [](CallArgs &a) {
                           Interpreter &in = a.in;
                           std::u32string h = a.exprs[0]->text;
                           std::u32string sample = in.app().sample.count(h) ? in.app().sample[h] : U"D" + h.substr(1);
                           ValueList data = listOrEmpty(in, sample);
                           std::vector<double> xs;
                           std::u32string fq = in.app().freq.count(h) ? in.app().freq[h] : U"1";
                           ValueList freqs = listOrEmpty(in, fq);
                           double fconst = freqs.empty() ? std::atof(u8(fq).c_str()) : 1;
                           for (size_t i = 0; i < data.size(); ++i) {
                               int f = freqs.empty() ? static_cast<int>(fconst) : (i < freqs.size() ? freqs[i].toInt() : 0);
                               for (int k = 0; k < f; ++k)
                                   xs.push_back(data[i].toReal());
                           }
                           if (xs.empty())
                               throw RuntimeError("Do1VStats: brak danych w " + u8(sample) + ".");
                           std::sort(xs.begin(), xs.end());
                           double n = static_cast<double>(xs.size()), s = 0, s2 = 0;
                           for (double x : xs) { s += x; s2 += x * x; }
                           double mean = s / n, ss = s2 - n * mean * mean;
                           auto median = [](const std::vector<double> &v, size_t b, size_t e) {
                               size_t len = e - b;
                               return len % 2 ? v[b + len / 2] : (v[b + len / 2 - 1] + v[b + len / 2]) / 2;
                           };
                           size_t N = xs.size();
                           setSys(in, U"NbItem", n);
                           setSys(in, U"MinVal", xs.front());
                           setSys(in, U"MaxVal", xs.back());
                           setSys(in, U"MedVal", median(xs, 0, N));
                           setSys(in, U"Q1", N > 1 ? median(xs, 0, N / 2) : xs[0]);
                           setSys(in, U"Q3", N > 1 ? median(xs, (N + 1) / 2, N) : xs[0]);
                           setSys(in, U"ΣX", s);
                           setSys(in, U"ΣX2", s2);
                           setSys(in, U"MeanX", mean);
                           setSys(in, U"ssX", ss);
                           setSys(in, U"σX", std::sqrt(std::max(0.0, ss / n)));
                           setSys(in, U"sX", n > 1 ? std::sqrt(std::max(0.0, ss / (n - 1))) : 0);
                           setSys(in, U"serrX", n > 1 ? std::sqrt(std::max(0.0, ss / (n - 1))) / std::sqrt(n) : 0);
                           return Value::real(0);
                       }};
    t[U"DO2VSTATS"] = {1, 1, true, [](CallArgs &a) {
                           Interpreter &in = a.in;
                           std::u32string s = a.exprs[0]->text;
                           int k = s.size() > 1 ? s[1] - U'0' : 1;
                           std::u32string xi = in.app().indep.count(s) ? in.app().indep[s] : U"C" + toU32(std::to_string(2 * k - 1));
                           std::u32string yi = in.app().depend.count(s) ? in.app().depend[s] : U"C" + toU32(std::to_string(2 * k));
                           ValueList X = listOrEmpty(in, xi), Y = listOrEmpty(in, yi);
                           size_t n = std::min(X.size(), Y.size());
                           if (n < 2)
                               throw RuntimeError("Do2VStats: za mało danych (potrzeba co najmniej 2 par).");
                           double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
                           for (size_t i = 0; i < n; ++i) {
                               double x = X[i].toReal(), y = Y[i].toReal();
                               sx += x; sy += y; sxx += x * x; syy += y * y; sxy += x * y;
                           }
                           double N = static_cast<double>(n), mx = sx / N, my = sy / N;
                           double cxx = sxx - N * mx * mx, cyy = syy - N * my * my, cxy = sxy - N * mx * my;
                           double r = cxy / std::sqrt(cxx * cyy);
                           setSys(in, U"NbItem", N);
                           setSys(in, U"MeanX", mx); setSys(in, U"MeanY", my);
                           setSys(in, U"ΣX", sx); setSys(in, U"ΣY", sy); setSys(in, U"ΣX2", sxx); setSys(in, U"ΣY2", syy); setSys(in, U"ΣXY", sxy);
                           setSys(in, U"sX", std::sqrt(cxx / (N - 1))); setSys(in, U"sY", std::sqrt(cyy / (N - 1)));
                           setSys(in, U"σX", std::sqrt(cxx / N)); setSys(in, U"σY", std::sqrt(cyy / N));
                           setSys(in, U"sCov", cxy / (N - 1)); setSys(in, U"σCov", cxy / N);
                           setSys(in, U"Corr", r); setSys(in, U"CoefDet", r * r);
                           in.avars()[U"__slope"] = Value::real(cxy / cxx);
                           in.avars()[U"__inter"] = Value::real(my - cxy / cxx * mx);
                           return Value::real(0);
                       }};
    t[U"PREDY"] = {1, 2, false, [](CallArgs &a) {
                       auto &av = a.in.avars();
                       if (!av.count(U"__slope"))
                           throw RuntimeError("PredY: najpierw wykonaj Do2VStats (dopasowanie liniowe).");
                       return Value::real(av[U"__inter"].toReal() + av[U"__slope"].toReal() * a.num(0));
                   }};
    t[U"PREDX"] = {1, 2, false, [](CallArgs &a) {
                       auto &av = a.in.avars();
                       if (!av.count(U"__slope"))
                           throw RuntimeError("PredX: najpierw wykonaj Do2VStats (dopasowanie liniowe).");
                       return Value::real((a.num(0) - av[U"__inter"].toReal()) / av[U"__slope"].toReal());
                   }};

    // ------------------------------------------------------------------ Finance
    auto tvm = [](const char *order, char unknown) {
        return Builtin{4, 7, false, [order, unknown](CallArgs &a) { return Value::real(solveTvm(tvmFrom(a, order), unknown)); }};
    };
    t[U"TVMPMT"] = tvm("NIVF", 'P');
    t[U"TVMFV"] = tvm("NIVP", 'F');
    t[U"TVMPV"] = tvm("NIPF", 'V');
    t[U"TVMIPYR"] = tvm("NVPF", 'I');
    t[U"TVMNBPMT"] = tvm("IVPF", 'N');
    t[U"CALCPMT"] = t[U"TVMPMT"];
    t[U"CALCFV"] = t[U"TVMFV"];
    t[U"CALCPV"] = t[U"TVMPV"];
    t[U"CALCIPYR"] = t[U"TVMIPYR"];
    t[U"CALCNBPMT"] = t[U"TVMNBPMT"];
    t[U"INTCONVEFF"] = {2, 2, false, [](CallArgs &a) {
                            double nom = a.num(0), c = a.num(1);
                            return Value::real((std::pow(1 + nom / 100 / c, c) - 1) * 100);
                        }};
    t[U"INTCONVNOM"] = {2, 2, false, [](CallArgs &a) {
                            double eff = a.num(0), c = a.num(1);
                            return Value::real(c * (std::pow(1 + eff / 100, 1 / c) - 1) * 100);
                        }};
    t[U"DATEDAYS"] = {2, 3, false, [](CallArgs &a) {
                          int y1, m1, d1, y2, m2, d2;
                          dateParts(a.num(0), y1, m1, d1);
                          dateParts(a.num(1), y2, m2, d2);
                          if (a.size() > 2 && a.num(2) != 0) {
                              d1 = std::min(d1, 30);
                              if (d1 == 30) d2 = std::min(d2, 30);
                              return Value::real((y2 - y1) * 360 + (m2 - m1) * 30 + (d2 - d1));
                          }
                          return Value::real(static_cast<double>(daysFromCivil(y2, m2, d2) - daysFromCivil(y1, m1, d1)));
                      }};
    t[U"PERCENTCHANGE"] = {2, 2, false, [](CallArgs &a) { return Value::real((a.num(1) - a.num(0)) / a.num(0) * 100); }};
    t[U"PERCENTTOTAL"] = {2, 2, false, [](CallArgs &a) { return Value::real(a.num(1) / a.num(0) * 100); }};
    t[U"PERCENTMARGIN"] = {2, 2, false, [](CallArgs &a) { return Value::real((a.num(1) - a.num(0)) / a.num(1) * 100); }};
    t[U"PERCENTMARKUP"] = {2, 2, false, [](CallArgs &a) { return Value::real((a.num(1) - a.num(0)) / a.num(0) * 100); }};
    // break-even: profit = quantity·(price − cost) − fixed
    t[U"BRKEVPROFIT"] = {4, 4, false, [](CallArgs &a) { return Value::real(a.num(1) * (a.num(3) - a.num(2)) - a.num(0)); }};
    t[U"BRKEVFIXED"] = {4, 4, false, [](CallArgs &a) { return Value::real(a.num(0) * (a.num(2) - a.num(1)) - a.num(3)); }};
    t[U"BRKEVQUANT"] = {4, 4, false, [](CallArgs &a) { return Value::real((a.num(3) + a.num(0)) / (a.num(2) - a.num(1))); }};
    t[U"BRKEVCOST"] = {4, 4, false, [](CallArgs &a) { return Value::real(a.num(2) - (a.num(3) + a.num(0)) / a.num(1)); }};
    t[U"BRKEVPRICE"] = {4, 4, false, [](CallArgs &a) { return Value::real(a.num(2) + (a.num(3) + a.num(0)) / a.num(1)); }};
    t[U"CASHFLOWTOTAL"] = {1, 1, false, [](CallArgs &a) {
                               double s = 0;
                               for (const auto &v : a[0].isList() ? a[0].items() : a[0].mat->data)
                                   s += v.toReal();
                               return Value::real(s);
                           }};
    auto npv = [](const ValueList &cf, double ratePct) {
        double r = ratePct / 100, s = 0;
        for (size_t k = 0; k < cf.size(); ++k)
            s += cf[k].toReal() / std::pow(1 + r, static_cast<double>(k));
        return s;
    };
    t[U"CASHFLOWNPV"] = {2, 3, false, [npv](CallArgs &a) { return Value::real(npv(a[0].items(), a.num(1))); }};
    t[U"CASHFLOWNFV"] = {2, 3, false, [npv](CallArgs &a) {
                             double r = a.num(1) / 100;
                             return Value::real(npv(a[0].items(), a.num(1)) * std::pow(1 + r, static_cast<double>(a[0].items().size() - 1)));
                         }};
    t[U"CASHFLOWIRR"] = {1, 2, false, [npv](CallArgs &a) {
                             ValueList cf = a[0].items();
                             return Value::real(findRoot([&](double r) { return npv(cf, r); }, 10));
                         }};

    // ------------------------------------------------------------------ system accessors
    auto listFrom = [](const std::vector<std::u32string> &names) {
        ValueList l;
        for (const auto &n : names)
            l.push_back(Value::string(n));
        return Value::makeList(l);
    };
    t[U"NOTES"] = {0, 1, false, [listFrom](CallArgs &a) {
                       auto &notes = a.in.notes();
                       if (a.size() == 0) {
                           std::vector<std::u32string> n;
                           for (const auto &[k, v] : notes)
                               n.push_back(k);
                           return listFrom(n);
                       }
                       std::u32string key;
                       if (a[0].isNumber()) {
                           int i = a.integer(0);
                           if (i < 1 || i > static_cast<int>(notes.size()))
                               throw RuntimeError("Notes: numer notatki poza zakresem.");
                           key = std::next(notes.begin(), i - 1)->first;
                       } else {
                           key = a.text(0);
                       }
                       auto it = notes.find(key);
                       if (it == notes.end())
                           throw RuntimeError("Brak notatki \"" + u8(key) + "\".");
                       return Value::string(it->second);
                   }};
    t[U"PROGRAMS"] = {0, 1, false, [listFrom](CallArgs &a) {
                          auto names = a.in.programNames();
                          if (a.size() == 0)
                              return listFrom(names);
                          std::u32string key = a[0].isNumber() ? names.at(static_cast<size_t>(a.integer(0) - 1)) : a.text(0);
                          std::u32string src = a.in.programSource(key);
                          if (src.empty())
                              throw RuntimeError("Brak programu \"" + u8(key) + "\".");
                          return Value::string(src);
                      }};
    t[U"HVARS"] = {0, 2, false, [listFrom](CallArgs &a) {
                       auto vars = a.in.globalVariables();
                       if (a.size() == 0) {
                           std::vector<std::u32string> n;
                           for (const auto &[k, v] : vars)
                               n.push_back(k);
                           return listFrom(n);
                       }
                       std::u32string key = a[0].isNumber() ? vars.at(static_cast<size_t>(a.integer(0) - 1)).first : a.text(0);
                       return a.in.getVariable(key);
                   }};
    t[U"AVARS"] = {0, 1, false, [listFrom](CallArgs &a) {
                       auto &av = a.in.avars();
                       if (a.size() == 0) {
                           std::vector<std::u32string> n;
                           for (const auto &[k, v] : av)
                               if (k.rfind(U"__", 0) != 0)
                                   n.push_back(k);
                           return listFrom(n);
                       }
                       std::u32string key = a.text(0);
                       auto it = av.find(key);
                       if (it == av.end())
                           throw RuntimeError("Brak zmiennej aplikacji \"" + u8(key) + "\".");
                       return it->second;
                   }};
    t[U"DELAVARS"] = {1, 1, false, [](CallArgs &a) {
                          a.in.avars().erase(a.text(0));
                          return Value::real(0);
                      }};
    t[U"DELHVARS"] = {1, 1, false, [](CallArgs &a) {
                          a.in.host().notice(U"[DelHVars] Usunięcie zmiennej " + a.in.format(a[0], false) + U" (symulowane).");
                          return Value::real(0);
                      }};
    t[U"DATE"] = {0, 0, false, [](CallArgs &) {
                      std::time_t now = std::time(nullptr);
                      std::tm *lt = std::localtime(&now);
                      return Value::real(lt->tm_year + 1900 + (lt->tm_mon + 1) / 100.0 + lt->tm_mday / 10000.0);
                  }};
    t[U"TIME"] = {0, 0, false, [](CallArgs &) {
                      std::time_t now = std::time(nullptr);
                      std::tm *lt = std::localtime(&now);
                      return Value::real(lt->tm_hour + lt->tm_min / 60.0 + lt->tm_sec / 3600.0);
                  }};
}

void registerAllBuiltins(Interpreter &in, std::map<std::u32string, Builtin> &table)
{
    registerCoreBuiltins(in, table);
    registerIoBuiltins(in, table);
    registerAppBuiltins(in, table);
}

} // namespace ppl
