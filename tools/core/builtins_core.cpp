// Built-in commands: mathematics, strings, lists, matrices, integers, calculus, selected CAS functions.
#include "arith.h"
#include "builtins.h"
#include "interpreter.h"
#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>

namespace ppl {

namespace {

std::string u8(const std::u32string &s) { return toUtf8(s); }

Value mapValue(const Value &v, const std::function<Value(const Value &)> &f)
{
    if (v.isList()) {
        ValueList out;
        for (const auto &x : v.items())
            out.push_back(mapValue(x, f));
        return Value::makeList(std::move(out));
    }
    if (v.isMatrix()) {
        Value r = v;
        Matrix &m = r.mutableMatrix();
        for (auto &x : m.data)
            x = f(x);
        return r;
    }
    return f(v);
}

// Real function applied element-wise; complex arguments use the complex version when provided.
void real1(BuiltinTable &t, const char32_t *name, std::function<double(double)> f,
           std::function<std::complex<double>(std::complex<double>)> cf = nullptr)
{
    std::u32string n = name;
    t[n] = Builtin{1, 1, false, [f, cf, n](CallArgs &a) {
                       return mapValue(a[0], [&](const Value &x) -> Value {
                           if (x.type == Value::Type::Complex) {
                               if (!cf)
                                   throw RuntimeError(u8(n) + " nie obsługuje liczb zespolonych w symulatorze.");
                               return normalizeComplex(cf(x.toComplex()));
                           }
                           double r = f(x.toReal(u8(n).c_str()));
                           if (std::isnan(r))
                               throw RuntimeError(u8(n) + ": argument poza dziedziną.");
                           return Value::real(r);
                       });
                   }};
}

// Trig with angle mode; inverse functions may produce complex values.
void trig(BuiltinTable &t, const char32_t *name, double (*f)(double), bool inverse, double lo = -1, double hi = 1,
          std::complex<double> (*cf)(const std::complex<double> &) = nullptr)
{
    std::u32string n = name;
    t[n] = Builtin{1, 1, false, [=](CallArgs &a) {
                       Interpreter &in = a.in;
                       return mapValue(a[0], [&](const Value &x) -> Value {
                           if (x.type == Value::Type::Complex) {
                               if (!cf)
                                   throw RuntimeError(u8(n) + " nie obsługuje liczb zespolonych w symulatorze.");
                               return normalizeComplex(cf(x.toComplex()));
                           }
                           double v = x.toReal(u8(n).c_str());
                           if (!inverse)
                               return Value::real(f(in.angleToRadians(v)));
                           if (v < lo || v > hi) {
                               if (in.complexMode() && cf)
                                   return normalizeComplex(cf({v, 0}));
                               throw RuntimeError(u8(n) + ": argument poza dziedziną (wynik zespolony; włącz HComplex:=1).");
                           }
                           return Value::real(in.radiansToAngle(f(v)));
                       });
                   }};
}

double roundTo(double x, int n)
{
    if (n >= 0) {
        double p = std::pow(10.0, n);
        return std::round(x * p) / p;
    }
    // negative: significant digits
    if (x == 0)
        return 0;
    int digits = -n;
    double mag = std::floor(std::log10(std::fabs(x))) + 1;
    double p = std::pow(10.0, digits - mag);
    return std::round(x * p) / p;
}

double truncateTo(double x, int n)
{
    double p = std::pow(10.0, n);
    return std::trunc(x * p) / p;
}

const ValueList &listArg(const CallArgs &a, size_t i)
{
    if (!a[i].isList())
        throw RuntimeError(u8(a.name) + ": argument " + std::to_string(i + 1) + " musi być listą, a jest "
                           + typeName(a[i]) + ".");
    return a[i].items();
}

// Elements of a list, vector or sequence
ValueList elements(const Value &v)
{
    if (v.isList())
        return v.items();
    if (v.isMatrix())
        return v.mat->data;
    return {v};
}

std::u32string strArg(const CallArgs &a, size_t i) { return a.text(i); }

// ---- numerical helpers
double derivative(const std::function<double(double)> &f, double x)
{
    // Ridders' method: central differences with a fairly large step, extrapolated to h → 0.
    // A large initial step keeps the 12-digit rounding of function values from dominating.
    const int ntab = 10;
    const double con = 1.4, con2 = con * con;
    double h = 0.1 * std::max(1.0, std::fabs(x));
    double a[ntab][ntab];
    a[0][0] = (f(x + h) - f(x - h)) / (2 * h);
    double best = a[0][0], err = 1e300;
    for (int i = 1; i < ntab; ++i) {
        h /= con;
        a[0][i] = (f(x + h) - f(x - h)) / (2 * h);
        double fac = con2;
        for (int j = 1; j <= i; ++j) {
            a[j][i] = (a[j - 1][i] * fac - a[j - 1][i - 1]) / (fac - 1);
            fac *= con2;
            double e = std::max(std::fabs(a[j][i] - a[j - 1][i]), std::fabs(a[j][i] - a[j - 1][i - 1]));
            if (e <= err) {
                err = e;
                best = a[j][i];
            }
        }
        if (std::fabs(a[i][i] - a[i - 1][i - 1]) >= 2 * err)
            break;
    }
    return best;
}

double simpson(const std::function<double(double)> &f, double a, double b, double fa, double fm, double fb, double whole,
               double eps, int depth)
{
    double m = (a + b) / 2, lm = (a + m) / 2, rm = (m + b) / 2;
    double flm = f(lm), frm = f(rm);
    double left = (m - a) / 6 * (fa + 4 * flm + fm), right = (b - m) / 6 * (fm + 4 * frm + fb);
    double delta = left + right - whole;
    if (depth <= 0 || std::fabs(delta) <= 15 * eps)
        return left + right + delta / 15;
    return simpson(f, a, m, fa, flm, fm, left, eps / 2, depth - 1) + simpson(f, m, b, fm, frm, fb, right, eps / 2, depth - 1);
}

} // namespace

double integrate(const std::function<double(double)> &f, double a, double b)
{
    if (a == b)
        return 0;
    double fa = f(a), fb = f(b), m = (a + b) / 2, fm = f(m);
    double whole = (b - a) / 6 * (fa + 4 * fm + fb);
    double scale = std::max(1.0, std::fabs(whole));
    return simpson(f, a, b, fa, fm, fb, whole, 1e-13 * scale, 40);
}

double findRoot(const std::function<double(double)> &f, double guess)
{
    // Newton with numeric derivative, falling back to a bracketing search + bisection.
    double x = guess;
    for (int i = 0; i < 100; ++i) {
        double fx = f(x);
        if (fx == 0)
            return x;
        double d = derivative(f, x);
        if (d == 0 || !std::isfinite(d))
            break;
        double nx = x - fx / d;
        if (!std::isfinite(nx))
            break;
        if (std::fabs(nx - x) <= 1e-14 * std::max(1.0, std::fabs(nx))) {
            x = nx;
            if (std::fabs(f(x)) < 1e-9 * std::max(1.0, std::fabs(fx)) || std::fabs(f(x)) < 1e-10)
                return x;
            break;
        }
        x = nx;
    }
    if (std::isfinite(x) && std::fabs(f(x)) < 1e-10)
        return x;
    // bracket around the guess
    double step = std::max(1e-3, std::fabs(guess) * 1e-3);
    double a = guess, b = guess, fa = f(a), fb = fa;
    for (int i = 0; i < 200; ++i) {
        a -= step;
        b += step;
        step *= 1.6;
        double na = f(a), nb = f(b);
        if (std::isfinite(na) && std::isfinite(fa) && na * fa <= 0) { b = a + step / 1.6; fb = f(b); fa = na; break; }
        if (std::isfinite(nb) && std::isfinite(fb) && nb * fb <= 0) { a = b - step / 1.6; fa = f(a); fb = nb; break; }
        fa = na;
        fb = nb;
        if (i == 199)
            throw RuntimeError("Nie znaleziono rozwiązania w pobliżu punktu startowego.");
    }
    if (fa * fb > 0)
        throw RuntimeError("Nie znaleziono rozwiązania w pobliżu punktu startowego.");
    for (int i = 0; i < 200; ++i) {
        double m = (a + b) / 2, fm = f(m);
        if (fm == 0 || (b - a) / 2 < 1e-15 * std::max(1.0, std::fabs(m)))
            return m;
        if (fa * fm < 0) { b = m; fb = fm; } else { a = m; fa = fm; }
    }
    return (a + b) / 2;
}

double numericDerivative(const std::function<double(double)> &f, double x) { return derivative(f, x); }

// Evaluates expression expr with variable var bound to x.
std::function<double(double)> exprFunction(Interpreter &in, const Expr &expr, const std::u32string &var)
{
    return [&in, &expr, var](double x) {
        FullPrecision fp;
        return in.withBinding(var, Value::real(x), [&] { return in.eval(expr); }).toReal("funkcja");
    };
}

void registerCoreBuiltins(Interpreter &, BuiltinTable &t)
{
    // ------------------------------------------------------------------ numbers
    real1(t, U"IP", [](double x) { return std::trunc(x); });
    real1(t, U"FP", [](double x) { return x - std::trunc(x); });
    real1(t, U"FLOOR", [](double x) { return std::floor(x); });
    real1(t, U"CEILING", [](double x) { return std::ceil(x); });
    real1(t, U"SIGN", [](double x) { return static_cast<double>((x > 0) - (x < 0)); });
    real1(t, U"MANT", [](double x) { return x == 0 ? 0 : x / std::pow(10.0, std::floor(std::log10(std::fabs(x)))); });
    real1(t, U"XPON", [](double x) { return x == 0 ? 0 : std::floor(std::log10(std::fabs(x))); });
    real1(t, U"EXP", [](double x) { return std::exp(x); }, [](std::complex<double> z) { return std::exp(z); });
    real1(t, U"ALOG", [](double x) { return std::pow(10.0, x); });
    real1(t, U"EXPM1", [](double x) { return std::expm1(x); });
    real1(t, U"LNP1", [](double x) { return std::log1p(x); });
    real1(t, U"SINH", [](double x) { return std::sinh(x); });
    real1(t, U"COSH", [](double x) { return std::cosh(x); });
    real1(t, U"TANH", [](double x) { return std::tanh(x); });
    real1(t, U"ASINH", [](double x) { return std::asinh(x); });
    real1(t, U"ACOSH", [](double x) { return std::acosh(x); });
    real1(t, U"ATANH", [](double x) { return std::atanh(x); });
    t[U"ABS"] = {1, 1, false, [](CallArgs &a) {
                     const Value &v = a[0];
                     if (v.isMatrix()) {
                         double s = 0;
                         for (const auto &x : v.mat->data)
                             s += std::norm(x.toComplex());
                         return Value::real(std::sqrt(s));
                     }
                     return mapValue(v, [](const Value &x) {
                         if (x.type == Value::Type::Integer)
                             return Value::integer(std::llabs(x.ival), x.base, x.bits);
                         return Value::real(std::abs(x.toComplex()));
                     });
                 }};
    auto logFn = [](const char32_t *name, double base) {
        return Builtin{1, 2, false, [base, name](CallArgs &a) {
                           double b = a.size() > 1 ? a.num(1) : base;
                           Interpreter &in = a.in;
                           return mapValue(a[0], [&](const Value &x) -> Value {
                               std::complex<double> z = x.toComplex();
                               if (x.type == Value::Type::Complex || z.real() < 0) {
                                   if (x.type != Value::Type::Complex && !in.complexMode())
                                       throw RuntimeError(u8(name) + " z liczby ujemnej: wynik zespolony (włącz HComplex:=1).");
                                   return normalizeComplex(std::log(z) / std::log(b));
                               }
                               if (z.real() == 0)
                                   throw RuntimeError(u8(name) + "(0) jest nieokreślony.");
                               return Value::real(std::log(z.real()) / std::log(b));
                           });
                       }};
    };
    t[U"LN"] = logFn(U"LN", M_E);
    t[U"LOG"] = logFn(U"LOG", 10);
    t[U"ROUND"] = {1, 2, false, [](CallArgs &a) {
                       int n = a.size() > 1 ? a.integer(1) : 0;
                       return mapValue(a[0], [n](const Value &x) { return Value::real(roundTo(x.toReal("ROUND"), n)); });
                   }};
    t[U"TRUNCATE"] = {1, 2, false, [](CallArgs &a) {
                          int n = a.size() > 1 ? a.integer(1) : 0;
                          return mapValue(a[0], [n](const Value &x) { return Value::real(truncateTo(x.toReal("TRUNCATE"), n)); });
                      }};
    auto minmax = [](bool isMax) {
        return Builtin{1, -1, false, [isMax](CallArgs &a) {
                           ValueList all;
                           for (const auto &v : a.values) {
                               auto e = elements(v);
                               all.insert(all.end(), e.begin(), e.end());
                           }
                           if (all.empty())
                               throw RuntimeError("MAX/MIN: brak wartości.");
                           Value best = all[0];
                           for (const auto &v : all)
                               if (isMax ? v.toReal() > best.toReal() : v.toReal() < best.toReal())
                                   best = v;
                           return best;
                       }};
    };
    t[U"MAX"] = minmax(true);
    t[U"MIN"] = minmax(false);
    trig(t, U"SIN", std::sin, false, 0, 0, [](const std::complex<double> &z) { return std::sin(z); });
    trig(t, U"COS", std::cos, false, 0, 0, [](const std::complex<double> &z) { return std::cos(z); });
    trig(t, U"TAN", std::tan, false, 0, 0, [](const std::complex<double> &z) { return std::tan(z); });
    trig(t, U"ASIN", std::asin, true, -1, 1, [](const std::complex<double> &z) { return std::asin(z); });
    trig(t, U"ACOS", std::acos, true, -1, 1, [](const std::complex<double> &z) { return std::acos(z); });
    trig(t, U"ATAN", std::atan, true, -1e308, 1e308, [](const std::complex<double> &z) { return std::atan(z); });
    trig(t, U"CSC", [](double x) { return 1 / std::sin(x); }, false);
    trig(t, U"SEC", [](double x) { return 1 / std::cos(x); }, false);
    trig(t, U"COT", [](double x) { return 1 / std::tan(x); }, false);
    trig(t, U"ACSC", [](double x) { return std::asin(1 / x); }, true, -1e308, 1e308);
    trig(t, U"ASEC", [](double x) { return std::acos(1 / x); }, true, -1e308, 1e308);
    trig(t, U"ACOT", [](double x) { return std::atan(1 / x); }, true, -1e308, 1e308);
    t[U"COMB"] = {2, 2, false, [](CallArgs &a) {
                      double n = a.num(0), k = a.num(1), r = 1;
                      if (k < 0 || k > n)
                          return Value::real(0);
                      for (int i = 1; i <= static_cast<int>(k); ++i)
                          r = r * (n - k + i) / i;
                      return Value::real(std::round(r));
                  }};
    t[U"PERM"] = {2, 2, false, [](CallArgs &a) {
                      double n = a.num(0), k = a.num(1), r = 1;
                      for (int i = 0; i < static_cast<int>(k); ++i)
                          r *= n - i;
                      return Value::real(r);
                  }};
    t[U"RANDOM"] = {0, 3, false, [](CallArgs &a) {
                        std::uniform_real_distribution<double> u(0.0, 1.0);
                        auto &g = a.in.rng();
                        if (a.size() == 0)
                            return Value::real(u(g));
                        double lo = a.size() >= 2 ? a.num(a.size() - 2) : 0, hi = a.num(a.size() - 1);
                        if (a.size() == 3) {
                            ValueList l;
                            for (int i = 0; i < a.integer(0); ++i)
                                l.push_back(Value::real(lo + (hi - lo) * u(g)));
                            return Value::makeList(l);
                        }
                        return Value::real(lo + (hi - lo) * u(g));
                    }};
    t[U"RANDINT"] = {1, 3, false, [](CallArgs &a) {
                         auto &g = a.in.rng();
                         auto pick = [&](int lo, int hi) {
                             if (lo > hi)
                                 std::swap(lo, hi);
                             std::uniform_int_distribution<int> d(lo, hi);
                             return Value::real(d(g));
                         };
                         if (a.size() == 1)
                             return pick(0, a.integer(0));
                         if (a.size() == 2)
                             return pick(a.integer(0), a.integer(1));
                         ValueList l;
                         for (int i = 0; i < a.integer(0); ++i)
                             l.push_back(pick(a.integer(1), a.integer(2)));
                         return Value::makeList(l);
                     }};
    t[U"RANDSEED"] = {0, 1, false, [](CallArgs &a) {
                          a.in.rng().seed(a.size() ? static_cast<uint64_t>(a.num(0) * 1e6) : 0);
                          return Value::real(0);
                      }};
    t[U"RE"] = {1, 1, false, [](CallArgs &a) { return mapValue(a[0], [](const Value &x) { return Value::real(x.toComplex().real()); }); }};
    t[U"IM"] = {1, 1, false, [](CallArgs &a) { return mapValue(a[0], [](const Value &x) { return Value::real(x.toComplex().imag()); }); }};
    t[U"CONJ"] = {1, 1, false, [](CallArgs &a) { return mapValue(a[0], [](const Value &x) { return normalizeComplex(std::conj(x.toComplex())); }); }};
    t[U"ARG"] = {1, 1, false, [](CallArgs &a) {
                     Interpreter &in = a.in;
                     return mapValue(a[0], [&](const Value &x) { return Value::real(in.radiansToAngle(std::arg(x.toComplex()))); });
                 }};
    t[U"WHEN"] = {3, 3, true, [](CallArgs &a) {
                      return a.in.eval(*a.exprs[0]).truthy() ? a.in.eval(*a.exprs[1]) : a.in.eval(*a.exprs[2]);
                  }};
    t[U"IREM"] = {2, 2, false, [](CallArgs &a) {
                      double x = a.num(0), y = a.num(1);
                      if (y == 0)
                          throw RuntimeError("irem przez zero.", 3);
                      return Value::real(std::fmod(x, y));
                  }};
    t[U"IQUO"] = {2, 2, false, [](CallArgs &a) {
                      double x = a.num(0), y = a.num(1);
                      if (y == 0)
                          throw RuntimeError("iquo przez zero.", 3);
                      return Value::real(std::trunc(x / y));
                  }};
    t[U"%CHANGE"] = {2, 2, false, [](CallArgs &a) { return Value::real((a.num(1) - a.num(0)) / a.num(0) * 100); }};
    t[U"%TOTAL"] = {2, 2, false, [](CallArgs &a) { return Value::real(a.num(1) / a.num(0) * 100); }};
    t[U"%"] = {2, 2, false, [](CallArgs &a) { return Value::real(a.num(0) * a.num(1) / 100); }};
    t[U"→HMS"] = {1, 1, false, [](CallArgs &a) {
                      // value stored as decimal; represented in H.MMSS style is not a PPL type — keep decimal hours
                      double x = a.num(0), s = x < 0 ? -1 : 1;
                      x = std::fabs(x);
                      double h = std::floor(x), m = std::floor((x - h) * 60), sec = ((x - h) * 60 - m) * 60;
                      return Value::real(s * (h + m / 100 + sec / 10000));
                  }};
    t[U"HMS→"] = {1, 1, false, [](CallArgs &a) {
                      double x = a.num(0), s = x < 0 ? -1 : 1;
                      x = std::fabs(x);
                      double h = std::floor(x), mm = std::floor((x - h) * 100 + 1e-9), sec = ((x - h) * 100 - mm) * 100;
                      return Value::real(s * (h + mm / 60 + sec / 3600));
                  }};
    t[U"TYPE"] = {1, 1, false, [](CallArgs &a) { return Value::real(a[0].typeCode()); }};
    t[U"EXPR"] = {1, 1, false, [](CallArgs &a) {
                      if (a[0].type != Value::Type::String && a[0].type != Value::Type::Symbolic)
                          return a[0];
                      return a.in.evalText(a[0].str);
                  }};
    t[U"EVAL"] = t[U"EXPR"];
    t[U"APPROX"] = {1, 1, false, [](CallArgs &a) { return a[0]; }};
    t[U"CAS.APPROX"] = t[U"APPROX"];

    // ------------------------------------------------------------------ integers
    auto intOf = [](const Value &v) -> int64_t {
        if (v.type == Value::Type::Integer)
            return v.ival;
        return static_cast<int64_t>(v.toReal("liczba całkowita"));
    };
    auto baseOf = [](const CallArgs &a) {
        for (const auto &v : a.values)
            if (v.type == Value::Type::Integer)
                return v;
        return Value();
    };
    auto bitOp = [intOf, baseOf](std::function<int64_t(int64_t, int64_t)> f) {
        return Builtin{2, -1, false, [=](CallArgs &a) {
                           int64_t r = intOf(a[0]);
                           for (size_t i = 1; i < a.size(); ++i)
                               r = f(r, intOf(a[i]));
                           Value b = baseOf(a);
                           return b.type == Value::Type::Integer ? Value::integer(r, b.base, b.bits) : Value::real(static_cast<double>(r));
                       }};
    };
    t[U"BITAND"] = bitOp([](int64_t x, int64_t y) { return x & y; });
    t[U"BITOR"] = bitOp([](int64_t x, int64_t y) { return x | y; });
    t[U"BITXOR"] = bitOp([](int64_t x, int64_t y) { return x ^ y; });
    t[U"BITNOT"] = {1, 1, false, [intOf](CallArgs &a) {
                        int bits = a[0].type == Value::Type::Integer && a[0].bits ? a[0].bits : a.in.formatSettings().bitsDefault;
                        uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1);
                        int64_t r = static_cast<int64_t>(~static_cast<uint64_t>(intOf(a[0])) & mask);
                        return a[0].type == Value::Type::Integer ? Value::integer(r, a[0].base, a[0].bits) : Value::real(static_cast<double>(r));
                    }};
    auto shift = [intOf](bool left) {
        return Builtin{1, 2, false, [=](CallArgs &a) {
                           int64_t v = intOf(a[0]);
                           int n = a.size() > 1 ? a.integer(1) : 1;
                           int64_t r = left ? (v << n) : static_cast<int64_t>(static_cast<uint64_t>(v) >> n);
                           return a[0].type == Value::Type::Integer ? Value::integer(r, a[0].base, a[0].bits) : Value::real(static_cast<double>(r));
                       }};
    };
    t[U"BITSL"] = shift(true);
    t[U"BITSR"] = shift(false);
    t[U"B→R"] = {1, 1, false, [intOf](CallArgs &a) { return Value::real(static_cast<double>(intOf(a[0]))); }};
    t[U"R→B"] = {1, 1, false, [intOf](CallArgs &a) {
                     Value base = a.in.getVariable(U"Base");
                     int b = static_cast<int>(base.toReal());
                     return Value::integer(intOf(a[0]), b == 0 ? 'b' : b == 1 ? 'o' : b == 2 ? 'd' : 'h');
                 }};
    t[U"SETBASE"] = {1, 2, false, [intOf](CallArgs &a) {
                         int c = a.size() > 1 ? a.integer(1) : 3;
                         char base = c == 1 ? 'b' : c == 2 ? 'o' : c == 0 ? 'd' : 'h';
                         return Value::integer(intOf(a[0]), base, a[0].bits);
                     }};
    t[U"GETBASE"] = {1, 1, false, [](CallArgs &a) {
                         char b = a[0].base;
                         return Value::integer(b == 'b' ? 1 : b == 'o' ? 2 : b == 'h' ? 3 : 0, 'h');
                     }};
    t[U"SETBITS"] = {1, 2, false, [intOf](CallArgs &a) {
                         int bits = a.size() > 1 ? a.integer(1) : a.in.formatSettings().bitsDefault;
                         return Value::integer(intOf(a[0]), a[0].type == Value::Type::Integer ? a[0].base : 'h', std::abs(bits));
                     }};
    t[U"GETBITS"] = {0, 1, false, [](CallArgs &a) {
                         int b = a.size() && a[0].type == Value::Type::Integer && a[0].bits ? a[0].bits : a.in.formatSettings().bitsDefault;
                         return Value::real(b);
                     }};

    // ------------------------------------------------------------------ strings
    t[U"DIM"] = {1, 1, false, [](CallArgs &a) {
                     const Value &v = a[0];
                     if (v.isString())
                         return Value::real(static_cast<double>(v.str.size()));
                     if (v.isMatrix()) {
                         if (v.mat->isVector)
                             return Value::makeList({Value::real(v.mat->cols)});
                         return Value::makeList({Value::real(v.mat->rows), Value::real(v.mat->cols)});
                     }
                     if (v.isList())
                         return Value::makeList({Value::real(static_cast<double>(v.items().size()))});
                     throw RuntimeError("DIM: oczekiwano tekstu, wektora lub macierzy.");
                 }};
    t[U"LEFT"] = {2, 2, false, [](CallArgs &a) {
                      std::u32string s = strArg(a, 0);
                      int n = a.integer(1);
                      if (n < 0 || n >= static_cast<int>(s.size()))
                          return Value::string(s);
                      return Value::string(s.substr(0, n));
                  }};
    t[U"RIGHT"] = {2, 2, false, [](CallArgs &a) {
                       std::u32string s = strArg(a, 0);
                       int n = a.integer(1);
                       if (n <= 0)
                           return Value::string(U"");
                       if (n >= static_cast<int>(s.size()))
                           return Value::string(s);
                       return Value::string(s.substr(s.size() - n));
                   }};
    t[U"MID"] = {2, 3, false, [](CallArgs &a) {
                     std::u32string s = strArg(a, 0);
                     int p = a.integer(1);
                     if (p < 1)
                         p = 1;
                     if (p > static_cast<int>(s.size()))
                         return Value::string(U"");
                     if (a.size() > 2)
                         return Value::string(s.substr(p - 1, std::max(0, a.integer(2))));
                     return Value::string(s.substr(p - 1));
                 }};
    t[U"INSTRING"] = {2, 3, false, [](CallArgs &a) {
                          std::u32string s = strArg(a, 0), f = strArg(a, 1);
                          size_t from = a.size() > 2 ? static_cast<size_t>(std::max(1, a.integer(2)) - 1) : 0;
                          size_t pos = s.find(f, from);
                          return Value::real(pos == std::u32string::npos ? 0 : static_cast<double>(pos + 1));
                      }};
    t[U"UPPER"] = {1, 1, false, [](CallArgs &a) {
                       std::u32string s = strArg(a, 0);
                       for (auto &c : s) {
                           if (c >= U'a' && c <= U'z') c -= 32;
                           else if ((c >= 0xE0 && c <= 0xFE && c != 0xF7)) c -= 32;
                           else if (c >= 0x100 && c <= 0x17F && (c % 2) == 1) c -= 1;
                       }
                       return Value::string(s);
                   }};
    t[U"LOWER"] = {1, 1, false, [](CallArgs &a) {
                       std::u32string s = strArg(a, 0);
                       for (auto &c : s) {
                           if (c >= U'A' && c <= U'Z') c += 32;
                           else if ((c >= 0xC0 && c <= 0xDE && c != 0xD7)) c += 32;
                           else if (c >= 0x100 && c <= 0x17F && (c % 2) == 0) c += 1;
                       }
                       return Value::string(s);
                   }};
    t[U"ASC"] = {1, 1, false, [](CallArgs &a) {
                     ValueList codes;
                     for (char32_t c : strArg(a, 0))
                         codes.push_back(Value::real(static_cast<double>(c)));
                     return Value::vector(codes);
                 }};
    t[U"CHAR"] = {1, 1, false, [](CallArgs &a) {
                      std::u32string s;
                      for (const auto &v : elements(a[0]))
                          s += static_cast<char32_t>(v.toInt("CHAR"));
                      return Value::string(s);
                  }};
    t[U"ROTATE"] = {2, 2, false, [](CallArgs &a) {
                        std::u32string s = strArg(a, 0);
                        int n = a.integer(1), len = static_cast<int>(s.size());
                        if (len == 0 || n > len || n < -len)
                            return Value::string(s);
                        if (n < 0)
                            n += len;
                        return Value::string(s.substr(n) + s.substr(0, n));
                    }};
    t[U"STRINGFROMID"] = {1, 1, false, [](CallArgs &a) {
                              int id = a.integer(0);
                              if (id == 56) return Value::string(U"Complex");
                              if (id == 202) return Value::string(U"Real");
                              a.in.unsupported(U"STRINGFROMID(" + toU32(std::to_string(id)) + U")");
                          }};
    t[U"STRING"] = {1, 5, false, [](CallArgs &a) {
                        const Value &v = a[0];
                        if (v.isString())
                            return Value::string(v.str);
                        if (a.size() == 1 || !v.isReal())
                            return Value::string(a.in.format(v, true));
                        int mode = a.integer(1);
                        int prec = a.size() > 2 ? a.integer(2) : -1;
                        FormatSettings fs = a.in.formatSettings();
                        int base = mode % 7;
                        if (base >= 1 && base <= 4)
                            fs.format = base - 1;
                        else if (base == 5 || base == 6)
                            fs.format = 0;
                        if (prec >= 0)
                            fs.digits = prec;
                        if (mode >= 7) {
                            // fraction form via continued fractions
                            double x = v.toReal();
                            double h1 = 1, h0 = 0, k1 = 0, k0 = 1, b = x;
                            for (int i = 0; i < 40; ++i) {
                                double ai = std::floor(b);
                                double h2 = ai * h1 + h0, k2 = ai * k1 + k0;
                                h0 = h1; h1 = h2; k0 = k1; k1 = k2;
                                if (std::fabs(x - h1 / k1) < 1e-12 * std::max(1.0, std::fabs(x)) || b - ai < 1e-15)
                                    break;
                                b = 1 / (b - ai);
                            }
                            if (k1 == 1)
                                return Value::string(formatNumber(h1));
                            return Value::string(formatNumber(h1) + U"/" + formatNumber(k1));
                        }
                        return Value::string(formatValue(v, fs, true));
                    }};

    // ------------------------------------------------------------------ lists
    t[U"SIZE"] = {1, 1, false, [](CallArgs &a) {
                      const Value &v = a[0];
                      if (v.isList())
                          return Value::real(static_cast<double>(v.items().size()));
                      if (v.isMatrix()) {
                          if (v.mat->isVector)
                              return Value::makeList({Value::real(v.mat->cols)});
                          return Value::makeList({Value::real(v.mat->rows), Value::real(v.mat->cols)});
                      }
                      if (v.isString())
                          return Value::real(static_cast<double>(v.str.size()));
                      return Value::real(1);
                  }};
    t[U"MAKELIST"] = {4, 5, true, [](CallArgs &a) {
                          Interpreter &in = a.in;
                          if (a.exprs[1]->kind != ExprKind::Ident)
                              throw RuntimeError("MAKELIST: drugi argument musi być nazwą zmiennej.");
                          std::u32string var = a.exprs[1]->text;
                          double from = in.eval(*a.exprs[2]).toReal(), to = in.eval(*a.exprs[3]).toReal();
                          double step = a.size() > 4 ? in.eval(*a.exprs[4]).toReal() : 1;
                          if (step == 0)
                              throw RuntimeError("MAKELIST: krok nie może być zerem.");
                          ValueList out;
                          for (int i = 0;; ++i) {
                              double x = round12(from + i * step);
                              if (step > 0 ? x > to : x < to)
                                  break;
                              if (i > 1000000)
                                  throw RuntimeError("MAKELIST: za dużo elementów.");
                              out.push_back(in.withBinding(var, Value::real(x), [&] { return in.eval(*a.exprs[0]); }));
                          }
                          return Value::makeList(std::move(out));
                      }};
    t[U"SORT"] = {1, 2, false, [](CallArgs &a) {
                      ValueList l = listArg(a, 0);
                      std::stable_sort(l.begin(), l.end(), [](const Value &x, const Value &y) {
                          if (x.isString() && y.isString())
                              return x.str < y.str;
                          return x.toReal("SORT") < y.toReal("SORT");
                      });
                      return Value::makeList(l);
                  }};
    t[U"REVERSE"] = {1, 1, false, [](CallArgs &a) {
                         if (a[0].isString()) {
                             std::u32string s = a[0].str;
                             std::reverse(s.begin(), s.end());
                             return Value::string(s);
                         }
                         ValueList l = listArg(a, 0);
                         std::reverse(l.begin(), l.end());
                         return Value::makeList(l);
                     }};
    t[U"CONCAT"] = {2, -1, false, [](CallArgs &a) {
                        if (a[0].isString()) {
                            std::u32string s;
                            for (const auto &v : a.values)
                                s += a.in.format(v, false);
                            return Value::string(s);
                        }
                        if (a[0].isMatrix()) {
                            ValueList all;
                            for (const auto &v : a.values)
                                for (const auto &x : elements(v))
                                    all.push_back(x);
                            return Value::vector(all);
                        }
                        ValueList out;
                        for (const auto &v : a.values) {
                            if (v.isList())
                                out.insert(out.end(), v.items().begin(), v.items().end());
                            else
                                out.push_back(v);
                        }
                        return Value::makeList(out);
                    }};
    t[U"POS"] = {2, 2, false, [](CallArgs &a) {
                     const auto e = elements(a[0]);
                     for (size_t i = 0; i < e.size(); ++i)
                         if (valuesEqual(e[i], a[1]))
                             return Value::real(static_cast<double>(i + 1));
                     return Value::real(0);
                 }};
    t[U"ΣLIST"] = {1, 1, false, [](CallArgs &a) {
                       Value s = Value::real(0);
                       for (const auto &v : listArg(a, 0))
                           s = arith(U"+", s, v, a.in);
                       return s;
                   }};
    t[U"ΠLIST"] = {1, 1, false, [](CallArgs &a) {
                       Value s = Value::real(1);
                       for (const auto &v : listArg(a, 0))
                           s = arith(U"*", s, v, a.in);
                       return s;
                   }};
    t[U"ΔLIST"] = {1, 1, false, [](CallArgs &a) {
                       const auto &l = listArg(a, 0);
                       ValueList out;
                       for (size_t i = 1; i < l.size(); ++i)
                           out.push_back(arith(U"-", l[i], l[i - 1], a.in));
                       return Value::makeList(out);
                   }};
    t[U"DIFFERENCE"] = {2, 2, false, [](CallArgs &a) {
                            const auto &x = listArg(a, 0), &y = listArg(a, 1);
                            auto has = [](const ValueList &l, const Value &v) {
                                return std::any_of(l.begin(), l.end(), [&](const Value &e) { return valuesEqual(e, v); });
                            };
                            ValueList out;
                            for (const auto &v : x)
                                if (!has(y, v) && !has(out, v))
                                    out.push_back(v);
                            for (const auto &v : y)
                                if (!has(x, v) && !has(out, v))
                                    out.push_back(v);
                            return Value::makeList(out);
                        }};
    t[U"INTERSECT"] = {2, 2, false, [](CallArgs &a) {
                           const auto &x = listArg(a, 0), &y = listArg(a, 1);
                           ValueList out;
                           for (const auto &v : x)
                               if (std::any_of(y.begin(), y.end(), [&](const Value &e) { return valuesEqual(e, v); })
                                   && std::none_of(out.begin(), out.end(), [&](const Value &e) { return valuesEqual(e, v); }))
                                   out.push_back(v);
                           return Value::makeList(out);
                       }};
    t[U"EVALLIST"] = {1, 1, false, [](CallArgs &a) {
                          ValueList out;
                          for (const auto &v : listArg(a, 0))
                              out.push_back(v.type == Value::Type::Symbolic ? a.in.evalText(v.str) : v);
                          return Value::makeList(out);
                      }};
    t[U"EXECON"] = {2, -1, false, [](CallArgs &a) {
                        std::u32string expr = strArg(a, 0);
                        std::vector<ValueList> lists;
                        for (size_t i = 1; i < a.size(); ++i)
                            lists.push_back(listArg(a, i));
                        ValueList out;
                        // determine how many pairs: substitute &k (single list) or &lk (list l, offset k)
                        size_t maxOffset = 1, startOffset = 0;
                        for (size_t i = 0; i + 1 < expr.size(); ++i) {
                            if (expr[i] != U'&')
                                continue;
                            if (lists.size() == 1 && isDigit(expr[i + 1]))
                                maxOffset = std::max<size_t>(maxOffset, expr[i + 1] - U'0');
                            if (lists.size() > 1 && i + 2 < expr.size() && isDigit(expr[i + 2]) && isDigit(expr[i + 1]))
                                startOffset = std::max<size_t>(startOffset, expr[i + 2] - U'1');
                        }
                        for (size_t pos = 0;; ++pos) {
                            std::u32string e;
                            bool ok = true;
                            for (size_t i = 0; i < expr.size(); ++i) {
                                if (expr[i] != U'&' || i + 1 >= expr.size() || !isDigit(expr[i + 1])) {
                                    e += expr[i];
                                    continue;
                                }
                                size_t listIdx = 0, off = 0;
                                if (lists.size() == 1) {
                                    off = expr[i + 1] - U'1';
                                    i += 1;
                                } else {
                                    listIdx = expr[i + 1] - U'1';
                                    off = 0;
                                    if (i + 2 < expr.size() && isDigit(expr[i + 2])) {
                                        off = expr[i + 2] - U'1';
                                        i += 2;
                                    } else {
                                        i += 1;
                                    }
                                    off = off > startOffset ? off : off; // explicit offset
                                }
                                if (listIdx >= lists.size() || pos + off >= lists[listIdx].size()) {
                                    ok = false;
                                    break;
                                }
                                e += U"(" + a.in.format(lists[listIdx][pos + off]) + U")";
                            }
                            if (!ok)
                                break;
                            out.push_back(a.in.evalText(e));
                            if (pos > 100000)
                                break;
                        }
                        (void)maxOffset;
                        return Value::makeList(out);
                    }};

    // ------------------------------------------------------------------ calculus (lazy)
    t[U"∂"] = {2, 2, true, [](CallArgs &a) {
                   const Expr &eq = *a.exprs[1];
                   if (eq.kind != ExprKind::Binary || eq.text != U"=" || eq.args[0]->kind != ExprKind::Ident)
                       throw RuntimeError("∂: drugi argument musi mieć postać zmienna=wartość, np. X=π/4.");
                   double x0 = a.in.eval(*eq.args[1]).toReal("∂");
                   return Value::real(derivative(exprFunction(a.in, *a.exprs[0], eq.args[0]->text), x0));
               }};
    t[U"∫"] = {4, 4, true, [](CallArgs &a) {
                   if (a.exprs[1]->kind != ExprKind::Ident)
                       throw RuntimeError("∫: drugi argument musi być nazwą zmiennej.");
                   double lo = a.in.eval(*a.exprs[2]).toReal("∫"), hi = a.in.eval(*a.exprs[3]).toReal("∫");
                   return Value::real(integrate(exprFunction(a.in, *a.exprs[0], a.exprs[1]->text), lo, hi));
               }};
    auto sumProd = [](bool product) {
        return Builtin{4, 4, true, [product](CallArgs &a) {
                           if (a.exprs[1]->kind != ExprKind::Ident)
                               throw RuntimeError("Σ/Π: drugi argument musi być nazwą zmiennej.");
                           std::u32string var = a.exprs[1]->text;
                           long long lo = std::llround(a.in.eval(*a.exprs[2]).toReal()), hi = std::llround(a.in.eval(*a.exprs[3]).toReal());
                           Value acc = Value::real(product ? 1 : 0);
                           for (long long k = lo; k <= hi; ++k)
                               acc = arith(product ? U"*" : U"+", acc,
                                           a.in.withBinding(var, Value::real(static_cast<double>(k)), [&] { return a.in.eval(*a.exprs[0]); }), a.in);
                           return acc;
                       }};
    };
    t[U"Σ"] = sumProd(false);
    t[U"Π"] = sumProd(true);
    t[U"ITERATE"] = {4, 4, true, [](CallArgs &a) {
                         if (a.exprs[1]->kind != ExprKind::Ident)
                             throw RuntimeError("ITERATE: drugi argument musi być nazwą zmiennej.");
                         Value v = a.in.eval(*a.exprs[2]);
                         int n = a.in.eval(*a.exprs[3]).toInt("ITERATE");
                         for (int i = 0; i < n; ++i)
                             v = a.in.withBinding(a.exprs[1]->text, v, [&] { return a.in.eval(*a.exprs[0]); });
                         return v;
                     }};

    // ------------------------------------------------------------------ matrices
    auto matArg = [](const CallArgs &a, size_t i) -> const Value & {
        if (!a[i].isMatrix())
            throw RuntimeError(u8(a.name) + ": oczekiwano macierzy, a jest " + typeName(a[i]) + ".");
        return a[i];
    };
    t[U"DET"] = {1, 1, false, [matArg](CallArgs &a) { return Value::real(matDeterminant(matArg(a, 0))); }};
    t[U"TRN"] = {1, 1, false, [matArg](CallArgs &a) {
                     const Matrix &m = *matArg(a, 0).mat;
                     Value r = Value::matrix(m.cols, m.rows);
                     for (int i = 0; i < m.rows; ++i)
                         for (int j = 0; j < m.cols; ++j)
                             r.mat->at(j, i) = m.at(i, j);
                     return r;
                 }};
    t[U"RREF"] = {1, 1, false, [matArg](CallArgs &a) {
                      auto d = toDoubles(matArg(a, 0));
                      rrefInPlace(d, nullptr);
                      return fromDoubles(d);
                  }};
    t[U"RANK"] = {1, 1, false, [matArg](CallArgs &a) {
                      auto d = toDoubles(matArg(a, 0));
                      return Value::real(rrefInPlace(d, nullptr));
                  }};
    t[U"TRACE"] = {1, 1, false, [matArg](CallArgs &a) {
                       const Matrix &m = *matArg(a, 0).mat;
                       Value s = Value::real(0);
                       for (int i = 0; i < std::min(m.rows, m.cols); ++i)
                           s = arith(U"+", s, m.at(i, i), a.in);
                       return s;
                   }};
    t[U"IDENMAT"] = {1, 1, false, [](CallArgs &a) {
                         int n = a.integer(0);
                         Value r = Value::matrix(n, n);
                         for (int i = 0; i < n; ++i)
                             r.mat->at(i, i) = Value::real(1);
                         return r;
                     }};
    t[U"MAKEMAT"] = {2, 3, true, [](CallArgs &a) {
                         int rows = a.in.eval(*a.exprs[1]).toInt("MAKEMAT");
                         int cols = a.size() > 2 ? a.in.eval(*a.exprs[2]).toInt("MAKEMAT") : 1;
                         Value r = Value::matrix(rows, cols, a.size() == 2);
                         for (int i = 0; i < rows; ++i)
                             for (int j = 0; j < cols; ++j)
                                 r.mat->at(i, j) = a.in.withBinding(U"I", Value::real(i + 1), [&] {
                                     return a.in.withBinding(U"J", Value::real(j + 1), [&] { return a.in.eval(*a.exprs[0]); });
                                 });
                         return r;
                     }};
    t[U"DOT"] = {2, 2, false, [](CallArgs &a) {
                     auto x = elements(a[0]), y = elements(a[1]);
                     if (x.size() != y.size())
                         throw RuntimeError("DOT: wektory różnej długości.");
                     Value s = Value::real(0);
                     for (size_t i = 0; i < x.size(); ++i)
                         s = arith(U"+", s, arith(U"*", x[i], y[i], a.in), a.in);
                     return s;
                 }};
    t[U"CROSS"] = {2, 2, false, [](CallArgs &a) {
                       auto x = elements(a[0]), y = elements(a[1]);
                       if (x.size() < 2 || x.size() > 3 || y.size() != x.size())
                           throw RuntimeError("CROSS: oczekiwano dwóch wektorów 2- lub 3-elementowych.");
                       double a1 = x[0].toReal(), a2 = x[1].toReal(), a3 = x.size() > 2 ? x[2].toReal() : 0;
                       double b1 = y[0].toReal(), b2 = y[1].toReal(), b3 = y.size() > 2 ? y[2].toReal() : 0;
                       return Value::vector({Value::real(a2 * b3 - a3 * b2), Value::real(a3 * b1 - a1 * b3), Value::real(a1 * b2 - a2 * b1)});
                   }};
    t[U"LSQ"] = {2, 2, false, [matArg](CallArgs &a) {
                     Value A = matArg(a, 0);
                     Value At = Value::matrix(A.mat->cols, A.mat->rows);
                     for (int i = 0; i < A.mat->rows; ++i)
                         for (int j = 0; j < A.mat->cols; ++j)
                             At.mat->at(j, i) = A.mat->at(i, j);
                     return matMul(matMul(matInverse(matMul(At, A, a.in)), At, a.in), a[1], a.in);
                 }};
    // In-place matrix commands: the first argument names a variable (or is a matrix literal).
    auto inPlace = [](int minA, int maxA, std::function<Value(CallArgs &, Value &)> f) {
        return Builtin{minA, maxA, true, [f](CallArgs &a) {
                           Interpreter &in = a.in;
                           a.values.clear();
                           for (const auto &e : a.exprs)
                               a.values.push_back(e ? in.eval(*e) : Value::real(0));
                           Value m = a.values[0];
                           Value result = f(a, m);
                           if (a.exprs[0]->kind == ExprKind::Ident)
                               in.setVariable(a.exprs[0]->text, m, false);
                           return result;
                       }};
    };
    auto needMat = [](Value &m, const char *name) -> Matrix & {
        if (!m.isMatrix())
            throw RuntimeError(std::string(name) + ": oczekiwano macierzy.");
        return m.mutableMatrix();
    };
    t[U"ADDROW"] = inPlace(3, 3, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "ADDROW");
        auto row = elements(a[1]);
        int at = a.integer(2);
        if (static_cast<int>(row.size()) != x.cols && x.rows > 0)
            throw RuntimeError("ADDROW: wektor ma inną liczbę elementów niż kolumn macierzy.");
        if (x.rows == 0)
            x.cols = static_cast<int>(row.size());
        at = std::clamp(at, 1, x.rows + 1);
        x.data.insert(x.data.begin() + static_cast<long>(at - 1) * x.cols, row.begin(), row.end());
        x.rows++;
        x.isVector = false;
        return m;
    });
    t[U"ADDCOL"] = inPlace(3, 3, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "ADDCOL");
        auto col = elements(a[1]);
        int at = std::clamp(a.integer(2), 1, x.cols + 1);
        if (static_cast<int>(col.size()) != x.rows)
            throw RuntimeError("ADDCOL: wektor ma inną liczbę elementów niż wierszy macierzy.");
        std::vector<Value> d;
        for (int i = 0; i < x.rows; ++i)
            for (int j = 0; j <= x.cols; ++j)
                d.push_back(j == at - 1 ? col[i] : x.at(i, j < at - 1 ? j : j - 1));
        x.data = d;
        x.cols++;
        return m;
    });
    t[U"DELROW"] = inPlace(2, 2, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "DELROW");
        int r = a.integer(1);
        if (r < 1 || r > x.rows)
            throw RuntimeError("DELROW: wiersz poza zakresem.");
        x.data.erase(x.data.begin() + static_cast<long>(r - 1) * x.cols, x.data.begin() + static_cast<long>(r) * x.cols);
        x.rows--;
        return m;
    });
    t[U"DELCOL"] = inPlace(2, 2, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "DELCOL");
        int c = a.integer(1);
        if (c < 1 || c > x.cols)
            throw RuntimeError("DELCOL: kolumna poza zakresem.");
        std::vector<Value> d;
        for (int i = 0; i < x.rows; ++i)
            for (int j = 0; j < x.cols; ++j)
                if (j != c - 1)
                    d.push_back(x.at(i, j));
        x.data = d;
        x.cols--;
        return m;
    });
    t[U"SWAPROW"] = inPlace(3, 3, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "SWAPROW");
        int r1 = a.integer(1), r2 = a.integer(2);
        if (r1 < 1 || r2 < 1 || r1 > x.rows || r2 > x.rows)
            throw RuntimeError("SWAPROW: wiersz poza zakresem.");
        for (int j = 0; j < x.cols; ++j)
            std::swap(x.at(r1 - 1, j), x.at(r2 - 1, j));
        return m;
    });
    t[U"SWAPCOL"] = inPlace(3, 3, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "SWAPCOL");
        int c1 = a.integer(1), c2 = a.integer(2);
        if (c1 < 1 || c2 < 1 || c1 > x.cols || c2 > x.cols)
            throw RuntimeError("SWAPCOL: kolumna poza zakresem.");
        for (int i = 0; i < x.rows; ++i)
            std::swap(x.at(i, c1 - 1), x.at(i, c2 - 1));
        return m;
    });
    t[U"SCALE"] = inPlace(3, 3, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "SCALE");
        int r = a.integer(2);
        if (r < 1 || r > x.rows)
            throw RuntimeError("SCALE: wiersz poza zakresem.");
        for (int j = 0; j < x.cols; ++j)
            x.at(r - 1, j) = arith(U"*", x.at(r - 1, j), a[1], a.in);
        return m;
    });
    t[U"SCALEADD"] = inPlace(4, 4, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "SCALEADD");
        int r1 = a.integer(2), r2 = a.integer(3);
        if (r1 < 1 || r2 < 1 || r1 > x.rows || r2 > x.rows)
            throw RuntimeError("SCALEADD: wiersz poza zakresem.");
        for (int j = 0; j < x.cols; ++j)
            x.at(r2 - 1, j) = arith(U"+", x.at(r2 - 1, j), arith(U"*", x.at(r1 - 1, j), a[1], a.in), a.in);
        return m;
    });
    t[U"REDIM"] = inPlace(2, 2, [needMat](CallArgs &a, Value &m) {
        Matrix &x = needMat(m, "REDIM");
        auto dims = elements(a[1]);
        int rows = dims.size() == 1 ? 1 : dims[0].toInt(), cols = dims.size() == 1 ? dims[0].toInt() : dims[1].toInt();
        std::vector<Value> d(static_cast<size_t>(rows) * cols, Value::real(0));
        for (int i = 0; i < std::min(rows, x.rows); ++i)
            for (int j = 0; j < std::min(cols, x.cols); ++j)
                d[static_cast<size_t>(i) * cols + j] = x.at(i, j);
        x.rows = rows;
        x.cols = cols;
        x.isVector = dims.size() == 1;
        x.data = d;
        return m;
    });
    t[U"REPLACE"] = inPlace(3, 3, [](CallArgs &a, Value &m) {
        if (m.isString()) {
            std::u32string s = m.str, w = a.in.format(a[2], false);
            int start = a[1].isString() ? static_cast<int>(s.find(a[1].str)) + 1 : a.integer(1);
            if (start < 1)
                return m;
            s.replace(start - 1, std::min(w.size(), s.size() - (start - 1)), w);
            if (start - 1 + w.size() > s.size())
                s = s.substr(0, start - 1) + w;
            m = Value::string(s);
            return m;
        }
        if (m.isList()) {
            ValueList &l = m.mutableList();
            int start = a.integer(1);
            auto src = a[2].isList() ? a[2].items() : ValueList{a[2]};
            for (size_t k = 0; k < src.size(); ++k) {
                size_t pos = static_cast<size_t>(start - 1) + k;
                if (pos < l.size()) l[pos] = src[k]; else l.push_back(src[k]);
            }
            return m;
        }
        if (m.isMatrix()) {
            Matrix &x = m.mutableMatrix();
            auto st = elements(a[1]);
            int r0 = st[0].toInt() - 1, c0 = st.size() > 1 ? st[1].toInt() - 1 : 0;
            if (x.isVector) { c0 = r0; r0 = 0; }
            const Value &src = a[2];
            if (src.isMatrix()) {
                for (int i = 0; i < src.mat->rows; ++i)
                    for (int j = 0; j < src.mat->cols; ++j)
                        if (r0 + i < x.rows && c0 + j < x.cols)
                            x.at(r0 + i, c0 + j) = src.mat->at(i, j);
            } else {
                x.at(r0, c0) = src;
            }
            return m;
        }
        throw RuntimeError("REPLACE: nieobsługiwany typ.");
    });
    t[U"SUB"] = {3, 3, false, [](CallArgs &a) {
                     const Value &v = a[0];
                     if (v.isList() || v.isString()) {
                         int s = a.integer(1), e = a.integer(2);
                         if (v.isString())
                             return Value::string(v.str.substr(std::max(0, s - 1), std::max(0, e - s + 1)));
                         ValueList out;
                         for (int k = s; k <= e && k <= static_cast<int>(v.items().size()); ++k)
                             if (k >= 1)
                                 out.push_back(v.items()[k - 1]);
                         return Value::makeList(out);
                     }
                     if (v.isMatrix()) {
                         auto s = elements(a[1]), e = elements(a[2]);
                         int r1 = s[0].toInt(), c1 = s.size() > 1 ? s[1].toInt() : 1, r2 = e[0].toInt(), c2 = e.size() > 1 ? e[1].toInt() : 1;
                         if (v.mat->isVector) {
                             ValueList out;
                             for (int k = r1; k <= r2; ++k)
                                 out.push_back(v.mat->at(0, k - 1));
                             return Value::vector(out);
                         }
                         Value r = Value::matrix(r2 - r1 + 1, c2 - c1 + 1);
                         for (int i = r1; i <= r2; ++i)
                             for (int j = c1; j <= c2; ++j)
                                 r.mat->at(i - r1, j - c1) = v.mat->at(i - 1, j - 1);
                         return r;
                     }
                     throw RuntimeError("SUB: nieobsługiwany typ.");
                 }};
    t[U"LIST2MAT"] = {2, 2, false, [](CallArgs &a) {
                          const auto &l = listArg(a, 0);
                          int cols = a.integer(1), rows = (static_cast<int>(l.size()) + cols - 1) / cols;
                          Value r = Value::matrix(rows, cols);
                          for (size_t k = 0; k < l.size(); ++k)
                              r.mat->data[k] = l[k];
                          return r;
                      }};
    t[U"MAT2LIST"] = {1, 1, false, [](CallArgs &a) { return Value::makeList(elements(a[0])); }};

    // ------------------------------------------------------------------ selected CAS functions
    auto divisors = [](long long n) {
        std::vector<long long> d;
        n = std::llabs(n);
        for (long long k = 1; k * k <= n; ++k)
            if (n % k == 0) {
                d.push_back(k);
                if (k != n / k)
                    d.push_back(n / k);
            }
        std::sort(d.begin(), d.end());
        return d;
    };
    auto isPrime = [](long long n) {
        if (n < 2)
            return false;
        for (long long k = 2; k * k <= n; ++k)
            if (n % k == 0)
                return false;
        return true;
    };
    auto integerArg = [](CallArgs &a, size_t i) {
        double x = a.num(i);
        if (x != std::floor(x) || std::fabs(x) > 9e15)
            throw RuntimeError(u8(a.name) + ": oczekiwano liczby całkowitej.");
        return static_cast<long long>(x);
    };
    t[U"CAS.IDIVIS"] = {1, 1, false, [divisors, integerArg](CallArgs &a) {
                            ValueList out;
                            for (long long d : divisors(integerArg(a, 0)))
                                out.push_back(Value::real(static_cast<double>(d)));
                            return Value::vector(out);
                        }};
    t[U"CAS.ISPRIME"] = {1, 1, false, [isPrime, integerArg](CallArgs &a) { return Value::real(isPrime(integerArg(a, 0))); }};
    t[U"CAS.NEXTPRIME"] = {1, 1, false, [isPrime, integerArg](CallArgs &a) {
                               long long n = integerArg(a, 0) + 1;
                               while (!isPrime(n))
                                   ++n;
                               return Value::real(static_cast<double>(n));
                           }};
    t[U"CAS.PREVPRIME"] = {1, 1, false, [isPrime, integerArg](CallArgs &a) {
                               long long n = integerArg(a, 0) - 1;
                               while (n > 1 && !isPrime(n))
                                   --n;
                               return Value::real(static_cast<double>(n));
                           }};
    auto factors = [](long long n) {
        std::vector<std::pair<long long, int>> f;
        n = std::llabs(n);
        for (long long p = 2; p * p <= n; ++p) {
            int e = 0;
            while (n % p == 0) {
                n /= p;
                ++e;
            }
            if (e)
                f.push_back({p, e});
        }
        if (n > 1)
            f.push_back({n, 1});
        return f;
    };
    t[U"CAS.IFACTORS"] = {1, 1, false, [factors, integerArg](CallArgs &a) {
                              ValueList out;
                              for (auto [p, e] : factors(integerArg(a, 0))) {
                                  out.push_back(Value::real(static_cast<double>(p)));
                                  out.push_back(Value::real(e));
                              }
                              return Value::vector(out);
                          }};
    t[U"CAS.IFACTOR"] = {1, 1, false, [factors, integerArg](CallArgs &a) {
                             std::string s;
                             for (auto [p, e] : factors(integerArg(a, 0))) {
                                 if (!s.empty())
                                     s += "*";
                                 s += std::to_string(p) + (e > 1 ? "^" + std::to_string(e) : "");
                             }
                             return Value::symbolic(toU32(s.empty() ? "1" : s));
                         }};
    t[U"CAS.GCD"] = {2, -1, false, [integerArg](CallArgs &a) {
                         long long g = 0;
                         for (size_t i = 0; i < a.size(); ++i)
                             g = std::gcd(g, integerArg(a, i));
                         return Value::real(static_cast<double>(g));
                     }};
    t[U"CAS.LCM"] = {2, -1, false, [integerArg](CallArgs &a) {
                         long long l = 1;
                         for (size_t i = 0; i < a.size(); ++i)
                             l = std::lcm(l, integerArg(a, i));
                         return Value::real(static_cast<double>(l));
                     }};
    t[U"CAS.IREM"] = t[U"IREM"];
    t[U"CAS.IQUO"] = t[U"IQUO"];
    t[U"CAS.LIST2MAT"] = t[U"LIST2MAT"];
    t[U"CAS.MAT2LIST"] = t[U"MAT2LIST"];
    // plain (unprefixed) access used in Home
    for (const char32_t *n : {U"IDIVIS", U"ISPRIME", U"NEXTPRIME", U"PREVPRIME", U"IFACTORS", U"IFACTOR", U"GCD", U"LCM"})
        t[n] = t[std::u32string(U"CAS.") + n];
}

} // namespace ppl
