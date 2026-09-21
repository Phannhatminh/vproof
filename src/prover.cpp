#include "v/prover.hpp"

#include <algorithm>
#include <functional>

namespace v {

Prover::Result Prover::fail(const std::string& msg) const {
  Result r;
  r.error = msg;
  return r;
}

Prover::Result Prover::missing(PropId p) const {
  Result r;
  r.missing = p;
  r.error = "chưa có: " + w_.showProp(p);
  return r;
}

// ------------------------------------------------------------------ stipulating

Prover::Result Prover::assume(PropId p, int line) {
  w_.tell(p, Reason::stipulate(line));
  Result r;
  r.ok = true;
  r.prop = p;
  return r;
}

// ---------------------------------------------------------------- introduction

Prover::Result Prover::introAnd(PropId a, PropId b, int line) {
  if (!w_.holds(a)) return missing(a);
  if (!w_.holds(b)) return missing(b);

  PropId out = w_.conj(a, b);
  Reason why = Reason::derive("and", line);
  why.antecedents = {Antecedent{a}, Antecedent{b}};
  w_.tell(out, std::move(why));

  Result r;
  r.ok = true;
  r.prop = out;
  return r;
}

Prover::Result Prover::introOr(PropId held, PropId other, bool heldOnLeft, int line) {
  if (!w_.holds(held)) return missing(held);

  // The other side needs no lookup: it is chosen freely.
  PropId out = heldOnLeft ? w_.disj(held, other) : w_.disj(other, held);
  Reason why = Reason::derive("or", line);
  why.antecedents = {Antecedent{held}};
  w_.tell(out, std::move(why));

  Result r;
  r.ok = true;
  r.prop = out;
  return r;
}

Prover::Result Prover::introIff(PropId aToB, PropId bToA, int line) {
  const Prop& f = w_.prop(aToB);
  const Prop& g = w_.prop(bToA);
  if (f.kind != PropKind::Implies || g.kind != PropKind::Implies)
    return fail("iff cần hai mệnh đề dạng `if … then …`");
  if (f.left != g.right || f.right != g.left)
    return fail("hai chiều không khớp nhau");
  if (!w_.holds(aToB)) return missing(aToB);
  if (!w_.holds(bToA)) return missing(bToA);

  PropId out = w_.iff(f.left, f.right);
  Reason why = Reason::derive("iff", line);
  why.antecedents = {Antecedent{aToB}, Antecedent{bToA}};
  w_.tell(out, std::move(why));

  Result r;
  r.ok = true;
  r.prop = out;
  return r;
}

Prover::Result Prover::introExists(PropId instance, ObjectId witness,
                                   const std::string& name, int line) {
  if (!w_.holds(instance)) return missing(instance);

  // Abstraction: every mention of the witness becomes the bound variable of a new binder.
  std::function<Term(const Term&, VarId)> absTerm = [&](const Term& t, VarId depth) -> Term {
    switch (t.kind) {
      case TermKind::Obj:
        return t.obj == witness ? Term::ofVar(depth) : t;
      case TermKind::Var:
        return Term::ofVar(t.var + 1);  // make room for the new outer binder
      case TermKind::Tuple: {
        std::vector<Term> out;
        for (const Term& e : t.elems) out.push_back(absTerm(e, depth));
        return Term::ofTuple(std::move(out));
      }
    }
    return t;
  };
  std::function<PropId(PropId, VarId)> go = [&](PropId cur, VarId depth) -> PropId {
    const Prop& p = w_.prop(cur);
    switch (p.kind) {
      case PropKind::Atom:
        return w_.atom(absTerm(p.subject, depth), absTerm(p.column, depth), p.positive);
      case PropKind::And:     return w_.conj(go(p.left, depth), go(p.right, depth));
      case PropKind::Or:      return w_.disj(go(p.left, depth), go(p.right, depth));
      case PropKind::Implies: return w_.implies(go(p.left, depth), go(p.right, depth));
      case PropKind::Iff:     return w_.iff(go(p.left, depth), go(p.right, depth));
      case PropKind::Not:     return w_.neg(go(p.left, depth));
      case PropKind::ForAll:  return w_.forAll(p.binderName, go(p.left, depth + 1));
      case PropKind::Exists:  return w_.exists(p.binderName, go(p.left, depth + 1));
    }
    return cur;
  };

  PropId out = w_.exists(name, go(instance, 0));
  Reason why = Reason::derive("exists", line);
  why.antecedents = {Antecedent{instance}};
  w_.tell(out, std::move(why));

  Result r;
  r.ok = true;
  r.prop = out;
  return r;
}

// -------------------------------------------------------------------- elimination

Prover::Result Prover::elimAnd(PropId conjunction, bool takeLeft, int line) {
  const Prop& p = w_.prop(conjunction);
  if (p.kind != PropKind::And) return fail("không phải mệnh đề `and`");
  if (!w_.holds(conjunction)) return missing(conjunction);

  PropId out = takeLeft ? p.left : p.right;
  Reason why = Reason::derive("and", line);
  why.antecedents = {Antecedent{conjunction}};
  w_.tell(out, std::move(why));

  Result r;
  r.ok = true;
  r.prop = out;
  return r;
}

Prover::Result Prover::elimIff(PropId equivalence, bool forward, int line) {
  const Prop& p = w_.prop(equivalence);
  if (p.kind != PropKind::Iff) return fail("không phải mệnh đề `iff`");
  if (!w_.holds(equivalence)) return missing(equivalence);

  PropId out = forward ? w_.implies(p.left, p.right) : w_.implies(p.right, p.left);
  Reason why = Reason::derive("iff", line);
  why.antecedents = {Antecedent{equivalence}};
  w_.tell(out, std::move(why));

  Result r;
  r.ok = true;
  r.prop = out;
  return r;
}

Prover::Result Prover::absurd(PropId p, PropId notP, int line) {
  if (!w_.holds(p)) return missing(p);
  if (!w_.holds(notP)) return missing(notP);

  // Two ways to point out an absurdity: `not (A)` wrapping A itself, or one cell with both
  // flags.
  const Prop& n = w_.prop(notP);
  bool wrapsIt = n.kind == PropKind::Not && n.left == p;
  bool oppositeFlags = false;
  const Prop& a = w_.prop(p);
  if (a.kind == PropKind::Atom && n.kind == PropKind::Atom)
    oppositeFlags = a.positive != n.positive && a.subject == n.subject && a.column == n.column;
  if (!wrapsIt && !oppositeFlags)
    return fail("hai mệnh đề này không phủ định nhau");

  if (scopes_.empty()) return fail("chỉ ra vô lý ngoài mọi scope thì không dựng được gì");
  scopes_.back().absurd = true;
  (void)line;

  Result r;
  r.ok = true;
  return r;
}

Prover::Result Prover::absurdFromOr(PropId disjunction, PropId notA, PropId notB, int line) {
  const Prop& d = w_.prop(disjunction);
  if (d.kind != PropKind::Or) return fail("không phải mệnh đề `or`");
  if (!w_.holds(disjunction)) return missing(disjunction);

  auto refutes = [&](PropId no, PropId side) {
    const Prop& n = w_.prop(no);
    if (n.kind == PropKind::Not && n.left == side) return true;
    const Prop& s2 = w_.prop(side);
    return n.kind == PropKind::Atom && s2.kind == PropKind::Atom &&
           n.positive != s2.positive && n.subject == s2.subject && n.column == s2.column;
  };
  if (!refutes(notA, d.left)) return fail("mệnh đề thứ hai không phủ định vế trái");
  if (!refutes(notB, d.right)) return fail("mệnh đề thứ ba không phủ định vế phải");
  if (!w_.holds(notA)) return missing(notA);
  if (!w_.holds(notB)) return missing(notB);

  if (scopes_.empty()) return fail("chỉ ra vô lý ngoài mọi scope thì không dựng được gì");
  scopes_.back().absurd = true;
  (void)line;

  Result r;
  r.ok = true;
  return r;
}

// ------------------------------------------------------------------- scope

Prover::Result Prover::suppose(PropId assumption, const std::string& label, int line) {
  Scope s;
  s.kind = ScopeKind::Suppose;
  s.mark = w_.mark();
  s.assumption = assumption;
  s.label = label;
  s.line = line;
  scopes_.push_back(s);
  w_.tell(assumption, Reason::stipulate(line));

  Result r;
  r.ok = true;
  r.prop = assumption;
  return r;
}

Prover::Result Prover::take(const std::string& name, int line) {
  Scope s;
  s.kind = ScopeKind::Take;
  s.mark = w_.mark();
  s.name = name;
  s.line = line;
  s.fresh = w_.declare(name);  // nobody has said anything about it
  scopes_.push_back(s);

  Result r;
  r.ok = true;
  return r;
}

Prover::Result Prover::takeIn(const std::string& name, ObjectId set, int line) {
  Scope s;
  s.kind = ScopeKind::TakeIn;
  s.mark = w_.mark();
  s.name = name;
  s.line = line;
  s.fresh = w_.declare(name);
  s.membership = w_.atom(Term::of(s.fresh), Term::of(set), true);
  scopes_.push_back(s);
  w_.tell(s.membership, Reason::stipulate(line));

  Result r;
  r.ok = true;
  return r;
}

Prover::Result Prover::takeFrom(const std::string& name, PropId existential, int line) {
  const Prop& e = w_.prop(existential);
  if (e.kind != PropKind::Exists) return fail("không phải mệnh đề `there exists`");
  if (!w_.holds(existential)) return missing(existential);

  Scope s;
  s.kind = ScopeKind::TakeFrom;
  s.mark = w_.mark();
  s.name = name;
  s.line = line;
  s.fresh = w_.declare(name);
  s.assumption = existential;
  scopes_.push_back(s);

  // Witnesses are always fresh: each time a different object is taken out.
  PropId about = w_.instantiate(existential, s.fresh);
  Reason why = Reason::derive("take", line);
  why.antecedents = {Antecedent{existential}};
  w_.tell(about, std::move(why));

  Result r;
  r.ok = true;
  r.prop = about;
  return r;
}

Prover::Result Prover::hence(PropId stated, const std::string& label, int line) {
  if (scopes_.empty()) return fail("không có scope nào đang mở");
  Scope s = scopes_.back();
  const Prop& out = w_.prop(stated);

  switch (s.kind) {
    case ScopeKind::Suppose: {
      if (s.absurd) {
        if (out.kind != PropKind::Not || out.left != s.assumption)
          return fail("scope này đã chỉ ra vô lý, nên chỉ thoát ra được `not (giả định)`");
      } else {
        if (out.kind != PropKind::Implies || out.left != s.assumption)
          return fail("mệnh đề thoát phải là `if <giả định> then …`");
        if (!w_.holds(out.right)) return missing(out.right);
      }
      break;
    }
    case ScopeKind::Take: {
      if (out.kind != PropKind::ForAll) return fail("mệnh đề thoát phải là `for every …`");
      PropId inner = w_.instantiate(stated, s.fresh);
      if (!w_.holds(inner)) return missing(inner);
      break;
    }
    case ScopeKind::TakeIn: {
      if (out.kind != PropKind::ForAll) return fail("mệnh đề thoát phải là `for every …`");
      PropId inner = w_.instantiate(stated, s.fresh);
      const Prop& i = w_.prop(inner);
      if (i.kind != PropKind::Implies || i.left != s.membership)
        return fail("mệnh đề thoát phải là `for every x, if x ∈ S then …`");
      if (!w_.holds(i.right)) return missing(i.right);
      break;
    }
    case ScopeKind::TakeFrom: {
      if (!w_.holds(stated)) return missing(stated);
      std::vector<ObjectId> mentioned;
      w_.objectsIn(stated, mentioned);
      if (std::find(mentioned.begin(), mentioned.end(), s.fresh) != mentioned.end())
        return fail("kết luận nhắc tới nhân chứng `" + s.name + "`, mà nó biến mất khi thoát");
      break;
    }
  }

  // Build the proposition first, roll back to the mark, then store it — so it survives
  // outside the removed region.
  World::PropTree keep = w_.snapshot(stated);
  scopes_.pop_back();
  w_.rollback(s.mark);
  PropId result = w_.rebuild(keep);

  Reason why = Reason::derive(label.empty() ? "scope" : label, line);
  // The scope body has been removed; the reason points at the exit step itself.
  why.antecedents = {};
  w_.tell(result, std::move(why));

  Result r;
  r.ok = true;
  r.prop = result;
  return r;
}

}  // namespace v
