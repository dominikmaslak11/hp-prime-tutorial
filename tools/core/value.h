#pragma once

#include <complex>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ppl {

struct Value;
using ValueList = std::vector<Value>;

struct Matrix {
    int rows = 0;
    int cols = 0;
    bool isVector = false;       // [1,2,3] (one row, displayed with single brackets)
    std::vector<Value> data;     // row-major; elements are Real or Complex
    Value &at(int r, int c);     // 0-based
    const Value &at(int r, int c) const;
};

// Runtime error raised by the interpreter (message in Polish).
struct RuntimeError : std::runtime_error {
    int code;
    explicit RuntimeError(const std::string &msg, int code = 1) : std::runtime_error(msg), code(code) {}
};

struct Value {
    enum class Type { Real, Integer, Complex, String, List, Matrix, Graphic, Symbolic };

    Type type = Type::Real;
    double re = 0;                    // Real, Complex
    double im = 0;                    // Complex
    int64_t ival = 0;                 // Integer (#)
    char base = 'h';                  // Integer: b o d h
    int bits = 0;                     // Integer: explicit wordsize (0 = default)
    std::u32string str;               // String, Symbolic (expression text)
    std::shared_ptr<ValueList> list;  // List
    std::shared_ptr<Matrix> mat;      // Matrix / vector
    int grob = -1;                    // Graphic: G0..G9

    Value() = default;
    static Value real(double x);
    static Value complex(double r, double i);
    static Value integer(int64_t v, char base, int bits = 0);
    static Value string(std::u32string s);
    static Value symbolic(std::u32string expr);
    static Value makeList(ValueList items = {});
    static Value matrix(int rows, int cols, bool isVector = false);
    static Value vector(const ValueList &items);
    static Value graphic(int index);

    bool isReal() const { return type == Type::Real; }
    bool isNumber() const { return type == Type::Real || type == Type::Integer || type == Type::Complex; }
    bool isList() const { return type == Type::List; }
    bool isMatrix() const { return type == Type::Matrix; }
    bool isString() const { return type == Type::String; }

    // Numeric conversions (throw RuntimeError on wrong type).
    double toReal(const char *what = nullptr) const;
    std::complex<double> toComplex() const;
    int toInt(const char *what = nullptr) const;   // rounds to nearest
    bool truthy() const;

    ValueList &mutableList();   // copy-on-write
    Matrix &mutableMatrix();
    const ValueList &items() const;

    int typeCode() const;       // TYPE() result
};

// Rounds to 12 significant digits like the calculator's display precision.
double round12(double x);
// While > 0, round12 does nothing (numerical algorithms keep full double precision internally).
int &fullPrecisionDepth();
struct FullPrecision {
    FullPrecision() { ++fullPrecisionDepth(); }
    ~FullPrecision() { --fullPrecisionDepth(); }
};
Value normalizeComplex(std::complex<double> z);

bool valuesEqual(const Value &a, const Value &b);

struct FormatSettings {
    int format = 0;      // HFormat: 0 Standard, 1 Fixed, 2 Scientific, 3 Engineering
    int digits = 4;      // HDigits
    int bitsDefault = 32;
};

std::u32string formatNumber(double x, const FormatSettings &fs = {});
std::u32string formatValue(const Value &v, const FormatSettings &fs = {}, bool quoteStrings = true);
std::string typeName(const Value &v);

} // namespace ppl
