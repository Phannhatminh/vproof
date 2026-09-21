#pragma once
#include <string>

namespace v {

// Exact rationals: numerator and denominator of unbounded size, always reduced. GMP sits
// behind this class and is not exposed by any other header, so the backend can be swapped.
class Rational {
 public:
  Rational();
  Rational(long long n);
  Rational(long long num, long long den);
  // Reads "3", "-7/4", "0.1" — decimals are read exactly, 0.1 is 1/10.
  explicit Rational(const std::string& text);

  Rational(const Rational&);
  Rational(Rational&&) noexcept;
  Rational& operator=(const Rational&);
  Rational& operator=(Rational&&) noexcept;
  ~Rational();

  Rational operator+(const Rational&) const;
  Rational operator-(const Rational&) const;
  Rational operator*(const Rational&) const;
  // Division by zero throws RationalDivByZero; the layer above turns it into a "no value"
  // fact, not a program error.
  Rational operator/(const Rational&) const;

  bool operator==(const Rational&) const;
  bool operator!=(const Rational& o) const { return !(*this == o); }
  bool operator<(const Rational&) const;
  bool operator<=(const Rational& o) const { return *this < o || *this == o; }

  bool isZero() const;
  std::string str() const;  // "3", "-7/4"

 private:
  struct Impl;
  Impl* p_;
};

struct RationalDivByZero {};

}  // namespace v
