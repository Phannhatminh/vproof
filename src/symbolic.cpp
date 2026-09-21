#include "v/symbolic.hpp"

#include <map>
#include <stdexcept>
#include <vector>

namespace v {

namespace {

// Monomial: variable -> exponent, exponents always positive. The empty monomial is the
// constant.
using Mono = std::map<ObjectId, int>;
// Polynomial: monomial -> coefficient, coefficients always non-zero.
using Poly = std::map<Mono, Rational>;

struct Frac {
  Poly num, den;
};

Poly constant(const Rational& c) {
  Poly p;
  if (!c.isZero()) p[Mono{}] = c;
  return p;
}

Poly atomPoly(ObjectId id) {
  Poly p;
  p[Mono{{id, 1}}] = Rational(1);
  return p;
}

void addInto(Poly& into, const Mono& m, const Rational& c) {
  if (c.isZero()) return;
  auto it = into.find(m);
  if (it == into.end()) {
    into[m] = c;
    return;
  }
  Rational sum = it->second + c;
  if (sum.isZero()) into.erase(it);
  else it->second = sum;
}

Poly add(const Poly& a, const Poly& b) {
  Poly out = a;
  for (const auto& [m, c] : b) addInto(out, m, c);
  return out;
}

Poly negate(const Poly& a) {
  Poly out;
  for (const auto& [m, c] : a) out[m] = c * Rational(-1);
  return out;
}

Poly mul(const Poly& a, const Poly& b) {
  Poly out;
  for (const auto& [ma, ca] : a)
    for (const auto& [mb, cb] : b) {
      Mono m = ma;
      for (const auto& [v, e] : mb) m[v] += e;
      addInto(out, m, ca * cb);
    }
  return out;
}

Poly power(const Poly& a, int n) {
  Poly out = constant(Rational(1));
  for (int i = 0; i < n; ++i) out = mul(out, a);
  return out;
}

// Common monomial divisor of the whole polynomial: the smallest exponent of each variable.
Mono commonMono(const Poly& p) {
  Mono g;
  bool first = true;
  for (const auto& [m, c] : p) {
    (void)c;
    if (first) { g = m; first = false; continue; }
    for (auto it = g.begin(); it != g.end();) {
      auto f = m.find(it->first);
      if (f == m.end()) it = g.erase(it);
      else { it->second = it->second < f->second ? it->second : f->second; ++it; }
    }
  }
  return g;
}

Poly divideByMono(const Poly& p, const Mono& g) {
  if (g.empty()) return p;
  Poly out;
  for (const auto& [m, c] : p) {
    Mono r = m;
    for (const auto& [v, e] : g) {
      r[v] -= e;
      if (r[v] == 0) r.erase(v);
    }
    out[r] = c;
  }
  return out;
}

std::vector<ObjectId> varsOf(const Poly& a, const Poly& b) {
  std::map<ObjectId, bool> seen;
  for (const Poly* p : {&a, &b})
    for (const auto& [m, c] : *p) {
      (void)c;
      for (const auto& [v, e] : m) { (void)e; seen[v] = true; }
    }
  std::vector<ObjectId> out;
  for (const auto& [v, ok] : seen) { (void)ok; out.push_back(v); }
  return out;
}

// --- one variable: coefficients by degree, degree 0 first ---
using Uni = std::vector<Rational>;

Uni toUni(const Poly& p, ObjectId v) {
  Uni u;
  for (const auto& [m, c] : p) {
    size_t d = 0;
    auto it = m.find(v);
    if (it != m.end()) d = static_cast<size_t>(it->second);
    if (u.size() <= d) u.resize(d + 1, Rational(0));
    u[d] = u[d] + c;
  }
  while (!u.empty() && u.back().isZero()) u.pop_back();
  return u;
}

Poly fromUni(const Uni& u, ObjectId v) {
  Poly p;
  for (size_t d = 0; d < u.size(); ++d) {
    if (u[d].isZero()) continue;
    Mono m;
    if (d) m[v] = static_cast<int>(d);
    p[m] = u[d];
  }
  return p;
}

// Division with remainder; returns false if it does not divide exactly.
bool uniDivide(Uni a, const Uni& b, Uni& q, bool wantExact) {
  if (b.empty()) return false;
  q.assign(a.size() >= b.size() ? a.size() - b.size() + 1 : 0, Rational(0));
  while (a.size() >= b.size() && !a.empty()) {
    size_t shift = a.size() - b.size();
    Rational factor = a.back() / b.back();
    q[shift] = factor;
    for (size_t i = 0; i < b.size(); ++i)
      a[shift + i] = a[shift + i] - factor * b[i];
    while (!a.empty() && a.back().isZero()) a.pop_back();
  }
  return wantExact ? a.empty() : true;
}

Uni uniGcd(Uni a, Uni b) {
  while (!b.empty()) {
    Uni q;
    Uni r = a;
    // r := a mod b
    while (r.size() >= b.size() && !r.empty()) {
      size_t shift = r.size() - b.size();
      Rational factor = r.back() / b.back();
      for (size_t i = 0; i < b.size(); ++i) r[shift + i] = r[shift + i] - factor * b[i];
      while (!r.empty() && r.back().isZero()) r.pop_back();
    }
    a = b;
    b = r;
  }
  if (!a.empty()) {  // normalise to leading coefficient 1
    Rational lead = a.back();
    for (Rational& c : a) c = c / lead;
  }
  return a;
}

// Normal form: divide numerator and denominator by the coefficient of a fixed monomial of
// the denominator. Cancelling coefficients is safe — a non-zero rational is non-zero
// everywhere.
void canonical(Frac& f) {
  if (f.den.empty()) throw std::runtime_error("denominator is 0");
  if (f.num.empty()) {
    f.den = constant(Rational(1));
    return;
  }
  Rational scale = f.den.begin()->second;
  Poly n, d;
  for (const auto& [m, c] : f.num) n[m] = c / scale;
  for (const auto& [m, c] : f.den) d[m] = c / scale;
  f.num = std::move(n);
  f.den = std::move(d);
}

Frac addF(const Frac& a, const Frac& b) {
  Frac r{add(mul(a.num, b.den), mul(b.num, a.den)), mul(a.den, b.den)};
  canonical(r);
  return r;
}

Frac mulF(const Frac& a, const Frac& b) {
  Frac r{mul(a.num, b.num), mul(a.den, b.den)};
  canonical(r);
  return r;
}

Frac divF(const Frac& a, const Frac& b) {
  if (b.num.empty()) throw std::runtime_error("division by zero");
  Frac r{mul(a.num, b.den), mul(a.den, b.num)};
  canonical(r);
  return r;
}

const std::string& opName(const World& w, const Term& t) {
  static const std::string none;
  if (t.kind != TermKind::Tuple || t.elems.empty() || t.elems[0].kind != TermKind::Obj)
    return none;
  return w.obj(t.elems[0].obj).name;
}

Frac build(World& w, const Term& t) {
  const std::string& op = opName(w, t);
  if (!op.empty() && t.elems.size() == 3 &&
      (op == "Plus" || op == "Minus" || op == "Times" || op == "Div" || op == "Pow")) {
    if (op == "Pow") {
      Frac base = build(w, t.elems[1]);
      const Term& e = t.elems[2];
      if (e.kind != TermKind::Obj || w.obj(e.obj).kind != Kind::Numeral)
        throw std::runtime_error("the exponent must be a non-negative integer");
      Rational v = w.obj(e.obj).value;
      int n = 0;
      while (Rational(n) < v) ++n;
      if (!(Rational(n) == v)) throw std::runtime_error("the exponent must be a non-negative integer");
      Frac r{power(base.num, n), power(base.den, n)};
      canonical(r);
      return r;
    }
    Frac a = build(w, t.elems[1]), b = build(w, t.elems[2]);
    if (op == "Plus") return addF(a, b);
    if (op == "Minus") return addF(a, Frac{negate(b.num), b.den});
    if (op == "Times") return mulF(a, b);
    return divF(a, b);
  }

  // Not an operation: the whole term is a variable of the algebra. Numbers are constants.
  ObjectId id = w.resolve(t);
  if (w.obj(id).kind == Kind::Numeral)
    return Frac{constant(w.obj(id).value), constant(Rational(1))};
  return Frac{atomPoly(id), constant(Rational(1))};
}

Term monoTerm(World& w, const Mono& m, const Rational& c) {
  std::vector<Term> factors;
  bool one = c == Rational(1);
  if (!one || m.empty()) factors.push_back(Term::of(w.numeral(c)));
  for (const auto& [v, e] : m) {
    Term base = Term::of(v);
    if (e == 1) factors.push_back(base);
    else
      factors.push_back(
          Term::ofTuple({Term::of(w.tag("Pow")), base, Term::of(w.numeral(Rational(e)))}));
  }
  Term out = factors[0];
  for (size_t i = 1; i < factors.size(); ++i)
    out = Term::ofTuple({Term::of(w.tag("Times")), out, factors[i]});
  return out;
}

Term polyTerm(World& w, const Poly& p) {
  if (p.empty()) return Term::of(w.numeral(Rational(0)));
  Term out;
  bool first = true;
  for (const auto& [m, c] : p) {
    Term piece = monoTerm(w, m, c);
    if (first) {
      out = piece;
      first = false;
    } else {
      out = Term::ofTuple({Term::of(w.tag("Plus")), out, piece});
    }
  }
  return out;
}

}  // namespace

Term simplify(World& w, const Term& t, std::vector<Term>* nonzero) {
  Frac f = build(w, t);
  canonical(f);

  // Cancel common factors that contain variables. Every factor removed must be non-zero,
  // and that condition goes outside instead of disappearing.
  if (nonzero) {
    Mono gn = commonMono(f.num), gd = commonMono(f.den), g;
    for (const auto& [v, e] : gn) {
      auto it = gd.find(v);
      if (it != gd.end()) g[v] = e < it->second ? e : it->second;
    }
    if (!g.empty()) {
      f.num = divideByMono(f.num, g);
      f.den = divideByMono(f.den, g);
      for (const auto& [v, e] : g) {
        (void)e;
        nonzero->push_back(Term::of(v));
      }
      canonical(f);
    }

    // With a single variable the fraction can be fully reduced with the Euclidean
    // algorithm.
    std::vector<ObjectId> vs = varsOf(f.num, f.den);
    if (vs.size() == 1 && f.den.size() > 1) {
      Uni un = toUni(f.num, vs[0]), ud = toUni(f.den, vs[0]);
      Uni gcd = uniGcd(un, ud);
      if (gcd.size() > 1) {
        Uni qn, qd;
        if (uniDivide(un, gcd, qn, true) && uniDivide(ud, gcd, qd, true)) {
          nonzero->push_back(polyTerm(w, fromUni(gcd, vs[0])));
          f.num = fromUni(qn, vs[0]);
          f.den = fromUni(qd, vs[0]);
          canonical(f);
        }
      }
    }
  }
  Term num = polyTerm(w, f.num);
  if (f.den.size() == 1 && f.den.begin()->first.empty() &&
      f.den.begin()->second == Rational(1))
    return num;
  return Term::ofTuple({Term::of(w.tag("Div")), num, polyTerm(w, f.den)});
}

}  // namespace v
