#include <iostream>
#include <string>

#include "v/world.hpp"

using namespace v;

static int failures = 0;
static void check(bool ok, const std::string& what) {
  std::cout << (ok ? "ok    " : "FAIL  ") << what << "\n";
  if (!ok) ++failures;
}

int main() {
  // --- Interning by structure ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S");
    PropId a1 = w.atom(Term::of(x), Term::of(S), true);
    PropId a2 = w.atom(Term::of(x), Term::of(S), true);
    check(a1 == a2, "building the same proposition twice gives the same PropId");

    PropId neg = w.atom(Term::of(x), Term::of(S), false);
    check(neg != a1, "t ∈ S and t ∉ S are two different propositions");
    check(w.neg(a1) != neg, "not (t ∈ S) differs from t ∉ S");
  }

  // --- Alpha-renaming is free, because bound variables are stored as indices ---
  {
    World w;
    ObjectId A = w.declare("A"), B = w.declare("B");
    PropId body1 = w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                             w.atom(Term::ofVar(0), Term::of(B), true));
    PropId r1 = w.forAll("x", body1);
    PropId r2 = w.forAll("y", body1);
    check(r1 == r2, "differing only in bound-variable names means the same proposition");
    check(w.showProp(r1) == "for every x, (if x ∈ A then x ∈ B)",
          "printing uses the binder name interned first: " + w.showProp(r1));
  }

  // --- A or B differs from B or A ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    check(w.disj(a, b) != w.disj(b, a), "A or B and B or A are two propositions");
  }

  // --- Ground atoms go into the matrix, compounds into the store ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);

    check(!w.holds(a), "not said means not there");
    w.tell(a, Reason::stipulate(1));
    check(w.holds(a) && w.toldIn(x, A), "a ground atom goes straight into the matrix");
    // 2 cells: (x, A) and (A, Column) — the mechanism records that A was used as a column.
    check(w.cellCount() == 2, "and it is a cell, not a store entry");

    PropId both = w.conj(a, b);
    w.tell(both, Reason::stipulate(2));
    check(w.holds(both), "a compound proposition goes into the store");
    check(!w.holds(b), "storing A and B does not make B hold — it takes a step");
    check(w.cellCount() == 2, "and it creates no cell");
  }

  // --- A tuple term builds a tuple object when stored ---
  {
    World w;
    ObjectId a = w.declare("a"), b = w.declare("b"), R = w.declare("R");
    size_t before = w.objectCount();
    PropId p = w.atom(Term::ofTuple({Term::of(a), Term::of(b)}), Term::of(R), true);
    check(w.objectCount() == before + 1, "(a, b) ∈ R builds exactly one tuple object");
    w.tell(p, Reason::stipulate(1));
    check(w.holds(p), "and its cell is raised");
  }

  // --- Reasons follow compound propositions ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId p = w.disj(w.atom(Term::of(x), Term::of(A), true),
                      w.atom(Term::of(x), Term::of(B), true));
    w.tell(p, Reason::stipulate(3));
    w.tell(p, Reason::derive("r", 9));
    check(w.propReasons(p).size() == 2, "the store keeps every reason too");
    check(!w.toldIn(x, A) && !w.toldIn(x, B), "A or B raises no flag in any cell");
  }

  // --- Rolling back removes both the propositions and the store entries ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    w.tell(a, Reason::stipulate(1));

    Mark m = w.mark();
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    PropId both = w.conj(a, b);
    w.tell(both, Reason::stipulate(2));
    check(w.holds(both), "inside the scope: the store has A and B");

    w.rollback(m);
    check(!w.holds(both), "after rollback: the store entry is removed");
    check(w.propCount() == 1, "after rollback: propositions built in the scope are gone too");
    check(w.holds(a), "propositions from before the scope remain");
  }

  // --- Rebuilding after a rollback still interns correctly ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A");
    Mark m = w.mark();
    PropId p1 = w.atom(Term::of(x), Term::of(A), true);
    w.rollback(m);
    PropId p2 = w.atom(Term::of(x), Term::of(A), true);
    check(p1 == p2 && w.propCount() == 1, "the proposition intern table is clean after rollback");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
