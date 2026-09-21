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
  // --- Ghép lười, và cấu trúc đọc được ---
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

  // --- Holds đồng bộ hai chiều ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    ObjectId ra = w.represent(a);
    ObjectId H = w.holdsColumn();

    check(!w.toldIn(ra, H), "chưa cất thì đại diện chưa thuộc Holds");
    p.assume(a, 1);
    check(w.toldIn(ra, H), "cất mệnh đề thì đại diện vào Holds");

    // chiều ngược lại
    PropId b = w.atom(Term::of(x), Term::of(w.declare("B")), true);
    ObjectId rb = w.represent(b);
    check(!w.holds(b), "B chưa được cất");
    w.tellIn(rb, H, Reason::stipulate(2));
    check(w.holds(b), "ghi vào Holds thì mệnh đề được cất theo");
  }

  // --- Đại diện sinh ra sau khi mệnh đề đã có thì bắt kịp ngay ---
  {
    World w; Prover p(w);
    ObjectId x = w.declare("x"), A = w.declare("A");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    p.assume(a, 1);
    ObjectId ra = w.represent(a);
    check(w.toldIn(ra, w.holdsColumn()), "đại diện sinh sau vẫn vào Holds ngay");
  }

  // --- Holds là một ô, nên nó theo scope ---
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

  // --- Luật cổ điển viết trên đại diện, áp một dòng ---
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

  // --- Mệnh đề có lượng từ: đại diện mờ, đủ cho luật coi nó như một hạt ---
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

    // Có cấu trúc: (All, (Mem, (Var, 0), A))
    const Object& o = w.obj(r1);
    check(o.kind == Kind::Tuple && o.elems.size() == 2, "đại diện có lượng từ là tuple hai phần");
    check(w.show(o.elems[0]) == "All", "phần đầu là nhãn All");
    check(w.show(o.elems[1]) == "(Mem, (Var, 0), A)",
          "thân đại diện được luôn, biến buộc thành (Var, 0): " + w.show(o.elems[1]));
  }

  // --- Phản chứng với kết luận có lượng từ: chạy được đến cùng ---
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
    // Ghép lười: phải gọi tên `not (not goal)` như một đối tượng thì đại diện
    // của nó mới sinh ra, và lúc đó Holds bắt kịp vì mệnh đề đã được cất.
    w.represent(w.neg(w.neg(goal)));
    auto res = w.applyRule(dne, {rg}, "dne", 3);
    check(res.ok && w.holds(goal),
          "not (not (for every x, ...)) qua (dne) ra được mệnh đề có lượng từ");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
