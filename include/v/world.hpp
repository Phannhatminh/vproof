#pragma once
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "v/core.hpp"
#include "v/prop.hpp"

namespace v {

using CellKey = std::pair<ObjectId, ObjectId>;  // (element position, column position)

// One premise of the step that produced a conclusion. A premise is a proposition — it may
// be a flag in a cell or an entry in the store; both are PropIds, so the reason tree spans
// both places without distinguishing them.
struct Antecedent {
  PropId prop = kNoProp;
};

// Reason: why a flag is raised. Two kinds, and only two.
//   stipulated — a statement in the program said it outright; keeps the source line.
//   derived    — a rule application; keeps the rule, the binding, and the premises.
struct Reason {
  bool stipulated = true;
  int line = 0;
  std::string rule;
  std::vector<std::pair<std::string, ObjectId>> binding;
  std::vector<Antecedent> antecedents;

  static Reason stipulate(int line) {
    Reason r;
    r.stipulated = true;
    r.line = line;
    return r;
  }
  static Reason derive(std::string rule, int line) {
    Reason r;
    r.stipulated = false;
    r.line = line;
    r.rule = std::move(rule);
    return r;
  }
};

// A cell carries two independent flags. No rule links them, and there is no "conflict"
// label: both being raised is an ordinary state.
struct Cell {
  bool in = false, out = false;
  Step inStep = 0, outStep = 0;
  std::vector<Reason> inReasons, outReasons;
};

using Mark = size_t;  // position in the journal

class World {
 public:
  // --- Object store ---
  ObjectId declare(const std::string& name);              // always a new object
  ObjectId tuple(const std::vector<ObjectId>& elems);     // identity by components
  ObjectId numeral(const Rational& value);                // identity by value

  const Object& obj(ObjectId id) const { return objects_[id]; }
  size_t objectCount() const { return objects_.size(); }
  std::string show(ObjectId id) const;

  // --- Membership matrix ---
  // Writing always succeeds. Flags are only raised, never lowered. Raising a flag that is
  // already up only adds another reason — all are kept, none replaced.
  void tellIn(ObjectId subject, ObjectId column, Reason why);
  void tellOut(ObjectId subject, ObjectId column, Reason why);

  bool toldIn(ObjectId subject, ObjectId column) const;
  bool toldOut(ObjectId subject, ObjectId column) const;
  const std::vector<Reason>& reasons(ObjectId subject, ObjectId column, bool positive) const;
  Step stepOf(ObjectId subject, ObjectId column, bool positive) const;

  size_t cellCount() const { return cells_.size(); }

  // --- Proposition store ---
  // Propositions are interned by structure: building the same shape twice gives the same
  // PropId. Bound variables are stored as indices, so alpha-renaming is free.
  PropId atom(Term subject, Term column, bool positive);
  PropId conj(PropId a, PropId b);
  PropId disj(PropId a, PropId b);
  PropId implies(PropId a, PropId b);
  PropId iff(PropId a, PropId b);
  PropId neg(PropId a);
  PropId forAll(const std::string& name, PropId body);
  PropId exists(const std::string& name, PropId body);

  const Prop& prop(PropId id) const { return props_[id]; }
  size_t propCount() const { return props_.size(); }
  std::string showProp(PropId id) const;

  // Store and look up. Ground atoms go into the matrix; everything else goes into the
  // store.
  void tell(PropId id, Reason why);
  bool holds(PropId id);
  const std::vector<Reason>& propReasons(PropId id) const;

  // --- Rule application ---
  // Substitute objects for the outermost bound variables, look up each premise, record the
  // conclusion. Lookup, not search: if a premise is missing, the step does not go through.
  struct StepResult {
    bool ok = false;
    PropId conclusion = kNoProp;
    PropId missing = kNoProp;  // first premise not found
  };
  StepResult applyRule(PropId rule, const std::vector<ObjectId>& args,
                       const std::string& label, int line);

  // Substitute an object for the outermost bound variable of a proposition with a binder.
  PropId instantiate(PropId binderProp, ObjectId with);
  // Fill the holes of a Notation template with terms.
  PropId fillHoles(PropId templateProp, const std::vector<Term>& args);

  // Split a premise joined by top-level `and` into a list of premises.
  std::vector<PropId> premisesOf(PropId antecedent) const;

  // A standalone copy of a proposition. Leaving a scope must build the proposition first,
  // roll back to the mark, and only then store it — so the proposition has to survive the
  // rollback, while every PropId built inside the scope is removed.
  struct PropTree {
    PropKind kind = PropKind::Atom;
    Term subject, column;
    bool positive = true;
    std::vector<PropTree> kids;
    std::string binderName;
  };
  PropTree snapshot(PropId id) const;
  PropId rebuild(const PropTree& t);

  // Which objects occur in a proposition — used to check that a conclusion leaving a scope
  // does not mention a witness that has disappeared.
  void objectsIn(PropId id, std::vector<ObjectId>& out) const;

  // A ground term resolves to one object; a tuple term builds a tuple object.
  bool ground(const Term& t) const;
  ObjectId resolve(const Term& t);

  // --- Propositions as objects ---
  // The mechanism provides exactly one thing: the pairing. The pairing is lazy — a
  // representative comes into existence only when the proof names it.
  //
  // A representative is a structured term built from the representatives of the parts, so
  // its shape can be read off. `Holds` is a cell like any other, so it follows scopes.
  ObjectId represent(PropId id);
  // Representative of a term, used inside the body of a binder: the bound variable at index
  // k becomes `(Var, k)`. Indices rather than names are required — two propositions
  // differing only in bound-variable names are the same proposition, so their
  // representatives must coincide.
  ObjectId representTerm(const Term& t);
  PropId representedBy(ObjectId rep) const;
  ObjectId holdsColumn();
  // Relation recording that an object has been used as a column. The mechanism checks
  // nothing before writing a cell — `alice ∈ 5` can be written — but it records it, so that
  // theories have the data to judge for themselves.
  ObjectId columnTag();
  ObjectId tag(const std::string& name);

  // --- Journal ---
  Mark mark() const { return journal_.size(); }
  void rollback(Mark m);

 private:
  enum class Undo {
    ObjectCreated, FlagRaised, ReasonAdded, PropCreated, PropTold, RepCreated, TagCreated
  };
  struct Entry {
    Undo what;
    ObjectId object = kNoObject;  // ObjectCreated
    CellKey cell{kNoObject, kNoObject};
    bool positive = true;
    PropId prop = kNoProp;  // PropCreated / PropTold
  };

  struct PropLess {
    bool operator()(const Prop& a, const Prop& b) const { return a.shapeLess(b); }
  };

  PropId intern(Prop p);
  // Keep `Holds` in sync with the store, in both directions. This is an obligation of the
  // mechanism and is the trust surface of the whole reflection layer.
  void syncFromProp(PropId id, const Reason& why);
  void syncFromHolds(ObjectId rep, const Reason& why);

  Cell& cellFor(CellKey k);
  void tell(ObjectId subject, ObjectId column, bool positive, Reason why);

  std::deque<Object> objects_;  // deque: references stay valid as the store grows
  std::map<CellKey, Cell> cells_;
  std::map<std::vector<ObjectId>, ObjectId> tuples_;
  std::map<Rational, ObjectId> numerals_;
  std::deque<Prop> props_;
  std::map<Prop, PropId, PropLess> propIndex_;
  std::map<PropId, std::vector<Reason>> held_;
  std::map<PropId, ObjectId> repOf_;
  std::map<ObjectId, PropId> propOf_;
  std::map<std::string, ObjectId> tags_;
  ObjectId holds_ = kNoObject;
  ObjectId column_ = kNoObject;
  std::vector<Entry> journal_;
  Step step_ = 0;
};

}  // namespace v
