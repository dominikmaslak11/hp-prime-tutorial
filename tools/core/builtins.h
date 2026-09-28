#pragma once

#include "ast.h"
#include "value.h"

#include <functional>
#include <map>
#include <string>

namespace ppl {

class Interpreter;
struct Builtin;
using BuiltinTable = std::map<std::u32string, Builtin>;

void registerCoreBuiltins(Interpreter &in, BuiltinTable &t);
void registerIoBuiltins(Interpreter &in, BuiltinTable &t);
void registerAppBuiltins(Interpreter &in, BuiltinTable &t);

// Numerical helpers shared between modules
double integrate(const std::function<double(double)> &f, double a, double b);
double findRoot(const std::function<double(double)> &f, double guess);
double numericDerivative(const std::function<double(double)> &f, double x);
std::function<double(double)> exprFunction(Interpreter &in, const Expr &expr, const std::u32string &var);

} // namespace ppl
