#include "v/rational.hpp"

#include <gmp.h>

#include <stdexcept>

namespace v {

struct Rational::Impl {
  mpq_t q;
  Impl() { mpq_init(q); }
  ~Impl() { mpq_clear(q); }
};

namespace {

// "0.1" -> 1/10, "-2.75" -> -11/4. Read exactly, never through floating point.
void setFromDecimal(mpq_t out, const std::string& text) {
  auto dot = text.find('.');
  std::string digits = text.substr(0, dot) + text.substr(dot + 1);
  std::string den = "1" + std::string(text.size() - dot - 1, '0');
  if (mpq_set_str(out, (digits + "/" + den).c_str(), 10) != 0)
    throw std::runtime_error("numeral không đọc được: " + text);
  mpq_canonicalize(out);
}

}  // namespace

Rational::Rational() : p_(new Impl) {}

Rational::Rational(long long n) : p_(new Impl) { mpq_set_si(p_->q, n, 1); }

Rational::Rational(long long num, long long den) : p_(new Impl) {
  if (den == 0) throw RationalDivByZero{};
  mpq_set_si(p_->q, num, den < 0 ? -den : den);
  if (den < 0) mpq_neg(p_->q, p_->q);
  mpq_canonicalize(p_->q);
}

Rational::Rational(const std::string& text) : p_(new Impl) {
  if (text.find('.') != std::string::npos) {
    setFromDecimal(p_->q, text);
    return;
  }
  if (mpq_set_str(p_->q, text.c_str(), 10) != 0)
    throw std::runtime_error("numeral không đọc được: " + text);
  if (mpz_sgn(mpq_denref(p_->q)) == 0) throw RationalDivByZero{};
  mpq_canonicalize(p_->q);
}

Rational::Rational(const Rational& o) : p_(new Impl) { mpq_set(p_->q, o.p_->q); }

Rational::Rational(Rational&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }

Rational& Rational::operator=(const Rational& o) {
  if (this != &o) {
    if (!p_) p_ = new Impl;
    mpq_set(p_->q, o.p_->q);
  }
  return *this;
}

Rational& Rational::operator=(Rational&& o) noexcept {
  if (this != &o) {
    delete p_;
    p_ = o.p_;
    o.p_ = nullptr;
  }
  return *this;
}

Rational::~Rational() { delete p_; }

Rational Rational::operator+(const Rational& o) const {
  Rational r;
  mpq_add(r.p_->q, p_->q, o.p_->q);
  return r;
}

Rational Rational::operator-(const Rational& o) const {
  Rational r;
  mpq_sub(r.p_->q, p_->q, o.p_->q);
  return r;
}

Rational Rational::operator*(const Rational& o) const {
  Rational r;
  mpq_mul(r.p_->q, p_->q, o.p_->q);
  return r;
}

Rational Rational::operator/(const Rational& o) const {
  if (o.isZero()) throw RationalDivByZero{};
  Rational r;
  mpq_div(r.p_->q, p_->q, o.p_->q);
  return r;
}

bool Rational::operator==(const Rational& o) const { return mpq_equal(p_->q, o.p_->q) != 0; }
bool Rational::operator<(const Rational& o) const { return mpq_cmp(p_->q, o.p_->q) < 0; }
bool Rational::isZero() const { return mpq_sgn(p_->q) == 0; }

std::string Rational::str() const {
  char* s = mpq_get_str(nullptr, 10, p_->q);
  std::string out(s);
  void (*freefn)(void*, size_t);
  mp_get_memory_functions(nullptr, nullptr, &freefn);
  freefn(s, out.size() + 1);
  // GMP prints "3/1" for integers; drop the denominator 1 for brevity.
  if (out.size() > 2 && out.compare(out.size() - 2, 2, "/1") == 0)
    out.resize(out.size() - 2);
  return out;
}

}  // namespace v
