// Operators on values: numbers, # integers, complex numbers, strings, lists (element-wise), matrices.
#include "arith.h"

#include "interpreter.h"
#include "utf8.h"

#include <cmath>

namespace ppl {

namespace {

std::string u8(const std::u32string &s) { return toUtf8(s); }

bool isComparison(const std::u32string &op)
{
    return op == U"<" || op == U">" || op == U"<=" || op == U">=";
}

bool isEquality(const std::u32string &op) { return op == U"==" || op == U"=" || op == U"<>"; }

int64_t maskBits(int64_t v, int bits)
{
    if (bits <= 0 || bits >= 64)
        return v;
    return static_cast<int64_t>(static_cast<uint64_t>(v) & ((1ULL << bits) - 1));
}

Value realOp(const std::u32string &op, double x, double y, Interpreter &in)
{
    if (op == U"+") return Value::real(x + y);
    if (op == U"-") return Value::real(x - y);
    if (op == U"*" || op == U".*") return Value::real(x * y);
    if (op == U"/" || op == U"./") {
        if (y == 0)
            throw RuntimeError(x == 0 ? "0/0 — wynik nieoznaczony." : "Dzielenie przez zero.", 3);
        return Value::real(x / y);
    }
    if (op == U"^" || op == U".^") {
        if (x == 0 && y < 0)
            throw RuntimeError("Dzielenie przez zero (0 do potęgi ujemnej).", 3);
        if (x < 0 && y != std::floor(y)) {
            if (!in.complexMode()) {
                // odd roots of negative numbers stay real, e.g. (-8)^(1/3) = -2
                double inv = 1.0 / y;
                if (std::fabs(inv - std::round(inv)) < 1e-9 && static_cast<long long>(std::llround(inv)) % 2 != 0)
                    return Value::real(-std::pow(-x, y));
                throw RuntimeError("Wynik zespolony (potęga liczby ujemnej). Włącz HComplex:=1.");
            }
            return normalizeComplex(std::pow(std::complex<double>(x, 0), y));
        }
        return Value::real(std::pow(x, y));
    }
    if (op == U"MOD") {
        if (y == 0)
            throw RuntimeError("MOD przez zero.", 3);
        return Value::real(x - y * std::floor(x / y));
    }
    double rx = round12(x), ry = round12(y);
    if (op == U"<") return Value::real(rx < ry);
    if (op == U">") return Value::real(rx > ry);
    if (op == U"<=") return Value::real(rx <= ry);
    if (op == U">=") return Value::real(rx >= ry);
    if (op == U"XOR") return Value::real((x != 0) != (y != 0));
    throw RuntimeError("Nieznany operator " + u8(op) + ".");
}

Value complexOp(const std::u32string &op, std::complex<double> x, std::complex<double> y)
{
    if (op == U"+") return normalizeComplex(x + y);
    if (op == U"-") return normalizeComplex(x - y);
    if (op == U"*" || op == U".*") return normalizeComplex(x * y);
    if (op == U"/" || op == U"./") {
        if (y == std::complex<double>(0, 0))
            throw RuntimeError("Dzielenie przez zero.", 3);
        return normalizeComplex(x / y);
    }
    if (op == U"^" || op == U".^") {
        if (y.imag() == 0 && y.real() == std::floor(y.real()) && std::fabs(y.real()) < 1e6) {
            // integer powers exactly by repeated multiplication
            long long n = static_cast<long long>(y.real());
            std::complex<double> r(1, 0), b = n < 0 ? 1.0 / x : x;
            for (long long k = 0; k < std::llabs(n); ++k)
                r *= b;
            return normalizeComplex(r);
        }
        return normalizeComplex(std::pow(x, y));
    }
    throw RuntimeError("Operator " + u8(op) + " nie działa na liczbach zespolonych.");
}

Value intOp(const std::u32string &op, const Value &a, const Value &b, Interpreter &in)
{
    int64_t x = a.ival, y = b.ival;
    int bits = a.bits > 0 ? a.bits : in.formatSettings().bitsDefault;
    auto mk = [&](int64_t v) { return Value::integer(maskBits(v, bits), a.base, a.bits); };
    if (op == U"+") return mk(x + y);
    if (op == U"-") return mk(x - y);
    if (op == U"*") return mk(x * y);
    if (op == U"/") {
        if (y == 0)
            throw RuntimeError("Dzielenie przez zero.", 3);
        return mk(x / y);
    }
    if (op == U"MOD") {
        if (y == 0)
            throw RuntimeError("MOD przez zero.", 3);
        return mk(x % y);
    }
    if (op == U"^") {
        int64_t r = 1;
        for (int64_t k = 0; k < y && k < 64; ++k)
            r *= x;
        return mk(r);
    }
    if (op == U"<") return Value::real(x < y);
    if (op == U">") return Value::real(x > y);
    if (op == U"<=") return Value::real(x <= y);
    if (op == U">=") return Value::real(x >= y);
    return realOp(op, static_cast<double>(x), static_cast<double>(y), in);
}

Value listOp(const std::u32string &op, const Value &a, const Value &b, Interpreter &in)
{
    ValueList out;
    if (a.isList() && b.isList()) {
        const auto &x = a.items(), &y = b.items();
        if (x.size() != y.size())
            throw RuntimeError("Listy mają różne długości (" + std::to_string(x.size()) + " i " + std::to_string(y.size())
                               + ").");
        for (size_t i = 0; i < x.size(); ++i)
            out.push_back(arith(op, x[i], y[i], in));
    } else if (a.isList()) {
        for (const auto &v : a.items())
            out.push_back(arith(op, v, b, in));
    } else {
        for (const auto &v : b.items())
            out.push_back(arith(op, a, v, in));
    }
    return Value::makeList(std::move(out));
}

Value elementwise(const std::u32string &op, const Value &a, const Value &b, Interpreter &in)
{
    std::u32string base = op == U".*" ? U"*" : op == U"./" ? U"/" : op == U".^" ? U"^" : op;
    if (a.isMatrix() && b.isMatrix()) {
        if (a.mat->rows != b.mat->rows || a.mat->cols != b.mat->cols)
            throw RuntimeError("Niezgodne wymiary macierzy.");
        Value r = a;
        Matrix &m = r.mutableMatrix();
        for (size_t i = 0; i < m.data.size(); ++i)
            m.data[i] = arith(base, a.mat->data[i], b.mat->data[i], in);
        return r;
    }
    const Value &mv = a.isMatrix() ? a : b;
    Value r = mv;
    Matrix &m = r.mutableMatrix();
    for (size_t i = 0; i < m.data.size(); ++i)
        m.data[i] = a.isMatrix() ? arith(base, mv.mat->data[i], b, in) : arith(base, a, mv.mat->data[i], in);
    return r;
}

} // namespace

Value matMul(const Value &a, const Value &b, Interpreter &in)
{
    const Matrix &x = *a.mat, &y = *b.mat;
    // vector · vector treated as row × column when sizes allow
    if (x.cols != y.rows) {
        if (x.isVector && y.isVector && x.cols == y.cols)
            throw RuntimeError("Mnożenie dwóch wektorów: użyj DOT(v1, v2) albo CROSS(v1, v2).");
        throw RuntimeError("Niezgodne wymiary do mnożenia macierzy (" + std::to_string(x.rows) + "×" + std::to_string(x.cols)
                           + " i " + std::to_string(y.rows) + "×" + std::to_string(y.cols) + ").");
    }
    Value r = Value::matrix(x.rows, y.cols);
    for (int i = 0; i < x.rows; ++i)
        for (int j = 0; j < y.cols; ++j) {
            Value s = Value::real(0);
            for (int k = 0; k < x.cols; ++k)
                s = arith(U"+", s, arith(U"*", x.at(i, k), y.at(k, j), in), in);
            r.mat->at(i, j) = s;
        }
    if (r.mat->rows == 1 && x.isVector)
        r.mat->isVector = true;
    return r;
}

std::vector<std::vector<double>> toDoubles(const Value &m)
{
    std::vector<std::vector<double>> a(m.mat->rows, std::vector<double>(m.mat->cols));
    for (int i = 0; i < m.mat->rows; ++i)
        for (int j = 0; j < m.mat->cols; ++j)
            a[i][j] = m.mat->at(i, j).toReal("macierz");
    return a;
}

Value fromDoubles(const std::vector<std::vector<double>> &a)
{
    int rows = static_cast<int>(a.size()), cols = rows ? static_cast<int>(a[0].size()) : 0;
    Value r = Value::matrix(rows, cols);
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            r.mat->at(i, j) = Value::real(a[i][j]);
    return r;
}

// Gauss–Jordan elimination; returns rank, fills determinant.
int rrefInPlace(std::vector<std::vector<double>> &a, double *det)
{
    int rows = static_cast<int>(a.size()), cols = rows ? static_cast<int>(a[0].size()) : 0;
    int r = 0;
    double d = 1;
    for (int c = 0; c < cols && r < rows; ++c) {
        int piv = r;
        for (int i = r + 1; i < rows; ++i)
            if (std::fabs(a[i][c]) > std::fabs(a[piv][c]))
                piv = i;
        if (std::fabs(a[piv][c]) < 1e-12) {
            d = 0;
            continue;
        }
        if (piv != r) {
            std::swap(a[piv], a[r]);
            d = -d;
        }
        double p = a[r][c];
        d *= p;
        for (int j = 0; j < cols; ++j)
            a[r][j] /= p;
        for (int i = 0; i < rows; ++i) {
            if (i == r || a[i][c] == 0)
                continue;
            double f = a[i][c];
            for (int j = 0; j < cols; ++j)
                a[i][j] -= f * a[r][j];
        }
        ++r;
    }
    if (r < rows || rows != cols)
        d = r < rows ? 0 : d;
    if (det)
        *det = d;
    return r;
}

Value matInverse(const Value &m)
{
    const Matrix &x = *m.mat;
    if (x.rows != x.cols || x.rows == 0)
        throw RuntimeError("Odwrotność istnieje tylko dla macierzy kwadratowej.");
    int n = x.rows;
    auto a = toDoubles(m);
    for (int i = 0; i < n; ++i) {
        a[i].resize(2 * n, 0);
        a[i][n + i] = 1;
    }
    double det;
    int rank = rrefInPlace(a, &det);
    for (int i = 0; i < n; ++i)
        if (rank < n || std::fabs(a[i][i] - 1) > 1e-9)
            throw RuntimeError("Macierz osobliwa (wyznacznik = 0) — brak odwrotności.");
    std::vector<std::vector<double>> inv(n, std::vector<double>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            inv[i][j] = a[i][n + j];
    return fromDoubles(inv);
}

double matDeterminant(const Value &m)
{
    if (m.mat->rows != m.mat->cols)
        throw RuntimeError("Wyznacznik istnieje tylko dla macierzy kwadratowej.");
    auto a = toDoubles(m);
    double det = 1;
    int n = m.mat->rows;
    for (int c = 0; c < n; ++c) {
        int piv = c;
        for (int i = c + 1; i < n; ++i)
            if (std::fabs(a[i][c]) > std::fabs(a[piv][c]))
                piv = i;
        if (a[piv][c] == 0)
            return 0;
        if (piv != c) {
            std::swap(a[piv], a[c]);
            det = -det;
        }
        det *= a[c][c];
        for (int i = c + 1; i < n; ++i) {
            double f = a[i][c] / a[c][c];
            for (int j = c; j < n; ++j)
                a[i][j] -= f * a[c][j];
        }
    }
    return det;
}

Value arith(const std::u32string &op, const Value &a, const Value &b, Interpreter &in)
{
    if (a.type == Value::Type::Symbolic)
        return arith(op, in.evalText(a.str), b, in);
    if (b.type == Value::Type::Symbolic)
        return arith(op, a, in.evalText(b.str), in);

    if (op == U"+" && (a.isString() || b.isString()))
        return Value::string(in.format(a, false) + in.format(b, false));

    if (isEquality(op)) {
        if (a.isList() && b.isList() && a.items().size() == b.items().size() && op != U"<>") {
            // element-wise comparison would be ambiguous in IF; compare whole lists
            return Value::real(valuesEqual(a, b));
        }
        bool eq = valuesEqual(a, b);
        return Value::real(op == U"<>" ? !eq : eq);
    }

    if (a.isList() || b.isList())
        return listOp(op, a, b, in);

    if (a.isString() || b.isString()) {
        if (a.isString() && b.isString() && isComparison(op)) {
            int c = a.str.compare(b.str);
            bool r = op == U"<" ? c < 0 : op == U">" ? c > 0 : op == U"<=" ? c <= 0 : c >= 0;
            return Value::real(r);
        }
        throw RuntimeError("Operator " + u8(op) + " nie działa na tekstach (" + typeName(a) + ", " + typeName(b) + ").");
    }

    if (a.isMatrix() || b.isMatrix()) {
        if (op == U".*" || op == U"./" || op == U".^")
            return elementwise(op, a, b, in);
        if (op == U"+" || op == U"-") {
            if (!(a.isMatrix() && b.isMatrix()))
                throw RuntimeError("Dodawanie liczby do macierzy nie jest dozwolone; użyj .+ na elementach lub pętli.");
            return elementwise(op, a, b, in);
        }
        if (op == U"*") {
            if (a.isMatrix() && b.isMatrix())
                return matMul(a, b, in);
            return elementwise(U"*", a, b, in);
        }
        if (op == U"/") {
            if (b.isMatrix()) {
                if (a.isMatrix())
                    return matMul(a, matInverse(b), in);
                return elementwise(U"*", a, matInverse(b), in);
            }
            return elementwise(U"/", a, b, in);
        }
        if (op == U"^" && a.isMatrix() && b.isNumber()) {
            int n = b.toInt("potęga");
            if (a.mat->rows != a.mat->cols)
                throw RuntimeError("Potęga macierzy wymaga macierzy kwadratowej.");
            Value base = n < 0 ? matInverse(a) : a;
            int k = std::abs(n);
            Value r = Value::matrix(a.mat->rows, a.mat->cols);
            for (int i = 0; i < a.mat->rows; ++i)
                r.mat->at(i, i) = Value::real(1);
            for (int i = 0; i < k; ++i)
                r = matMul(r, base, in);
            return r;
        }
        throw RuntimeError("Operator " + u8(op) + " nie działa na macierzach.");
    }

    if (a.type == Value::Type::Integer && b.type == Value::Type::Integer)
        return intOp(op, a, b, in);
    if (a.type == Value::Type::Complex || b.type == Value::Type::Complex) {
        if (isComparison(op))
            throw RuntimeError("Liczb zespolonych nie można porównywać operatorem " + u8(op) + ".");
        return complexOp(op, a.toComplex(), b.toComplex());
    }
    if (a.isNumber() && b.isNumber())
        return realOp(op, a.toReal(), b.toReal(), in);
    throw RuntimeError("Operator " + u8(op) + " nie działa na typach " + typeName(a) + " i " + typeName(b) + ".");
}

} // namespace ppl
