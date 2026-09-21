#pragma once
#include <string>
#include <vector>

#include "v/prop.hpp"

#include <utility>

namespace v {

// Một câu lệnh sau khi parse. Mệnh đề trong đây đã là PropId — parser dựng
// thẳng vào runtime, nên không có cây cú pháp nào sống riêng ở tầng ngôn ngữ.
enum class StmtKind {
  Let,        // Let a, b be entities.   /   Let t = (a, b).
  Assume,     // Assume (h): A.
  Rule,       // Rule (r): A.
  ByRule,     // By rule (r) applied to (a, b), it follows that A.
  From,       // From (h1), (h2), it follows that A.
  Suppose,    // Suppose (h): A {
  Take,       // Take x {
  TakeIn,     // Take x with x ∈ S {
  TakeFrom,   // Take w from (h) {
  Close,      // }
  Hence,      // Hence (r): A.
  Absurd,     // Absurd from (p), (q).
  Therefore,  // Therefore A.
  Why,        // Why A.
  Compute,    // Compute <biểu thức>.
  Simplify,   // Simplify <biểu thức>.
  Expand,     // Expand (a, b, c).
  Instantiate,// Instantiate (h) at t.
  Apply,      // Apply F to a.   /   Apply F to a as name.
  Theory,     // Theory M { ... }
  Import,     // Import M as Alias with (X := Y, ...).
};

struct Stmt {
  StmtKind kind = StmtKind::Assume;
  int line = 0;
  std::string text;  // nguyên văn, để báo lỗi

  std::vector<std::string> names;  // Let: tên khai báo; Take: tên đối tượng mới
  std::string label;               // (h) đặt tên cho mệnh đề
  PropId prop = kNoProp;
  std::vector<std::string> refs;   // nhãn được trích lại
  std::vector<Term> args;          // By rule applied to (...)
  std::string setName;             // Take x with x ∈ S
  std::vector<std::string> tupleElems;  // Let t = (a, b)
  bool isTupleLet = false;
  Term expr;       // Compute
  std::string letKind;  // Let: set / relation / map / entity
  Term fn, arg;         // Apply
  std::string cmpOp;    // Compute a <= b
  Term rhs;

  // Theory: thân được giữ nguyên ở dạng token, chưa parse — vì tên bên trong
  // chỉ có nghĩa sau khi Import gán chúng vào đâu đó.
  std::vector<std::pair<std::string, int>> body;  // (text, line) — word/number đánh dấu riêng
  std::vector<bool> bodyWord, bodyNumber;
  std::string alias;
  std::vector<std::pair<std::string, std::string>> mapping;
};

}  // namespace v
