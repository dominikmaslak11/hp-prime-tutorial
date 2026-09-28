#include "value.h"

#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ppl {

Value &Matrix::at(int r, int c) { return data[static_cast<size_t>(r) * cols + c]; }
const Value &Matrix::at(int r, int c) const { return data[static_cast<size_t>(r) * cols + c]; }

int &fullPrecisionDepth()
{
    thread_local int depth = 0;
    return depth;
}

double round12(double x)
{
    if (fullPrecisionDepth() > 0 || !std::isfinite(x) || x == 0)
        return x;
    char buf[40];
    std::snprintf(buf, sizeof buf, "%.11e", x);
    return std::strtod(buf, nullptr);
}

Value Value::real(double x)
{
    Value v;
    v.type = Type::Real;
    v.re = round12(x);
    if (v.re == 0)
        v.re = 0; // no negative zero
    return v;
}

Value Value::complex(double r, double i)
{
    Value v;
    v.type = Type::Complex;
    v.re = round12(r);
    v.im = round12(i);
    return v;
}

Value normalizeComplex(std::complex<double> z)
{
    double r = round12(z.real()), i = round12(z.imag());
    if (i == 0 || (r != 0 && std::fabs(i) < std::fabs(r) * 1e-14))
        return Value::real(r);
    return Value::complex(r, i);
}

Value Value::integer(int64_t v, char base, int bits)
{
    Value x;
    x.type = Type::Integer;
    x.ival = v;
    x.base = base;
    x.bits = bits;
    return x;
}

Value Value::string(std::u32string s)
{
    Value v;
    v.type = Type::String;
    v.str = std::move(s);
    return v;
}

Value Value::symbolic(std::u32string expr)
{
    Value v;
    v.type = Type::Symbolic;
    v.str = std::move(expr);
    return v;
}

Value Value::makeList(ValueList items)
{
    Value v;
    v.type = Type::List;
    v.list = std::make_shared<ValueList>(std::move(items));
    return v;
}

Value Value::matrix(int rows, int cols, bool isVector)
{
    Value v;
    v.type = Type::Matrix;
    v.mat = std::make_shared<Matrix>();
    v.mat->rows = rows;
    v.mat->cols = cols;
    v.mat->isVector = isVector;
    v.mat->data.assign(static_cast<size_t>(rows) * cols, Value::real(0));
    return v;
}

Value Value::vector(const ValueList &items)
{
    Value v = matrix(1, static_cast<int>(items.size()), true);
    for (size_t i = 0; i < items.size(); ++i)
        v.mat->data[i] = items[i];
    return v;
}

Value Value::graphic(int index)
{
    Value v;
    v.type = Type::Graphic;
    v.grob = index;
    return v;
}

double Value::toReal(const char *what) const
{
    switch (type) {
    case Type::Real: return re;
    case Type::Integer: return static_cast<double>(ival);
    case Type::Complex:
        if (im == 0)
            return re;
        break;
    default: break;
    }
    throw RuntimeError(std::string("Zły typ argumentu") + (what ? std::string(" (") + what + ")" : "")
                       + ": oczekiwano liczby rzeczywistej, a jest " + typeName(*this) + ".");
}

std::complex<double> Value::toComplex() const
{
    if (type == Type::Complex)
        return {re, im};
    return {toReal(), 0};
}

int Value::toInt(const char *what) const
{
    double x = toReal(what);
    if (!std::isfinite(x) || std::fabs(x) > 2e9)
        throw RuntimeError(std::string("Wartość poza zakresem") + (what ? std::string(" (") + what + ")" : "") + ".");
    return static_cast<int>(std::llround(x));
}

bool Value::truthy() const
{
    switch (type) {
    case Type::Real: return re != 0;
    case Type::Integer: return ival != 0;
    case Type::Complex: return re != 0 || im != 0;
    default: throw RuntimeError("Warunek musi być liczbą, a jest " + typeName(*this) + ".");
    }
}

ValueList &Value::mutableList()
{
    if (!list)
        list = std::make_shared<ValueList>();
    else if (list.use_count() > 1)
        list = std::make_shared<ValueList>(*list);
    return *list;
}

Matrix &Value::mutableMatrix()
{
    if (!mat)
        mat = std::make_shared<Matrix>();
    else if (mat.use_count() > 1)
        mat = std::make_shared<Matrix>(*mat);
    return *mat;
}

const ValueList &Value::items() const
{
    static const ValueList empty;
    return list ? *list : empty;
}

int Value::typeCode() const
{
    switch (type) {
    case Type::Real: return 0;
    case Type::Integer: return 1;
    case Type::String: return 2;
    case Type::Complex: return 3;
    case Type::Matrix: return 4;
    case Type::List: return 6;
    case Type::Symbolic: return 8;
    case Type::Graphic: return 7;
    }
    return 0;
}

bool valuesEqual(const Value &a, const Value &b)
{
    if (a.isNumber() && b.isNumber()) {
        if (a.type == Value::Type::Integer && b.type == Value::Type::Integer)
            return a.ival == b.ival;
        auto x = a.toComplex(), y = b.toComplex();
        return round12(x.real()) == round12(y.real()) && round12(x.imag()) == round12(y.imag());
    }
    if (a.type != b.type)
        return false;
    switch (a.type) {
    case Value::Type::String:
    case Value::Type::Symbolic: return a.str == b.str;
    case Value::Type::List: {
        const auto &x = a.items(), &y = b.items();
        if (x.size() != y.size())
            return false;
        for (size_t i = 0; i < x.size(); ++i)
            if (!valuesEqual(x[i], y[i]))
                return false;
        return true;
    }
    case Value::Type::Matrix: {
        if (a.mat->rows != b.mat->rows || a.mat->cols != b.mat->cols)
            return false;
        for (size_t i = 0; i < a.mat->data.size(); ++i)
            if (!valuesEqual(a.mat->data[i], b.mat->data[i]))
                return false;
        return true;
    }
    case Value::Type::Graphic: return a.grob == b.grob;
    default: return false;
    }
}

namespace {

std::string trimZeros(std::string s)
{
    size_t e = s.find_first_of("eE");
    std::string exp = e == std::string::npos ? "" : s.substr(e);
    std::string mant = e == std::string::npos ? s : s.substr(0, e);
    if (mant.find('.') != std::string::npos) {
        while (!mant.empty() && mant.back() == '0')
            mant.pop_back();
        if (!mant.empty() && mant.back() == '.')
            mant.pop_back();
    }
    return mant + exp;
}

std::string sciString(double x, int digits)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*e", digits, x);
    std::string s = buf;
    // 1.5e-05 -> 1.5E-5
    size_t e = s.find('e');
    std::string mant = s.substr(0, e);
    int exp = std::atoi(s.c_str() + e + 1);
    return mant + "E" + std::to_string(exp);
}

} // namespace

std::u32string formatNumber(double x, const FormatSettings &fs)
{
    if (std::isnan(x))
        return U"NaN";
    if (std::isinf(x))
        return x > 0 ? U"∞" : U"-∞";
    if (x == 0)
        return U"0";
    std::string out;
    double ax = std::fabs(x);
    if (fs.format == 1) { // Fixed
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.*f", std::clamp(fs.digits, 0, 11), x);
        out = buf;
    } else if (fs.format == 2) { // Scientific
        out = sciString(x, std::clamp(fs.digits, 0, 11));
    } else if (fs.format == 3) { // Engineering
        int e = static_cast<int>(std::floor(std::log10(ax)));
        int e3 = static_cast<int>(std::floor(e / 3.0)) * 3;
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.*f", std::clamp(fs.digits, 0, 11), x / std::pow(10.0, e3));
        out = std::string(buf) + "E" + std::to_string(e3);
    } else { // Standard: up to 12 significant digits
        double r = round12(x);
        double ar = std::fabs(r);
        if (ar >= 1e12 || ar < 1e-3) {
            out = trimZeros(sciString(r, 11));
            size_t e = out.find('E');
            // trimZeros works on the mantissa; rebuild exponent without leading +
            (void)e;
        } else {
            int intDigits = static_cast<int>(std::floor(std::log10(ar))) + 1;
            int decimals = std::clamp(12 - intDigits, 0, 16);
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.*f", decimals, r);
            out = trimZeros(buf);
        }
    }
    return toU32(out);
}

namespace {

std::u32string formatInteger(const Value &v, const FormatSettings &fs)
{
    int bits = v.bits > 0 ? v.bits : fs.bitsDefault;
    uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1);
    uint64_t u = static_cast<uint64_t>(v.ival) & mask;
    int radix = v.base == 'b' ? 2 : v.base == 'o' ? 8 : v.base == 'd' ? 10 : 16;
    std::string digits;
    if (u == 0)
        digits = "0";
    while (u > 0) {
        int d = static_cast<int>(u % radix);
        digits.insert(digits.begin(), static_cast<char>(d < 10 ? '0' + d : 'A' + d - 10));
        u /= radix;
    }
    std::string s = "#" + digits;
    if (v.bits > 0)
        s += ":" + std::to_string(v.bits);
    s.push_back(v.base);
    return toU32(s);
}

} // namespace

std::u32string formatValue(const Value &v, const FormatSettings &fs, bool quoteStrings)
{
    switch (v.type) {
    case Value::Type::Real: return formatNumber(v.re, fs);
    case Value::Type::Integer: return formatInteger(v, fs);
    case Value::Type::Complex: {
        std::u32string s;
        if (v.re != 0)
            s = formatNumber(v.re, fs);
        std::u32string im = formatNumber(std::fabs(v.im), fs);
        if (v.im < 0)
            s += U"-";
        else if (!s.empty())
            s += U"+";
        s += (im == U"1" ? U"" : im + U"*") + std::u32string(U"i");
        return s;
    }
    case Value::Type::String: {
        if (!quoteStrings)
            return v.str;
        std::u32string s = U"\"";
        for (char32_t c : v.str) {
            if (c == U'"') // the calculator shows an embedded quote as \"
                s += U"\\\"";
            else
                s += c;
        }
        return s + U"\"";
    }
    case Value::Type::Symbolic: return U"'" + v.str + U"'";
    case Value::Type::List: {
        std::u32string s = U"{";
        const auto &l = v.items();
        for (size_t i = 0; i < l.size(); ++i) {
            if (i)
                s += U",";
            s += formatValue(l[i], fs, true);
        }
        return s + U"}";
    }
    case Value::Type::Matrix: {
        const Matrix &m = *v.mat;
        auto row = [&](int r) {
            std::u32string s = U"[";
            for (int c = 0; c < m.cols; ++c) {
                if (c)
                    s += U",";
                s += formatValue(m.at(r, c), fs, true);
            }
            return s + U"]";
        };
        if (m.isVector)
            return m.rows > 0 ? row(0) : U"[]";
        std::u32string s = U"[";
        for (int r = 0; r < m.rows; ++r) {
            if (r)
                s += U",";
            s += row(r);
        }
        return s + U"]";
    }
    case Value::Type::Graphic: return U"G" + toU32(std::to_string(v.grob));
    }
    return U"";
}

std::string typeName(const Value &v)
{
    switch (v.type) {
    case Value::Type::Real: return "liczba rzeczywista";
    case Value::Type::Integer: return "liczba całkowita (#)";
    case Value::Type::Complex: return "liczba zespolona";
    case Value::Type::String: return "tekst";
    case Value::Type::List: return "lista";
    case Value::Type::Matrix: return v.mat && v.mat->isVector ? "wektor" : "macierz";
    case Value::Type::Graphic: return "grafika";
    case Value::Type::Symbolic: return "wyrażenie";
    }
    return "?";
}

} // namespace ppl
