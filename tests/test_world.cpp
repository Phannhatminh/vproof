#include <cassert>
#include <iostream>
#include <string>

#include "v/world.hpp"

using namespace v;

static int failures = 0;
static void check(bool ok, const std::string& what) {
  if (!ok) {
    std::cout << "FAIL  " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok    " << what << "\n";
  }
}

int main() {
  // --- Object store: a declaration always gives a new object ---
  {
    World w;
    ObjectId a1 = w.declare("alice");
    ObjectId a2 = w.declare("alice");
    check(a1 != a2, "hai lần khai báo cùng tên ra hai đối tượng");
  }

  // --- Tuples: identity by components; Let t1 = Let t2 = (a,c) gives 3 objects ---
  {
    World w;
    ObjectId alice = w.declare("alice");
    ObjectId carol = w.declare("carol");
    ObjectId t1 = w.declare("tuple1");
    ObjectId t2 = w.declare("tuple2");
    ObjectId pair1 = w.tuple({alice, carol});
    ObjectId pair2 = w.tuple({alice, carol});
    check(pair1 == pair2, "cùng thành phần thì cùng tuple");
    check(t1 != t2 && t1 != pair1 && t2 != pair1, "tuple1, tuple2, (alice,carol) là 3 đối tượng");
    check(w.objectCount() == 5, "tổng cộng 5 đối tượng: alice, carol, tuple1, tuple2, cặp");
  }

  // --- Numerals: identity by value, decimals read exactly ---
  {
    World w;
    check(w.numeral(Rational("0.1")) == w.numeral(Rational(1, 10)), "0.1 là 1/10");
    check(w.numeral(Rational("2")) == w.numeral(Rational(4, 2)), "4/2 là 2");
    check(w.obj(w.numeral(Rational(-6, 4))).name == "-3/2", "numeral luôn tối giản");
    ObjectId big = w.numeral(Rational("123456789012345678901234567890"));
    check(w.obj(big).name == "123456789012345678901234567890", "tử mẫu không giới hạn độ lớn");
  }

  // --- Matrix: an empty cell means nobody has spoken; two independent flags; writing
  // always succeeds ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S");
    check(!w.toldIn(x, S) && !w.toldOut(x, S), "ô vắng: chưa ai nói gì");

    w.tellIn(x, S, Reason::stipulate(1));
    check(w.toldIn(x, S) && !w.toldOut(x, S), "nói ∈ thì chỉ cờ ∈ bật");

    w.tellOut(x, S, Reason::stipulate(2));
    check(w.toldIn(x, S) && w.toldOut(x, S), "cả hai cờ cùng bật được, không nổ, không từ chối");

    check(w.reasons(x, S, true).size() == 1 && w.reasons(x, S, false).size() == 1,
          "mỗi cực giữ lý do riêng");
  }

  // --- All reasons are kept, and the step stamp does not change on re-raise ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S");
    w.tellIn(x, S, Reason::stipulate(1));
    Step first = w.stepOf(x, S, true);
    w.tellIn(x, S, Reason::derive("r", 7));
    check(w.reasons(x, S, true).size() == 2, "suy ra lại thì thêm lý do, không thay thế");
    check(w.stepOf(x, S, true) == first, "cờ đã bật thì dấu bước giữ nguyên");
  }

  // --- Journal: rolling back to a mark ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S"), T = w.declare("T");
    w.tellIn(x, S, Reason::stipulate(1));

    Mark m = w.mark();
    ObjectId tmp = w.declare("w");
    (void)tmp;
    w.tellIn(x, T, Reason::stipulate(2));
    w.tellIn(x, S, Reason::derive("r", 3));  // flag already raised before the mark
    // 5 = x, S, T, the Column tag (created on the first write), and w created in the scope.
    check(w.toldIn(x, T) && w.objectCount() == 5, "trong scope: có thêm đối tượng và ô");

    w.rollback(m);
    check(!w.toldIn(x, T), "quay về mốc: cờ bật trong scope bị tắt");
    check(w.objectCount() == 4, "quay về mốc: đối tượng tạo trong scope biến mất");
    check(w.toldIn(x, S), "cờ bật từ trước scope thì không tắt");
    check(w.reasons(x, S, true).size() == 1, "chỉ lý do thêm trong scope bị gỡ");
  }

  // --- Rolling back also removes tuple and numeral interning ---
  {
    World w;
    ObjectId a = w.declare("a"), b = w.declare("b");
    Mark m = w.mark();
    ObjectId p = w.tuple({a, b});
    (void)p;
    w.numeral(Rational(5));
    check(w.objectCount() == 4, "trong scope có tuple và numeral");
    w.rollback(m);
    check(w.objectCount() == 2, "quay lại gỡ cả hai");
    ObjectId again = w.tuple({a, b});
    check(w.objectCount() == 3 && w.obj(again).kind == Kind::Tuple,
          "dựng lại tuple sau khi quay lại thì ra đối tượng mới, bảng intern đã sạch");
  }

  // --- Division by zero is a resource error of the number layer, not a fact ---
  {
    bool threw = false;
    try {
      (void)(Rational(1) / Rational(0));
    } catch (const RationalDivByZero&) {
      threw = true;
    }
    check(threw, "chia cho 0 báo lên tầng trên, không tự quy ước giá trị");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
