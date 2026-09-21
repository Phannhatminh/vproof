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
    check(a1 != a2, "declaring the same name twice gives two objects");
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
    check(pair1 == pair2, "same components, same tuple");
    check(t1 != t2 && t1 != pair1 && t2 != pair1, "tuple1, tuple2, (alice,carol) are 3 objects");
    check(w.objectCount() == 5, "5 objects in all: alice, carol, tuple1, tuple2, the pair");
  }

  // --- Numerals: identity by value, decimals read exactly ---
  {
    World w;
    check(w.numeral(Rational("0.1")) == w.numeral(Rational(1, 10)), "0.1 is 1/10");
    check(w.numeral(Rational("2")) == w.numeral(Rational(4, 2)), "4/2 is 2");
    check(w.obj(w.numeral(Rational(-6, 4))).name == "-3/2", "numerals are always reduced");
    ObjectId big = w.numeral(Rational("123456789012345678901234567890"));
    check(w.obj(big).name == "123456789012345678901234567890", "numerator and denominator are unbounded");
  }

  // --- Matrix: an empty cell means nobody has spoken; two independent flags; writing
  // always succeeds ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S");
    check(!w.toldIn(x, S) && !w.toldOut(x, S), "empty cell: nobody has spoken");

    w.tellIn(x, S, Reason::stipulate(1));
    check(w.toldIn(x, S) && !w.toldOut(x, S), "saying ∈ raises only the ∈ flag");

    w.tellOut(x, S, Reason::stipulate(2));
    check(w.toldIn(x, S) && w.toldOut(x, S), "both flags can be raised: no explosion, no refusal");

    check(w.reasons(x, S, true).size() == 1 && w.reasons(x, S, false).size() == 1,
          "each polarity keeps its own reasons");
  }

  // --- All reasons are kept, and the step stamp does not change on re-raise ---
  {
    World w;
    ObjectId x = w.declare("x"), S = w.declare("S");
    w.tellIn(x, S, Reason::stipulate(1));
    Step first = w.stepOf(x, S, true);
    w.tellIn(x, S, Reason::derive("r", 7));
    check(w.reasons(x, S, true).size() == 2, "deriving again adds a reason, replaces nothing");
    check(w.stepOf(x, S, true) == first, "an already-raised flag keeps its step stamp");
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
    check(w.toldIn(x, T) && w.objectCount() == 5, "inside the scope: extra objects and cells");

    w.rollback(m);
    check(!w.toldIn(x, T), "rollback: flags raised in the scope are lowered");
    check(w.objectCount() == 4, "rollback: objects created in the scope are gone");
    check(w.toldIn(x, S), "flags raised before the scope stay up");
    check(w.reasons(x, S, true).size() == 1, "only the reasons added in the scope are removed");
  }

  // --- Rolling back also removes tuple and numeral interning ---
  {
    World w;
    ObjectId a = w.declare("a"), b = w.declare("b");
    Mark m = w.mark();
    ObjectId p = w.tuple({a, b});
    (void)p;
    w.numeral(Rational(5));
    check(w.objectCount() == 4, "the scope has a tuple and a numeral");
    w.rollback(m);
    check(w.objectCount() == 2, "rollback removes both");
    ObjectId again = w.tuple({a, b});
    check(w.objectCount() == 3 && w.obj(again).kind == Kind::Tuple,
          "rebuilding the tuple after rollback gives a new object; the intern table is clean");
  }

  // --- Division by zero is a resource error of the number layer, not a fact ---
  {
    bool threw = false;
    try {
      (void)(Rational(1) / Rational(0));
    } catch (const RationalDivByZero&) {
      threw = true;
    }
    check(threw, "division by zero is reported upward, no value is made up");
  }

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
