#pragma once
#include <deque>
#include <map>
#include <stdexcept>
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

// `F(a)` with a concrete `a` that has no value yet, and the value cannot be made: `a` is not
// established to be in Domain(F), or Domain(F) does not exist. `missing` is `a ∈ Domain(F)`
// when that is what is missing.
struct Unapplied : std::runtime_error {
  PropId missing = kNoProp;
  Unapplied(const std::string& msg, PropId missing)
      : std::runtime_error(msg), missing(missing) {}
};

class World {
 public:
  // --- Object store ---
  ObjectId declare(const std::string& name);              // always a new object
  ObjectId tuple(const std::vector<ObjectId>& elems);     // identity by components
  ObjectId numeral(const Rational& value);                // identity by value
  ObjectId text(const std::string& value);                // identity by the characters
  // A variable: a new object, like a declaration, that a binder can bind.
  ObjectId variable(const std::string& name);

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
  // PropId. A variable is keyed by its position among the binders around it, so binding a
  // different variable object gives the same PropId.
  PropId atom(Term subject, Term column, bool positive);
  PropId conj(PropId a, PropId b);
  PropId disj(PropId a, PropId b);
  PropId implies(PropId a, PropId b);
  PropId iff(PropId a, PropId b);
  PropId neg(PropId a);
  // Bind the variable object `var` in `body`.
  PropId forAll(ObjectId var, PropId body);
  PropId exists(ObjectId var, PropId body);

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

  // Put an object in place of the variable the outermost binder binds. Where that makes
  // F(a) with no value yet: with `apply` it is made the way `Apply F to a` makes it, domain
  // check included — applying a rule; without, only the object is made, and nothing is
  // stated about it — taking a witness, or checking what a scope proved.
  PropId instantiate(PropId binderProp, ObjectId with, int line = 0, bool apply = true);
  // Put `to` in place of the object `from` everywhere in a proposition, `F(from)` included.
  PropId substitute(PropId id, ObjectId from, ObjectId to, int line = 0, bool apply = true);
  // Fill the holes of a Notation template — variables of the template — with terms.
  PropId fillHoles(PropId templateProp, const std::vector<ObjectId>& holes,
                   const std::vector<Term>& args);

  // Split a premise joined by top-level `and` into a list of premises.
  std::vector<PropId> premisesOf(PropId antecedent) const;

  // A standalone copy of a proposition. Leaving a scope must build the proposition first,
  // roll back to the mark, and only then store it — so the proposition has to survive the
  // rollback, while every PropId built inside the scope is removed.
  //
  // Objects in it may have been made inside the scope too: its variables, `F(x)` with them,
  // tuples, numerals. The copy keeps how each was made, and rebuilding makes them again, so
  // the copy does not point at objects the rollback removed. `from` is the object count when
  // the scope opened; objects below it survive the rollback as they are.
  struct Remade {
    Kind kind = Kind::Declared;
    std::string name;
    bool variable = false;
    std::vector<ObjectId> elems;               // a tuple
    Rational value;                            // a numeral
    ObjectId fn = kNoObject, arg = kNoObject;  // F(x)
  };
  struct PropTree {
    PropKind kind = PropKind::Atom;
    Term subject, column;
    bool positive = true;
    std::vector<PropTree> kids;
    ObjectId bound = kNoObject;
    std::map<ObjectId, Remade> remade;  // root only
  };
  PropTree snapshot(PropId id, ObjectId from) const;
  PropId rebuild(const PropTree& t);

  // Which objects occur in a proposition — used to check that a conclusion leaving a scope
  // does not mention a witness that has disappeared.
  void objectsIn(PropId id, std::vector<ObjectId>& out) const;

  // A ground term resolves to one object; a tuple term builds a tuple object. A term with a
  // variable in it is not ground: it resolves to an open object.
  bool ground(const Term& t) const;
  ObjectId resolve(const Term& t);

  // --- Function application ---
  // The value of F at a, made by `Apply F to a` or by putting a in place of x in `F(x)`.
  ObjectId appliedValue(ObjectId fn, ObjectId arg) const;
  // Whether `value` was made as F(a), and if so which F and a.
  bool appliedFrom(ObjectId value, ObjectId& fn, ObjectId& arg) const;
  // `Apply F to a as name`: the value is stipulated.
  void stipulateValue(ObjectId fn, ObjectId arg, ObjectId value, int line);
  // F(a). With a variable in F or a, the value is an open object, made on first use. With
  // neither, it is the value already made, or a new one when `a ∈ Domain(F)` holds — the
  // same step as `Apply F to a`; otherwise it throws Unapplied.
  //
  // A value can exist without `(a, F(a)) ∈ F`: one made inside a scope that has closed.
  // `establish` asks for that fact too, with the same domain check — what `Apply` does.
  ObjectId apply(ObjectId fn, ObjectId arg, int line, bool establish = false);
  // Which object is `Domain`. Declared by the prelude, so it is handed over once that runs.
  void setDomain(ObjectId domain) { domain_ = domain; }

  // --- Propositions as objects ---
  // The mechanism provides exactly one thing: the pairing. The pairing is lazy — a
  // representative comes into existence only when the proof names it.
  //
  // A representative is a structured term built from the representatives of the parts, so
  // its shape can be read off. `Holds` is a cell like any other, so it follows scopes.
  ObjectId represent(PropId id);
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
    ObjectCreated, FlagRaised, ReasonAdded, PropCreated, PropTold, RepCreated, TagCreated,
    ValueMade
  };
  struct Entry {
    Undo what;
    ObjectId object = kNoObject;  // ObjectCreated
    CellKey cell{kNoObject, kNoObject};  // ValueMade: (F, a)
    bool positive = true;
    PropId prop = kNoProp;  // PropCreated / PropTold
  };

  // Propositions are interned by a key that writes each variable as its position among the
  // binders around it, so two propositions differing only in which variable objects they
  // bind get the same key: they are the same proposition.
  using Key = std::vector<uint64_t>;
  void keyObj(ObjectId o, std::vector<ObjectId>& ctx, Key& k) const;
  void keyTerm(const Term& t, std::vector<ObjectId>& ctx, Key& k) const;
  void keyProp(PropId id, std::vector<ObjectId>& ctx, Key& k) const;
  void keyShape(const Prop& p, std::vector<ObjectId>& ctx, Key& k) const;
  void varsOf(ObjectId o, std::vector<ObjectId>& out) const;
  void varsOf(const Term& t, std::vector<ObjectId>& out) const;
  bool mentions(ObjectId o, ObjectId x) const;
  ObjectId substObj(ObjectId o, ObjectId from, ObjectId to, bool top, int line, bool apply);
  Term substTerm(const Term& t, ObjectId from, ObjectId to, int line, bool apply);
  ObjectId makeValue(ObjectId fn, ObjectId arg, bool open);
  ObjectId represent(PropId id, std::vector<ObjectId>& ctx);
  // Read a representative off its shape: (Mem, t, S), (And, a, b), (All, a) with `(Var, k)`
  // inside, (App, F, x), … — the proposition it stands for, or kNoProp if it is not one.
  PropId decode(ObjectId rep, std::vector<ObjectId>& ctx);
  bool decodeTerm(ObjectId o, std::vector<ObjectId>& ctx, ObjectId& out);
  ObjectId representTerm(const Term& t, const std::vector<ObjectId>& ctx);
  ObjectId representObj(ObjectId o, const std::vector<ObjectId>& ctx);

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
  std::map<std::string, ObjectId> texts_;
  std::deque<Prop> props_;
  std::deque<Key> keys_;  // keys_[id] is the key of props_[id]
  std::map<Key, PropId> propIndex_;
  std::map<std::pair<ObjectId, ObjectId>, ObjectId> values_;  // (F, a) -> F(a)
  std::map<ObjectId, std::pair<ObjectId, ObjectId>> madeFrom_;  // F(a) -> (F, a), when made
  ObjectId domain_ = kNoObject;
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
