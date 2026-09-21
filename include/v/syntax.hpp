#pragma once
#include <string>
#include <vector>

#include "v/prop.hpp"

#include <utility>

namespace v {

// A statement after parsing. Propositions here are already PropIds — the parser builds
// straight into the runtime, so no separate syntax tree lives in the language layer.
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
  Compute,    // Compute <expression>.
  Simplify,   // Simplify <expression>.
  Expand,     // Expand (a, b, c).
  Instantiate,// Instantiate (h) at t.
  Apply,      // Apply F to a.   /   Apply F to a as name.
  Theory,     // Theory M { ... }
  Import,     // Import M as Alias with (X := Y, ...).
};

struct Stmt {
  StmtKind kind = StmtKind::Assume;
  int line = 0;
  std::string text;  // verbatim, for error messages

  std::vector<std::string> names;  // Let: declared names; Take: name of the fresh object
  std::string label;               // (h) names the proposition
  PropId prop = kNoProp;
  std::vector<std::string> refs;   // labels referred to
  std::vector<Term> args;          // By rule applied to (...)
  std::string setName;             // Take x with x ∈ S
  std::vector<std::string> tupleElems;  // Let t = (a, b)
  bool isTupleLet = false;
  Term expr;       // Compute
  std::string letKind;  // Let: set / relation / map / entity
  Term fn, arg;         // Apply
  std::string cmpOp;    // Compute a <= b
  Term rhs;

  // Theory: the body is kept as tokens, unparsed — names inside only mean something once
  // Import binds them.
  // (text, line); the word/number flags are kept separately
  std::vector<std::pair<std::string, int>> body;
  std::vector<bool> bodyWord, bodyNumber;
  std::string alias;
  std::vector<std::pair<std::string, std::string>> mapping;
};

}  // namespace v
