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
  // --- Introducing and eliminating `and` ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);

    check(!p.introAnd(a, b, 1).ok, "thiếu một vế thì không đưa `and` vào được");
    p.assume(a, 1);
    p.assume(b, 2);
    auto r = p.introAnd(a, b, 3);
    check(r.ok && w.holds(r.prop), "có A và B thì đưa `A and B` vào được");

    World w2; Prover p2(w2);
    ObjectId y = w2.declare("y"), C = w2.declare("C"), D = w2.declare("D");
    PropId c = w2.atom(Term::of(y), Term::of(C), true);
    PropId d = w2.atom(Term::of(y), Term::of(D), true);
    p2.assume(w2.conj(c, d), 1);
    check(!w2.holds(c), "cất `C and D` không tự làm C có");
    auto e = p2.elimAnd(w2.conj(c, d), true, 2);
    check(e.ok && w2.holds(c), "một bước mới lấy C ra");
  }

  // --- `or` introduction: the other side is free ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    p.assume(a, 1);
    auto r = p.introOr(a, b, true, 2);
    check(r.ok && w.holds(r.prop), "có A thì có A or B");
    check(!w.toldIn(x, B), "và B vẫn chưa ai nói gì");
  }

  // --- `if … then` introduction by a scope ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B"), C = w.declare("C");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    PropId c = w.atom(Term::of(x), Term::of(C), true);
    // Rule: for every y, if y ∈ A then y ∈ B.
    PropId rule = w.forAll("y", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                          w.atom(Term::ofVar(0), Term::of(B), true)));
    p.assume(rule, 1);

    p.suppose(a, "h", 2);
    w.applyRule(rule, {x}, "rule", 3);
    check(w.holds(b), "trong scope: B suy ra được");
    auto bad = p.hence(w.implies(a, c), "r", 4);
    check(!bad.ok, "thoát với mệnh đề chưa suy ra được thì không đi được");
    auto good = p.hence(w.implies(a, b), "r", 4);
    check(good.ok, "thoát với `if A then B`");
    check(w.holds(good.prop), "mệnh đề thoát sống sót sau khi quay về mốc");
    check(!w.holds(a) && !w.holds(b), "giả định và mọi thứ trong scope đã bị gỡ");
  }

  // --- `for every` introduction ---
  {
    World w; Prover p(w);
    ObjectId A = w.declare("A"), B = w.declare("B");
    PropId rule = w.forAll("y", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                          w.atom(Term::ofVar(0), Term::of(B), true)));
    p.assume(rule, 1);

    p.takeIn("x", A, 2);
    ObjectId fresh = p.freshOf();
    w.applyRule(rule, {fresh}, "rule", 3);
    PropId stated = w.forAll("x", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                            w.atom(Term::ofVar(0), Term::of(B), true)));
    auto r = p.hence(stated, "subset", 4);
    check(r.ok, "thoát với `for every x in A, x ∈ B`");
    check(w.objectCount() == 2, "đối tượng tạm biến mất, còn A và B");
  }

  // --- `not` introduction: pointing out an absurdity inside a scope ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), S = w.declare("S");
    PropId in = w.atom(Term::of(x), Term::of(S), true);
    PropId out = w.atom(Term::of(x), Term::of(S), false);
    p.assume(out, 1);

    p.suppose(in, "h", 2);
    auto ab = p.absurd(in, out, 3);
    check(ab.ok, "ô có cả hai cờ thì chỉ ra vô lý được");
    auto r = p.hence(w.neg(in), "r", 4);
    check(r.ok && w.holds(r.prop), "thoát ra `not (x ∈ S)`");
    check(w.showProp(r.prop) == "not (x ∈ S)", w.showProp(r.prop));
  }

  // --- `there exists` introduction and elimination ---
  {
    World w; Prover p(w);
    ObjectId a = w.declare("a"), P = w.declare("P"), Q = w.declare("Q");
    PropId aP = w.atom(Term::of(a), Term::of(P), true);
    p.assume(aP, 1);
    auto ex = p.introExists(aP, a, "z", 2);
    check(ex.ok && w.showProp(ex.prop) == "there exists z such that z ∈ P",
          "chỉ ra nhân chứng: " + w.showProp(ex.prop));

    // Rule: for every z, if z ∈ P then z ∈ Q.
    PropId rule = w.forAll("z", w.implies(w.atom(Term::ofVar(0), Term::of(P), true),
                                          w.atom(Term::ofVar(0), Term::of(Q), true)));
    p.assume(rule, 3);
    p.takeFrom("w", ex.prop, 4);
    ObjectId witness = p.freshOf();
    w.applyRule(rule, {witness}, "rule", 5);
    PropId bad = w.atom(Term::of(witness), Term::of(Q), true);
    check(!p.hence(bad, "r", 6).ok, "kết luận nhắc tới nhân chứng thì không thoát được");
    auto exQ = p.introExists(bad, witness, "z", 6);
    check(exQ.ok, "gói lại thành `there exists z such that z ∈ Q`");
    auto r = p.hence(exQ.prop, "r", 7);
    check(r.ok && w.holds(r.prop), "thoát với kết luận không nhắc nhân chứng");
  }

  // --- Full example: P, P→Q∨R, Q→S, R→S ⊢ not (not S) ---
  {
    World w; Prover p(w);
    ObjectId t = w.declare("t"), SP = w.declare("P"), SQ = w.declare("Q"),
             SR = w.declare("R"), SS = w.declare("S");
    PropId P = w.atom(Term::of(t), Term::of(SP), true);
    PropId Q = w.atom(Term::of(t), Term::of(SQ), true);
    PropId R = w.atom(Term::of(t), Term::of(SR), true);
    PropId S = w.atom(Term::of(t), Term::of(SS), true);
    PropId notS = w.neg(S);

    p.assume(P, 1);
    PropId pqr = w.implies(P, w.disj(Q, R));
    PropId qs = w.implies(Q, S);
    PropId rs = w.implies(R, S);
    p.assume(pqr, 2);
    p.assume(qs, 3);
    p.assume(rs, 4);

    p.suppose(notS, "ns", 5);
    {
      p.suppose(Q, "q", 6);
      // use `if Q then S`: a rule with no variables, applied to no arguments
      w.applyRule(qs, {}, "qs", 7);
      p.absurd(S, notS, 8);
      auto nq = p.hence(w.neg(Q), "nq", 9);
      check(nq.ok, "nhánh Q: ra not Q");

      p.suppose(R, "r", 10);
      w.applyRule(rs, {}, "rs", 11);
      p.absurd(S, notS, 12);
      auto nr = p.hence(w.neg(R), "nr", 13);
      check(nr.ok, "nhánh R: ra not R");

      w.applyRule(pqr, {}, "pqr", 14);
      check(w.holds(w.disj(Q, R)), "từ P và P→Q∨R ra Q or R");
      auto ab = p.absurdFromOr(w.disj(Q, R), w.neg(Q), w.neg(R), 15);
      check(ab.ok, "Q or R cộng not Q cộng not R: vô lý");
    }
    auto nns = p.hence(w.neg(notS), "nns", 16);
    check(nns.ok && w.holds(nns.prop), "thoát ra not (not S)");
    check(w.showProp(nns.prop) == "not (not (t ∈ S))", w.showProp(nns.prop));
    check(!w.holds(notS) && !w.holds(Q), "mọi thứ trong scope đã bị gỡ");
    check(w.holds(P) && w.holds(pqr), "bốn giả thiết ngoài scope còn nguyên");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
