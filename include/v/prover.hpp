#pragma once
#include <string>
#include <vector>

#include "v/world.hpp"

namespace v {

// The proof-step operations. There are exactly two shapes: whatever binds a variable or
// discharges an assumption needs a scope; everything else is just look up, then store.
//
// Prover keeps no logical state of its own besides the scope stack — every fact still lives
// in World.
class Prover {
 public:
  explicit Prover(World& w) : w_(w) {}

  struct Result {
    bool ok = false;
    PropId prop = kNoProp;     // the proposition just stored
    PropId missing = kNoProp;  // first premise not found
    std::string error;
  };

  // --- Stipulating ---
  Result assume(PropId p, int line);

  // --- Introduction, no scope ---
  Result introAnd(PropId a, PropId b, int line);
  Result introOr(PropId held, PropId other, bool heldOnLeft, int line);
  Result introIff(PropId aToB, PropId bToA, int line);
  // Look up A(t), store `there exists x such that A(x)`: abstracts `witness`.
  Result introExists(PropId instance, ObjectId witness, const std::string& name, int line);

  // --- Elimination, no scope ---
  Result elimAnd(PropId conjunction, bool takeLeft, int line);
  Result elimIff(PropId equivalence, bool forward, int line);

  // Point out an absurdity. Does not explode: it only marks the current scope, so that `not
  // (...)` can be built on exit.
  Result absurd(PropId p, PropId notP, int line);
  // `or` elimination: look up `A or B`, `not A`, `not B` — three separate propositions —
  // then point out the absurdity. No case split, no parallel worlds.
  Result absurdFromOr(PropId disjunction, PropId notA, PropId notB, int line);

  // --- Scope ---
  Result suppose(PropId assumption, const std::string& label, int line);
  Result take(const std::string& name, int line);
  Result takeIn(const std::string& name, ObjectId set, int line);
  Result takeFrom(const std::string& name, PropId existential, int line);

  // Exit: check that the stated proposition is what this scope can produce, then roll back
  // to the mark and store it.
  Result hence(PropId stated, const std::string& label, int line);

  size_t depth() const { return scopes_.size(); }
  ObjectId freshOf() const { return scopes_.empty() ? kNoObject : scopes_.back().fresh; }

 private:
  enum class ScopeKind { Suppose, Take, TakeIn, TakeFrom };
  struct Scope {
    ScopeKind kind = ScopeKind::Suppose;
    Mark mark = 0;
    PropId assumption = kNoProp;
    PropId membership = kNoProp;
    ObjectId fresh = kNoObject;
    std::string name, label;
    bool absurd = false;
    int line = 0;
  };

  Result fail(const std::string& msg) const;
  Result missing(PropId p) const;

  World& w_;
  std::vector<Scope> scopes_;
};

}  // namespace v
