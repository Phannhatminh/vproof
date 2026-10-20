#pragma once
#include <string>
#include <vector>

#include "v/core.hpp"

namespace v {

using PropId = uint32_t;
constexpr PropId kNoProp = UINT32_MAX;

// A term in either position of an atomic proposition. Leaves are handles, not surface names
// — that is where the old version broke, and it is what lets the runtime stand on its own.
//
// A variable is an object like any other (see Object::variable), so it sits in a term as
// Obj. So does a hole of a Notation template: it is a variable of the template.
enum class TermKind { Obj, Tuple };

struct Term {
  TermKind kind = TermKind::Obj;
  ObjectId obj = kNoObject;
  std::vector<Term> elems;  // Tuple

  static Term of(ObjectId id) {
    Term t;
    t.kind = TermKind::Obj;
    t.obj = id;
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

// A proposition. An atom carries two terms and a polarity — `t ∉ S` is a negative Atom,
// while `not (t ∈ S)` is a Not wrapping a positive Atom: two different propositions.
// ForAll/Exists bind exactly one variable; `for every x, y, A` is two nested binders.
struct Prop {
  PropKind kind = PropKind::Atom;

  Term subject, column;   // Atom
  bool positive = true;   // Atom

  PropId left = kNoProp;  // And/Or/Implies/Iff/Not, and the body of a binder
  PropId right = kNoProp; // And/Or/Implies/Iff

  ObjectId bound = kNoObject;  // ForAll/Exists: the variable object it binds

  // Variables occurring in it that no binder inside it binds, sorted. Empty means the
  // proposition is closed.
  std::vector<ObjectId> freeVars;
};

}  // namespace v
