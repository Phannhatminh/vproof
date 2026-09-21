#pragma once
#include <string>
#include <vector>

#include "v/core.hpp"

namespace v {

using PropId = uint32_t;
constexpr PropId kNoProp = UINT32_MAX;

using VarId = uint32_t;  // chỉ số de Bruijn: đếm ngược ra ngoài qua các binder

// Lỗ của một mẫu Notation, đánh số từ kHole. Để riêng khỏi chỉ số de Bruijn
// nên một mẫu không bao giờ bắt nhầm biến buộc ở chỗ dùng.
constexpr VarId kHole = 1u << 20;

// Term đứng ở hai vị trí của một mệnh đề nguyên tử. Lá là handle, không phải
// tên bề mặt — đó là chỗ bản cũ hỏng, và là điều kiện để runtime đứng một mình.
enum class TermKind { Obj, Var, Tuple };

struct Term {
  TermKind kind = TermKind::Obj;
  ObjectId obj = kNoObject;
  VarId var = 0;
  std::vector<Term> elems;  // Tuple

  static Term of(ObjectId id) {
    Term t;
    t.kind = TermKind::Obj;
    t.obj = id;
    return t;
  }
  static Term ofVar(VarId v) {
    Term t;
    t.kind = TermKind::Var;
    t.var = v;
    return t;
  }
  static Term ofTuple(std::vector<Term> elems) {
    Term t;
    t.kind = TermKind::Tuple;
    t.elems = std::move(elems);
    return t;
  }

  bool operator==(const Term& o) const;
  bool operator<(const Term& o) const;
};

enum class PropKind { Atom, And, Or, Implies, Iff, Not, ForAll, Exists };

// Mệnh đề. Nguyên tử mang hai term và một cực — `t ∉ S` là Atom cực âm, còn
// `not (t ∈ S)` là Not bọc một Atom cực dương: hai mệnh đề khác nhau.
// ForAll/Exists buộc đúng một biến; `for every x, y, A` là hai binder lồng.
struct Prop {
  PropKind kind = PropKind::Atom;

  Term subject, column;   // Atom
  bool positive = true;   // Atom

  PropId left = kNoProp;  // And/Or/Implies/Iff/Not, và body của binder
  PropId right = kNoProp; // And/Or/Implies/Iff

  std::string binderName;  // chỉ để in ra

  bool sameShape(const Prop& o) const;
  bool shapeLess(const Prop& o) const;
};

}  // namespace v
