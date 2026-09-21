#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

#include "v/parser.hpp"
#include "v/prover.hpp"

namespace v {

// Runs a V program. This layer only connects statements to runtime operations; it has no
// logic of its own.
class Interp {
 public:
  struct Report {
    int checksRun = 0;
    int checksFailed = 0;
    std::vector<std::string> lines;  // run trace, printed for the reader
  };

  Report run(const std::string& source);

  World& world() { return w_; }

 private:
  void exec(const Stmt& s, Report& rep);
  PropId ref(const std::string& label, int line) const;
  void bind(const std::string& label, PropId p);
  void whyChain(PropId p, int depth, Report& rep) const;
  // The arithmetic door: hand the expression to the number layer and get a value back.
  // Every result comes with an error bound; exact means the error is 0.
  Rational evalExpr(const Term& t, int line);
  // Read off a witness by matching, not by trying objects one by one.
  bool matchTerm(const Term& concrete, const Term& pattern, VarId depth,
                 ObjectId& witness) const;
  bool matchWitness(PropId concrete, PropId pattern, VarId depth, ObjectId& witness) const;

  std::map<std::string, ObjectId> names_;
  AppliedMap applied_;
  std::map<std::string, PropId> labels_;
  World w_;
  Prover pv_{w_};
  Parser parser_{w_, names_, applied_, labels_};
  // Labels and names declared inside a scope live only in that scope: on exit, the
  // propositions and objects they point at have been removed from the world.
  struct Frame {
    std::vector<std::string> labels, names;
  };
  std::vector<Frame> frames_;
  bool preludeLoaded_ = false;

  // Theory bodies, kept as tokens. An instance is identified by (theory name, mapping) —
  // importing the same pair again does nothing.
  std::map<std::string, Stmt> theories_;
  std::set<std::string> imported_;
};

}  // namespace v
