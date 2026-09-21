#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "v/rational.hpp"

namespace v {

using ObjectId = uint32_t;
constexpr ObjectId kNoObject = UINT32_MAX;

// Bước: mỗi lần ghi làm số này tăng. Cờ bật ở bước nào thì mang dấu bước đó.
using Step = uint64_t;

// Ba nguồn sinh đối tượng, khác nhau ở danh tính.
//   Declared — mỗi lần khai báo là một đối tượng mới, không thành phần.
//   Tuple    — danh tính theo danh sách thành phần.
//   Numeral  — danh tính theo giá trị.
enum class Kind { Declared, Tuple, Numeral };

struct Object {
  ObjectId id = kNoObject;
  Kind kind = Kind::Declared;
  std::string name;             // chỉ để in ra, không mang nghĩa
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
