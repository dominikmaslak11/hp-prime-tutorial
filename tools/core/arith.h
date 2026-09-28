#pragma once

#include "value.h"

#include <vector>

namespace ppl {

class Interpreter;

Value arith(const std::u32string &op, const Value &a, const Value &b, Interpreter &in);
Value matMul(const Value &a, const Value &b, Interpreter &in);
Value matInverse(const Value &m);
double matDeterminant(const Value &m);
int rrefInPlace(std::vector<std::vector<double>> &a, double *det);
std::vector<std::vector<double>> toDoubles(const Value &m);
Value fromDoubles(const std::vector<std::vector<double>> &a);

} // namespace ppl
