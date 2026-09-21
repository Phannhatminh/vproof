#include "v/prop.hpp"

namespace v {

bool Term::operator==(const Term& o) const {
  if (kind != o.kind) return false;
  switch (kind) {
    case TermKind::Obj: return obj == o.obj;
    case TermKind::Var: return var == o.var;
    case TermKind::Tuple: return elems == o.elems;
  }
  return false;
}

bool Term::operator<(const Term& o) const {
  if (kind != o.kind) return kind < o.kind;
  switch (kind) {
    case TermKind::Obj: return obj < o.obj;
    case TermKind::Var: return var < o.var;
    case TermKind::Tuple: return elems < o.elems;
  }
  return false;
}

// Danh tính của mệnh đề theo cấu trúc, và binderName không tính vào — nó chỉ
// để in ra. Vì biến buộc lưu theo chỉ số de Bruijn, hai mệnh đề chỉ khác tên
// biến buộc là cùng một mệnh đề, không cần so sánh riêng.
bool Prop::sameShape(const Prop& o) const {
  if (kind != o.kind) return false;
  if (kind == PropKind::Atom)
    return positive == o.positive && subject == o.subject && column == o.column;
  return left == o.left && right == o.right;
}

bool Prop::shapeLess(const Prop& o) const {
  if (kind != o.kind) return kind < o.kind;
  if (kind == PropKind::Atom) {
    if (positive != o.positive) return positive < o.positive;
    if (!(subject == o.subject)) return subject < o.subject;
    return column < o.column;
  }
  if (left != o.left) return left < o.left;
  return right < o.right;
}

}  // namespace v
