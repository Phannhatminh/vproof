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
enum class Kind { Declared, Tuple, Numeral };

struct Object {
  ObjectId id = kNoObject;
  Kind kind = Kind::Declared;
  std::string name;             // for printing only, carries no meaning
  std::vector<ObjectId> elems;  // Tuple
  Rational value;               // Numeral
};

inline const char* kindName(Kind k) {
  switch (k) {
    case Kind::Declared: return "declared";
    case Kind::Tuple:    return "tuple";
    case Kind::Numeral:  return "numeral";
  }
  return "?";
}

}  // namespace v
