#include <iostream>
#include <string>

#include "v/prover.hpp"

using namespace v;

static int failures = 0;
static void check(bool ok, const std::string& what) {
  std::cout << (ok ? "ok    " : "FAIL  ") << what << "\n";
  if (!ok) ++failures;
}

int main() {
  // --- Lazy pairing, and readable structure ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    size_t before = w.objectCount();
    check(w.representedBy(0) == kNoProp, "no representative until named");

    ObjectId ra = w.represent(a);
    check(w.objectCount() > before, "naming it creates the representative");
    check(w.obj(ra).kind == Kind::Tuple && w.obj(ra).elems.size() == 3,
          "the representative of an atom is the tuple (Mem, subject, column)");
    check(w.representedBy(ra) == a, "the representative maps back to the proposition");

    ObjectId rBoth = w.represent(w.conj(a, b));
    check(w.obj(rBoth).elems[1] == ra, "a compound representative is built from its parts");
    check(w.represent(a) == ra, "naming it again gives the same representative");
  }

  // --- Holds stays in sync in both directions ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    ObjectId ra = w.represent(a);
    ObjectId H = w.holdsColumn();

    check(!w.toldIn(ra, H), "not stored yet, so the representative is not in Holds");
    p.assume(a, 1);
    check(w.toldIn(ra, H), "storing the proposition puts its representative in Holds");

    // the other direction
    PropId b = w.atom(Term::of(x), Term::of(w.declare("B")), true);
    ObjectId rb = w.represent(b);
    check(!w.holds(b), "B has not been stored");
    w.tellIn(rb, H, Reason::stipulate(2));
    check(w.holds(b), "writing into Holds stores the proposition too");
  }

  // --- A representative created after its proposition catches up immediately ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    p.assume(a, 1);
    ObjectId ra = w.represent(a);
    check(w.toldIn(ra, w.holdsColumn()), "a representative created later is in Holds at once");
  }

  // --- Holds is a cell, so it follows scopes ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    ObjectId H = w.holdsColumn();
    ObjectId ra = w.represent(a);

    p.suppose(a, "h", 1);
    check(w.toldIn(ra, H), "inside the scope: the assumption's representative is in Holds");
    p.assume(b, 2);
    p.hence(w.implies(a, b), "r", 3);
    check(!w.toldIn(ra, H), "after the scope: it is gone, because Holds is a cell");
  }

  // --- A classical rule written over representatives, applied in one line ---
  {
    World w; Prover p(w);
    ObjectId t = w.declare("t"), S = w.declare("S");
    PropId s = w.atom(Term::of(t), Term::of(S), true);
    PropId nns = w.neg(w.neg(s));
    ObjectId H = w.holdsColumn();

    // Rule (dne): for every p, if (not, (not, p)) ∈ Holds then p ∈ Holds.
    ObjectId notTag = w.tag("Not");
    PropId dne = w.forAll(
        "p",
        w.implies(w.atom(Term::ofTuple({Term::of(notTag),
                                        Term::ofTuple({Term::of(notTag), Term::ofVar(0)})}),
                         Term::of(H), true),
                  w.atom(Term::ofVar(0), Term::of(H), true)));
    p.assume(dne, 1);

    p.assume(nns, 2);
    ObjectId rs = w.represent(s);
    check(w.toldIn(w.represent(nns), H), "not (not S) is in Holds");
    check(!w.holds(s), "but S is not yet");

    auto res = w.applyRule(dne, {rs}, "dne", 3);
    check(res.ok, "apply double negation over representatives");
    check(w.toldIn(rs, H), "conclusion: the representative of S is in Holds");
    check(w.holds(s), "and S is stored too — the reverse sync works");
  }

  // --- Quantified propositions: representatives with identity preserved ---
  {
    World w; Prover p(w);
    ObjectId A = w.declare("A");
    PropId all = w.forAll("x", w.atom(Term::ofVar(0), Term::of(A), true));
    ObjectId r1 = w.represent(all);
    check(r1 != kNoObject, "a quantified proposition has a representative");
    check(w.representedBy(r1) == all, "it maps back");

    PropId same = w.forAll("y", w.atom(Term::ofVar(0), Term::of(A), true));
    check(same == all && w.represent(same) == r1,
          "differing only in bound-variable names gives the same representative");

    PropId other = w.exists("x", w.atom(Term::ofVar(0), Term::of(A), true));
    check(w.represent(other) != r1, "for every and there exists are two different representatives");

    // Structured: (All, (Mem, (Var, 0), A))
    const Object& o = w.obj(r1);
    check(o.kind == Kind::Tuple && o.elems.size() == 2, "a quantified representative is a two-part tuple");
    check(w.show(o.elems[0]) == "All", "the first part is the All tag");
    check(w.show(o.elems[1]) == "(Mem, (Var, 0), A)",
          "the body is represented too, with the bound variable as (Var, 0): " + w.show(o.elems[1]));
  }

  // --- Proof by contradiction with a quantified conclusion: runs to the end ---
  {
    World w; Prover p(w);
    ObjectId A = w.declare("A"), B = w.declare("B");
    PropId goal = w.forAll("x", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                          w.atom(Term::ofVar(0), Term::of(B), true)));
    ObjectId H = w.holdsColumn(), notTag = w.tag("Not");
    PropId dne = w.forAll(
        "p",
        w.implies(w.atom(Term::ofTuple({Term::of(notTag),
                                        Term::ofTuple({Term::of(notTag), Term::ofVar(0)})}),
                         Term::of(H), true),
                  w.atom(Term::ofVar(0), Term::of(H), true)));
    p.assume(dne, 1);
    p.assume(w.neg(w.neg(goal)), 2);

    ObjectId rg = w.represent(goal);
    // Lazy pairing: `not (not goal)` has to be named as an object before its representative
    // exists, and at that moment Holds catches up because the proposition is already
    // stored.
    w.represent(w.neg(w.neg(goal)));
    auto res = w.applyRule(dne, {rg}, "dne", 3);
    check(res.ok && w.holds(goal),
          "not (not (for every x, ...)) through (dne) yields the quantified proposition");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
