#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "v/rational.hpp"

namespace v {

using ObjectId = uint32_t;
constexpr ObjectId kNoObject = UINT32_MAX;

// Step: every write increments it. A flag raised at a step carries that step's stamp.
using Step = uint64_t;

// Three sources of objects, differing in identity.
//   Declared — every declaration is a new object, with no components.
//   Tuple    — identity by the list of components.
//   Numeral  — identity by value.
//   Text     — identity by the characters: `"Alice"` written twice is one object.
enum class Kind { Declared, Tuple, Numeral, Text };

struct Object {
  ObjectId id = kNoObject;
  Kind kind = Kind::Declared;
  std::string name;             // for printing only, carries no meaning — except for Text,
                                // where it is the text itself
  // A variable of a rule: the object a `for every` or `there exists` binds. Applying the
  // rule puts another object in its place.
  bool variable = false;
  // A variable, or built from one: a tuple with a variable in it, or `F(x)` with x a
  // variable. Nothing is stated about an open object; it only stands in a rule.
  bool open = false;
  std::vector<ObjectId> elems;  // Tuple
  Rational value;               // Numeral
};

inline const char* kindName(Kind k) {
  switch (k) {
    case Kind::Declared: return "declared";
    case Kind::Tuple:    return "tuple";
    case Kind::Numeral:  return "numeral";
    case Kind::Text:     return "text";
  }
  return "?";
}

}  // namespace v
