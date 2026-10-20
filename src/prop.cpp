#include "v/prop.hpp"

namespace v {

bool Term::operator==(const Term& o) const {
  if (kind != o.kind) return false;
  switch (kind) {
    case TermKind::Obj: return obj == o.obj;
    case TermKind::Tuple: return elems == o.elems;
  }
  return false;
}

bool Term::operator<(const Term& o) const {
  if (kind != o.kind) return kind < o.kind;
  switch (kind) {
    case TermKind::Obj: return obj < o.obj;
    case TermKind::Tuple: return elems < o.elems;
  }
  return false;
}

}  // namespace v
