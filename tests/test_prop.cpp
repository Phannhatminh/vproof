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
  // --- Intern theo cấu trúc ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S");
    PropId a1 = w.atom(Term::of(x), Term::of(S), true);
    PropId a2 = w.atom(Term::of(x), Term::of(S), true);
    check(a1 == a2, "dựng cùng một mệnh đề hai lần ra cùng PropId");

    PropId neg = w.atom(Term::of(x), Term::of(S), false);
    check(neg != a1, "t ∈ S và t ∉ S là hai mệnh đề khác nhau");
    check(w.neg(a1) != neg, "not (t ∈ S) khác t ∉ S");
  }

  // --- Alpha-đổi-tên miễn phí, vì biến buộc lưu theo chỉ số ---
  {
    World w;
    ObjectId A = w.declare("A"), B = w.declare("B");
    PropId body1 = w.implies(w.atom(Term::ofVar(0), Term::of(A), true),
                             w.atom(Term::ofVar(0), Term::of(B), true));
    PropId r1 = w.forAll("x", body1);
    PropId r2 = w.forAll("y", body1);
    check(r1 == r2, "chỉ khác tên biến buộc thì là cùng một mệnh đề");
    check(w.showProp(r1) == "for every x, (if x ∈ A then x ∈ B)",
          "in ra dùng tên của binder được intern trước: " + w.showProp(r1));
  }

  // --- A or B khác B or A ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    check(w.disj(a, b) != w.disj(b, a), "A or B và B or A là hai mệnh đề");
  }

  // --- Nguyên tử ground vào ma trận, ghép vào kho ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    PropId b = w.atom(Term::of(x), Term::of(B), true);

    check(!w.holds(a), "chưa nói thì chưa có");
    w.tell(a, Reason::stipulate(1));
    check(w.holds(a) && w.toldIn(x, A), "nguyên tử ground đi thẳng vào ma trận");
    // 2 ô: (x, A) và (A, Column) — cơ chế ghi lại A đã được dùng làm cột.
    check(w.cellCount() == 2, "và nó là một ô, không phải một mục trong kho");

    PropId both = w.conj(a, b);
    w.tell(both, Reason::stipulate(2));
    check(w.holds(both), "mệnh đề ghép vào kho");
    check(!w.holds(b), "cất A and B không tự làm cho B có — phải qua một bước");
    check(w.cellCount() == 2, "và nó không đẻ ra ô nào");
  }

  // --- Tuple term dựng ra đối tượng tuple khi cất ---
  {
    World w;
    ObjectId a = w.declare("a"), b = w.declare("b"), R = w.declare("R");
    size_t before = w.objectCount();
    PropId p = w.atom(Term::ofTuple({Term::of(a), Term::of(b)}), Term::of(R), true);
    check(w.objectCount() == before + 1, "(a, b) ∈ R dựng ra đúng một đối tượng tuple");
    w.tell(p, Reason::stipulate(1));
    check(w.holds(p), "và ô của nó bật");
  }

  // --- Lý do đi theo mệnh đề ghép ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId p = w.disj(w.atom(Term::of(x), Term::of(A), true),
                      w.atom(Term::of(x), Term::of(B), true));
    w.tell(p, Reason::stipulate(3));
    w.tell(p, Reason::derive("r", 9));
    check(w.propReasons(p).size() == 2, "kho cũng giữ hết lý do");
    check(!w.toldIn(x, A) && !w.toldIn(x, B), "A or B không bật cờ ở ô nào");
  }

  // --- Quay về mốc gỡ cả mệnh đề lẫn mục trong kho ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A"), B = w.declare("B");
    PropId a = w.atom(Term::of(x), Term::of(A), true);
    w.tell(a, Reason::stipulate(1));

    Mark m = w.mark();
    PropId b = w.atom(Term::of(x), Term::of(B), true);
    PropId both = w.conj(a, b);
    w.tell(both, Reason::stipulate(2));
    check(w.holds(both), "trong scope: kho có A and B");

    w.rollback(m);
    check(!w.holds(both), "quay lại: mục trong kho bị gỡ");
    check(w.propCount() == 1, "quay lại: mệnh đề dựng trong scope cũng biến mất");
    check(w.holds(a), "mệnh đề có từ trước scope thì còn");
  }

  // --- Dựng lại sau khi quay lại thì intern vẫn đúng ---
  {
    World w;
    ObjectId x = w.declare("x"), A = w.declare("A");
    Mark m = w.mark();
    PropId p1 = w.atom(Term::of(x), Term::of(A), true);
    w.rollback(m);
    PropId p2 = w.atom(Term::of(x), Term::of(A), true);
    check(p1 == p2 && w.propCount() == 1, "bảng intern mệnh đề sạch sau khi quay lại");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
