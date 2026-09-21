#pragma once
#include <vector>

#include "v/world.hpp"

namespace v {

// The symbolic algebra door: transforms expressions, exactly, without producing numbers.
//
// The normal form is a multivariate rational function — numerator and denominator are
// polynomials with exact rational coefficients, and a "variable" is any object: a declared
// name, `pi`, `e`, or a function value. A single normal form does simplification,
// expansion, adding fractions and adding rational functions.
//
// Cancelling a factor that contains a variable must carry a condition: `(x^2-1)/(x-1) =
// x+1` holds only when `x - 1` is non-zero. Every cancelled factor is returned in
// `nonzero`, and the layer above has to turn them into a proposition that must be
// established.
//
// Throws std::runtime_error if the expression is not algebraic, or the denominator is 0.
Term simplify(World& w, const Term& t, std::vector<Term>* nonzero = nullptr);

}  // namespace v
