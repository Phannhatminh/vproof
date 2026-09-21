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
  // --- Luật một biến, một tiền đề ---
  {
    World w;
    ObjectId alice = w.declare("alice"), A = w.declare("A"), B = w.declare("B");
    // Rule (r): for every x, if x ∈ A then x ∈ B.
    PropId r = w.forAll("x", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                       w.atom(Term::ofVar(0), Term::of(B), true)));
    w.tell(r, Reason::stipulate(1));

    auto miss = w.applyRule(r, {alice}, "r", 3);
    check(!miss.ok, "tiền đề chưa có thì bước không đi được");
    check(w.showProp(miss.missing) == "alice ∈ A", "và nó chỉ đúng chỗ thiếu");
    check(!w.toldIn(alice, B), "bước không đi được thì không ghi gì");

    w.tell(w.atom(Term::of(alice), Term::of(A), true), Reason::stipulate(2));
    auto ok = w.applyRule(r, {alice}, "r", 3);
    check(ok.ok && w.toldIn(alice, B), "đủ tiền đề thì kết luận được ghi");
    check(w.showProp(ok.conclusion) == "alice ∈ B", "kết luận đúng hình dạng");
  }

  // --- Lý do mang luật, binding, và tiền đề ---
  {
    World w;
    ObjectId alice = w.declare("alice"), A = w.declare("A"), B = w.declare("B");
    PropId r = w.forAll("x", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                       w.atom(Term::ofVar(0), Term::of(B), true)));
    w.tell(w.atom(Term::of(alice), Term::of(A), true), Reason::stipulate(2));
    w.applyRule(r, {alice}, "r", 7);

    const auto& rs = w.reasons(alice, B, true);
    check(rs.size() == 1 && !rs[0].stipulated, "lý do là suy ra, không phải đặt ra");
    check(rs[0].rule == "r" && rs[0].line == 7, "giữ nhãn luật và dòng");
    check(rs[0].binding.size() == 1 && rs[0].binding[0].first == "x" &&
              rs[0].binding[0].second == alice,
          "giữ binding x := alice");
    check(rs[0].antecedents.size() == 1 &&
              w.showProp(rs[0].antecedents[0].prop) == "alice ∈ A",
          "giữ tiền đề đã tra");
  }

  // --- Nhiều biến, tuple, bắc cầu ---
  {
    World w;
    ObjectId a = w.declare("a"), b = w.declare("b"), c = w.declare("c");
    ObjectId Boss = w.declare("Boss");
    auto pair = [&](VarId i, VarId j) {
      return Term::ofTuple({Term::ofVar(i), Term::ofVar(j)});
    };
    // for every x, y, z, if (x,y) ∈ Boss and (y,z) ∈ Boss then (x,z) ∈ Boss.
    PropId body = w.implies(
        w.conj(w.atom(pair(2, 1), Term::of(Boss), true),
               w.atom(pair(1, 0), Term::of(Boss), true)),
        w.atom(pair(2, 0), Term::of(Boss), true));
    PropId r = w.forAll("x", w.forAll("y", w.forAll("z", body)));

    w.tell(w.atom(Term::ofTuple({Term::of(a), Term::of(b)}), Term::of(Boss), true),
           Reason::stipulate(1));
    w.tell(w.atom(Term::ofTuple({Term::of(b), Term::of(c)}), Term::of(Boss), true),
           Reason::stipulate(2));

    auto res = w.applyRule(r, {a, b, c}, "join", 5);
    check(res.ok, "luật ba biến với tiền đề ghép chạy");
    check(w.showProp(res.conclusion) == "(a, c) ∈ Boss", "kết luận: " + w.showProp(res.conclusion));
    check(w.reasons(w.resolve(Term::ofTuple({Term::of(a), Term::of(c)})), Boss, true)[0]
                  .antecedents.size() == 2,
          "tiền đề ghép bằng and tách ra thành hai tiền đề");
  }

  // --- Kết luận âm ---
  {
    World w;
    ObjectId kim = w.declare("kim"), Minors = w.declare("Minors"), Voters = w.declare("Voters");
    PropId r = w.forAll("x", w.implies(w.atom(Term::ofVar(0), Term::of(Minors), true),
                                       w.atom(Term::ofVar(0), Term::of(Voters), false)));
    w.tell(w.atom(Term::of(kim), Term::of(Minors), true), Reason::stipulate(1));
    auto res = w.applyRule(r, {kim}, "no vote", 4);
    check(res.ok && w.toldOut(kim, Voters), "kết luận ∉ bật cờ âm");
    check(!w.toldIn(kim, Voters), "và không đụng cờ dương");
  }

  // --- Luật không có tiền đề ---
  {
    World w;
    ObjectId n = w.declare("n"), N = w.declare("N");
    PropId r = w.forAll("x", w.atom(Term::ofVar(0), Term::of(N), true));
    auto res = w.applyRule(r, {n}, "everything", 2);
    check(res.ok && w.toldIn(n, N), "luật không tiền đề áp được ngay");
  }

  // --- Kết luận là mệnh đề ghép thì vào kho ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S"), P = w.declare("P"), Q = w.declare("Q");
    PropId r = w.forAll("x", w.implies(
        w.atom(Term::ofVar(0), Term::of(S), true),
        w.conj(w.atom(Term::ofVar(0), Term::of(P), true),
               w.atom(Term::ofVar(0), Term::of(Q), true))));
    w.tell(w.atom(Term::of(x), Term::of(S), true), Reason::stipulate(1));
    auto res = w.applyRule(r, {x}, "staff", 3);
    check(res.ok && w.holds(res.conclusion), "kết luận ghép vào kho");
    check(!w.toldIn(x, P) && !w.toldIn(x, Q), "và không tự tách ra thành hai ô");
  }

  // --- Bước không đi được thì thế giới không đổi ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId r = w.forAll("x", w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                                       w.atom(Term::ofVar(0), Term::of(B), true)));
    size_t cells = w.cellCount();
    auto res = w.applyRule(r, {x}, "r", 9);
    check(!res.ok && w.cellCount() == cells, "không ô nào mọc thêm khi bước hỏng");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
