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
    check(w.representedBy(0) == kNoProp, "chưa gọi tên thì chưa có đại diện");

    ObjectId ra = w.represent(a);
    check(w.objectCount() > before, "gọi tên thì đại diện mới sinh ra");
    check(w.obj(ra).kind == Kind::Tuple && w.obj(ra).elems.size() == 3,
          "đại diện nguyên tử là tuple (mem, chủ thể, cột)");
    check(w.representedBy(ra) == a, "đi ngược được từ đại diện về mệnh đề");

    ObjectId rBoth = w.represent(w.conj(a, b));
    check(w.obj(rBoth).elems[1] == ra, "đại diện ghép dựng từ đại diện con");
    check(w.represent(a) == ra, "gọi lại thì ra đúng đại diện cũ");
  }

  // --- Holds stays in sync in both directions ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    ObjectId ra = w.represent(a);
    ObjectId H = w.holdsColumn();

    check(!w.toldIn(ra, H), "chưa cất thì đại diện chưa thuộc Holds");
    p.assume(a, 1);
    check(w.toldIn(ra, H), "cất mệnh đề thì đại diện vào Holds");

    // the other direction
    PropId b = w.atom(Term::of(x), Term::of(w.declare("B")), true);
    ObjectId rb = w.represent(b);
    check(!w.holds(b), "B chưa được cất");
    w.tellIn(rb, H, Reason::stipulate(2));
    check(w.holds(b), "ghi vào Holds thì mệnh đề được cất theo");
  }

  // --- A representative created after its proposition catches up immediately ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    p.assume(a, 1);
    ObjectId ra = w.represent(a);
    check(w.toldIn(ra, w.holdsColumn()), "đại diện sinh sau vẫn vào Holds ngay");
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
    check(w.toldIn(ra, H), "trong scope: đại diện của giả định thuộc Holds");
    p.assume(b, 2);
    p.hence(w.implies(a, b), "r", 3);
    check(!w.toldIn(ra, H), "ra scope: rụng theo, vì Holds là một ô");
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
    check(w.toldIn(w.represent(nns), H), "not (not S) vào Holds");
    check(!w.holds(s), "nhưng S thì chưa");

    auto res = w.applyRule(dne, {rs}, "dne", 3);
    check(res.ok, "áp luật phủ định kép trên đại diện");
    check(w.toldIn(rs, H), "kết luận: đại diện của S thuộc Holds");
    check(w.holds(s), "và S được cất theo — đồng bộ ngược chạy");
  }

  // --- Quantified propositions: representatives with identity preserved ---
  {
    World w; Prover p(w);
    ObjectId A = w.declare("A");
    PropId all = w.forAll("x", w.atom(Term::ofVar(0), Term::of(A), true));
    ObjectId r1 = w.represent(all);
    check(r1 != kNoObject, "mệnh đề có lượng từ có đại diện");
    check(w.representedBy(r1) == all, "đi ngược được");

    PropId same = w.forAll("y", w.atom(Term::ofVar(0), Term::of(A), true));
    check(same == all && w.represent(same) == r1,
          "chỉ khác tên biến buộc thì cùng một đại diện");

    PropId other = w.exists("x", w.atom(Term::ofVar(0), Term::of(A), true));
    check(w.represent(other) != r1, "for every và there exists là hai đại diện khác nhau");

    // Structured: (All, (Mem, (Var, 0), A))
    const Object& o = w.obj(r1);
    check(o.kind == Kind::Tuple && o.elems.size() == 2, "đại diện có lượng từ là tuple hai phần");
    check(w.show(o.elems[0]) == "All", "phần đầu là nhãn All");
    check(w.show(o.elems[1]) == "(Mem, (Var, 0), A)",
          "thân đại diện được luôn, biến buộc thành (Var, 0): " + w.show(o.elems[1]));
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
          "not (not (for every x, ...)) qua (dne) ra được mệnh đề có lượng từ");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
