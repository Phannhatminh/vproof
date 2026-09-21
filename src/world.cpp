#include "v/world.hpp"

#include <functional>
#include <stdexcept>

namespace v {

ObjectId World::declare(const std::string& name) {
  Object o;
  o.id = static_cast<ObjectId>(objects_.size());
  o.kind = Kind::Declared;
  o.name = name;
  objects_.push_back(std::move(o));
  journal_.push_back({Undo::ObjectCreated, objects_.back().id, {}, true});
  return objects_.back().id;
}

ObjectId World::tuple(const std::vector<ObjectId>& elems) {
  auto it = tuples_.find(elems);
  if (it != tuples_.end()) return it->second;  // same components, same object

  Object o;
  o.id = static_cast<ObjectId>(objects_.size());
  o.kind = Kind::Tuple;
  o.elems = elems;
  o.name = "(";
  for (size_t i = 0; i < elems.size(); ++i)
    o.name += (i ? ", " : "") + objects_[elems[i]].name;
  o.name += ")";
  objects_.push_back(std::move(o));
  tuples_[elems] = objects_.back().id;
  journal_.push_back({Undo::ObjectCreated, objects_.back().id, {}, true});
  return objects_.back().id;
}

ObjectId World::numeral(const Rational& value) {
  auto it = numerals_.find(value);
  if (it != numerals_.end()) return it->second;  // same value, same object

  Object o;
  o.id = static_cast<ObjectId>(objects_.size());
  o.kind = Kind::Numeral;
  o.value = value;
  o.name = value.str();
  objects_.push_back(std::move(o));
  numerals_[value] = objects_.back().id;
  journal_.push_back({Undo::ObjectCreated, objects_.back().id, {}, true});
  return objects_.back().id;
}

std::string World::show(ObjectId id) const {
  return id == kNoObject ? "?" : objects_[id].name;
}

Cell& World::cellFor(CellKey k) { return cells_[k]; }

void World::tell(ObjectId subject, ObjectId column, bool positive, Reason why) {
  CellKey k{subject, column};
  Cell& c = cellFor(k);
  bool& flag = positive ? c.in : c.out;
  Step& at = positive ? c.inStep : c.outStep;
  auto& list = positive ? c.inReasons : c.outReasons;

  if (!flag) {
    flag = true;
    at = ++step_;
    journal_.push_back({Undo::FlagRaised, kNoObject, k, positive});
  }
  int line = why.line;
  list.push_back(std::move(why));
  journal_.push_back({Undo::ReasonAdded, kNoObject, k, positive});

  // Record that `column` has been used as a column. Checks nothing, refuses nothing — it
  // only supplies data. `Column` itself is skipped to avoid looping.
  ObjectId col = columnTag();
  if (column != col && !toldIn(column, col)) {
    Cell& c2 = cells_[{column, col}];
    c2.in = true;
    c2.inStep = ++step_;
    journal_.push_back({Undo::FlagRaised, kNoObject, {column, col}, true});
    c2.inReasons.push_back(Reason::derive("dùng làm cột", line));
    journal_.push_back({Undo::ReasonAdded, kNoObject, {column, col}, true});
  }
}

void World::tellIn(ObjectId subject, ObjectId column, Reason why) {
  bool wasOn = toldIn(subject, column);
  Reason copy = why;
  tell(subject, column, true, std::move(why));
  // Writing `p ∈ Holds` also stores the proposition p represents. The two directions must
  // agree, or the whole reflection layer lies.
  if (!wasOn && column == holds_ && holds_ != kNoObject) syncFromHolds(subject, copy);
}

void World::tellOut(ObjectId subject, ObjectId column, Reason why) {
  tell(subject, column, false, std::move(why));
}

bool World::toldIn(ObjectId subject, ObjectId column) const {
  auto it = cells_.find({subject, column});
  return it != cells_.end() && it->second.in;
}

bool World::toldOut(ObjectId subject, ObjectId column) const {
  auto it = cells_.find({subject, column});
  return it != cells_.end() && it->second.out;
}

const std::vector<Reason>& World::reasons(ObjectId subject, ObjectId column,
                                          bool positive) const {
  static const std::vector<Reason> none;
  auto it = cells_.find({subject, column});
  if (it == cells_.end()) return none;
  return positive ? it->second.inReasons : it->second.outReasons;
}

Step World::stepOf(ObjectId subject, ObjectId column, bool positive) const {
  auto it = cells_.find({subject, column});
  if (it == cells_.end()) return 0;
  return positive ? it->second.inStep : it->second.outStep;
}

void World::rollback(Mark m) {
  if (m > journal_.size()) throw std::runtime_error("mốc không hợp lệ");

  while (journal_.size() > m) {
    Entry e = journal_.back();
    journal_.pop_back();

    switch (e.what) {
      case Undo::ReasonAdded: {
        Cell& c = cells_[e.cell];
        auto& list = e.positive ? c.inReasons : c.outReasons;
        list.pop_back();
        break;
      }
      case Undo::FlagRaised: {
        // This flag was raised by the scope. A flag already up before the scope has no
        // FlagRaised entry in the part of the journal being undone, so it stays up.
        Cell& c = cells_[e.cell];
        (e.positive ? c.in : c.out) = false;
        (e.positive ? c.inStep : c.outStep) = 0;
        if (!c.in && !c.out && c.inReasons.empty() && c.outReasons.empty())
          cells_.erase(e.cell);  // the cell goes back to "nobody has said anything"
        break;
      }
      case Undo::PropTold: {
        auto it = held_.find(e.prop);
        if (it != held_.end()) {
          it->second.pop_back();
          if (it->second.empty()) held_.erase(it);
        }
        break;
      }
      case Undo::PropCreated: {
        const Prop& p = props_.back();
        propIndex_.erase(p);
        props_.pop_back();
        break;
      }
      case Undo::RepCreated: {
        repOf_.erase(e.prop);
        propOf_.erase(e.object);
        break;
      }
      case Undo::TagCreated: {
        for (auto it = tags_.begin(); it != tags_.end(); ++it)
          if (it->second == e.object) {
            if (holds_ == e.object) holds_ = kNoObject;
            tags_.erase(it);
            break;
          }
        break;
      }
      case Undo::ObjectCreated: {
        const Object& o = objects_.back();
        if (o.id != e.object) throw std::runtime_error("nhật ký lệch với kho đối tượng");
        if (o.kind == Kind::Tuple) tuples_.erase(o.elems);
        if (o.kind == Kind::Numeral) numerals_.erase(o.value);
        objects_.pop_back();
        break;
      }
    }
  }
}


// ---------------------------------------------------------------- propositions

PropId World::intern(Prop p) {
  auto it = propIndex_.find(p);
  if (it != propIndex_.end()) return it->second;

  PropId id = static_cast<PropId>(props_.size());
  props_.push_back(p);
  propIndex_.emplace(std::move(p), id);
  journal_.push_back({Undo::PropCreated, kNoObject, {}, true, id});
  return id;
}

PropId World::atom(Term subject, Term column, bool positive) {
  // Ground terms resolve to an object right away, so an atomic proposition has exactly one
  // shape. `(a, b) ∈ R` cannot be written two ways that give two different PropIds pointing
  // at the same cell.
  auto normalize = [this](Term t) {
    return ground(t) ? Term::of(resolve(t)) : t;
  };
  Prop p;
  p.kind = PropKind::Atom;
  p.subject = normalize(std::move(subject));
  p.column = normalize(std::move(column));
  p.positive = positive;
  return intern(std::move(p));
}

static Prop binary(PropKind k, PropId a, PropId b) {
  Prop p;
  p.kind = k;
  p.left = a;
  p.right = b;
  return p;
}

PropId World::conj(PropId a, PropId b) { return intern(binary(PropKind::And, a, b)); }
PropId World::disj(PropId a, PropId b) { return intern(binary(PropKind::Or, a, b)); }
PropId World::implies(PropId a, PropId b) { return intern(binary(PropKind::Implies, a, b)); }
PropId World::iff(PropId a, PropId b) { return intern(binary(PropKind::Iff, a, b)); }

PropId World::neg(PropId a) {
  Prop p;
  p.kind = PropKind::Not;
  p.left = a;
  return intern(std::move(p));
}

PropId World::forAll(const std::string& name, PropId body) {
  Prop p;
  p.kind = PropKind::ForAll;
  p.left = body;
  p.binderName = name;
  return intern(std::move(p));
}

PropId World::exists(const std::string& name, PropId body) {
  Prop p;
  p.kind = PropKind::Exists;
  p.left = body;
  p.binderName = name;
  return intern(std::move(p));
}

bool World::ground(const Term& t) const {
  switch (t.kind) {
    case TermKind::Obj: return true;
    case TermKind::Var: return false;
    case TermKind::Tuple:
      for (const Term& e : t.elems)
        if (!ground(e)) return false;
      return true;
  }
  return false;
}

ObjectId World::resolve(const Term& t) {
  switch (t.kind) {
    case TermKind::Obj:
      return t.obj;
    case TermKind::Var:
      throw std::runtime_error("term còn biến tự do, không quy về đối tượng được");
    case TermKind::Tuple: {
      std::vector<ObjectId> ids;
      ids.reserve(t.elems.size());
      for (const Term& e : t.elems) ids.push_back(resolve(e));
      return tuple(ids);
    }
  }
  throw std::runtime_error("term hỏng");
}

void World::tell(PropId id, Reason why) {
  const Prop& p = props_[id];
  // Ground atoms have their own indexed storage: the matrix. Everything else goes into the
  // store.
  if (p.kind == PropKind::Atom && ground(p.subject) && ground(p.column)) {
    ObjectId s = resolve(p.subject), c = resolve(p.column);
    if (p.positive) tellIn(s, c, why);
    else tellOut(s, c, why);
    syncFromProp(id, why);
    return;
  }
  held_[id].push_back(why);
  journal_.push_back({Undo::PropTold, kNoObject, {}, true, id});
  syncFromProp(id, why);
}

bool World::holds(PropId id) {
  const Prop& p = props_[id];
  if (p.kind == PropKind::Atom && ground(p.subject) && ground(p.column)) {
    ObjectId s = resolve(p.subject), c = resolve(p.column);
    return p.positive ? toldIn(s, c) : toldOut(s, c);
  }
  auto it = held_.find(id);
  return it != held_.end() && !it->second.empty();
}

const std::vector<Reason>& World::propReasons(PropId id) const {
  static const std::vector<Reason> none;
  const Prop& p = props_[id];
  if (p.kind == PropKind::Atom && ground(p.subject) && ground(p.column)) {
    // Resolve a ground term to an object without creating anything if the tuple already
    // exists. Looking up reasons must not make the world grow, so this only reads.
    auto lookup = [this](const Term& t) -> ObjectId {
      if (t.kind == TermKind::Obj) return t.obj;
      std::vector<ObjectId> ids;
      for (const Term& e : t.elems) {
        if (e.kind != TermKind::Obj) return kNoObject;
        ids.push_back(e.obj);
      }
      auto it = tuples_.find(ids);
      return it == tuples_.end() ? kNoObject : it->second;
    };
    ObjectId s = lookup(p.subject), c = lookup(p.column);
    if (s == kNoObject || c == kNoObject) return none;
    return reasons(s, c, p.positive);
  }
  auto it = held_.find(id);
  return it == held_.end() ? none : it->second;
}

std::string World::showProp(PropId id) const {
  std::vector<std::string> binders;  // innermost first
  std::function<std::string(PropId)> go = [&](PropId cur) -> std::string {
    const Prop& p = props_[cur];
    std::function<std::string(const Term&)> term = [&](const Term& t) -> std::string {
      switch (t.kind) {
        case TermKind::Obj: return show(t.obj);
        case TermKind::Var:
          return t.var < binders.size() ? binders[t.var] : "?" + std::to_string(t.var);
        case TermKind::Tuple: {
          std::string out = "(";
          for (size_t i = 0; i < t.elems.size(); ++i)
            out += (i ? ", " : "") + term(t.elems[i]);
          return out + ")";
        }
      }
      return "?";
    };
    switch (p.kind) {
      case PropKind::Atom:
        return term(p.subject) + (p.positive ? " ∈ " : " ∉ ") + term(p.column);
      case PropKind::And:     return "(" + go(p.left) + " and " + go(p.right) + ")";
      case PropKind::Or:      return "(" + go(p.left) + " or " + go(p.right) + ")";
      case PropKind::Implies: return "(if " + go(p.left) + " then " + go(p.right) + ")";
      case PropKind::Iff:     return "(" + go(p.left) + " iff " + go(p.right) + ")";
      case PropKind::Not:     return "not (" + go(p.left) + ")";
      case PropKind::ForAll:
      case PropKind::Exists: {
        binders.insert(binders.begin(), p.binderName);
        std::string body = go(p.left);
        std::string head = p.kind == PropKind::ForAll
                               ? "for every " + p.binderName + ", "
                               : "there exists " + p.binderName + " such that ";
        binders.erase(binders.begin());
        return head + body;
      }
    }
    return "?";
  };
  return go(id);
}


// ---------------------------------------------------------------- rule application

namespace {

// Replace Var(depth) with the object, and shift every free variable further out down by
// one.
Term substTerm(const Term& t, VarId depth, ObjectId with) {
  switch (t.kind) {
    case TermKind::Obj:
      return t;
    case TermKind::Var:
      if (t.var == depth) return Term::of(with);
      return t.var > depth ? Term::ofVar(t.var - 1) : t;
    case TermKind::Tuple: {
      std::vector<Term> out;
      out.reserve(t.elems.size());
      for (const Term& e : t.elems) out.push_back(substTerm(e, depth, with));
      return Term::ofTuple(std::move(out));
    }
  }
  return t;
}

}  // namespace

PropId World::instantiate(PropId binderProp, ObjectId with) {
  const Prop& b = props_[binderProp];
  if (b.kind != PropKind::ForAll && b.kind != PropKind::Exists)
    throw std::runtime_error("mệnh đề này không có biến buộc để thế");

  std::function<PropId(PropId, VarId)> go = [&](PropId cur, VarId depth) -> PropId {
    const Prop& p = props_[cur];
    switch (p.kind) {
      case PropKind::Atom:
        return atom(substTerm(p.subject, depth, with), substTerm(p.column, depth, with),
                    p.positive);
      case PropKind::And:     return conj(go(p.left, depth), go(p.right, depth));
      case PropKind::Or:      return disj(go(p.left, depth), go(p.right, depth));
      case PropKind::Implies: return implies(go(p.left, depth), go(p.right, depth));
      case PropKind::Iff:     return iff(go(p.left, depth), go(p.right, depth));
      case PropKind::Not:     return neg(go(p.left, depth));
      case PropKind::ForAll:  return forAll(p.binderName, go(p.left, depth + 1));
      case PropKind::Exists:  return exists(p.binderName, go(p.left, depth + 1));
    }
    throw std::runtime_error("mệnh đề hỏng");
  };
  return go(b.left, 0);
}

namespace {

Term fillTerm(const Term& t, const std::vector<Term>& args) {
  if (t.kind == TermKind::Var && t.var >= kHole) {
    size_t i = t.var - kHole;
    if (i >= args.size()) throw std::runtime_error("mẫu thiếu đối số");
    return args[i];
  }
  if (t.kind != TermKind::Tuple) return t;
  std::vector<Term> out;
  out.reserve(t.elems.size());
  for (const Term& e : t.elems) out.push_back(fillTerm(e, args));
  return Term::ofTuple(std::move(out));
}

}  // namespace

PropId World::fillHoles(PropId templateProp, const std::vector<Term>& args) {
  std::function<PropId(PropId)> go = [&](PropId cur) -> PropId {
    const Prop& p = props_[cur];
    switch (p.kind) {
      case PropKind::Atom:
        return atom(fillTerm(p.subject, args), fillTerm(p.column, args), p.positive);
      case PropKind::And:     return conj(go(p.left), go(p.right));
      case PropKind::Or:      return disj(go(p.left), go(p.right));
      case PropKind::Implies: return implies(go(p.left), go(p.right));
      case PropKind::Iff:     return iff(go(p.left), go(p.right));
      case PropKind::Not:     return neg(go(p.left));
      case PropKind::ForAll:
      case PropKind::Exists:
        throw std::runtime_error("mẫu Notation không chứa lượng từ được");
    }
    throw std::runtime_error("mệnh đề hỏng");
  };
  return go(templateProp);
}

std::vector<PropId> World::premisesOf(PropId antecedent) const {
  std::vector<PropId> out;
  std::function<void(PropId)> flatten = [&](PropId cur) {
    const Prop& p = props_[cur];
    if (p.kind == PropKind::And) {
      flatten(p.left);
      flatten(p.right);
      return;
    }
    out.push_back(cur);
  };
  flatten(antecedent);
  return out;
}

World::StepResult World::applyRule(PropId rule, const std::vector<ObjectId>& args,
                                   const std::string& label, int line) {
  StepResult res;

  // Peel the outermost binders one by one, substituting the objects the author supplied.
  PropId body = rule;
  std::vector<std::pair<std::string, ObjectId>> binding;
  for (ObjectId a : args) {
    const Prop& p = props_[body];
    if (p.kind != PropKind::ForAll)
      throw std::runtime_error("áp luật với nhiều đối số hơn số biến của luật");
    binding.emplace_back(p.binderName, a);
    body = instantiate(body, a);
  }

  // The body is either `if premises then conclusion`, or just the conclusion.
  PropId conclusion = body;
  std::vector<PropId> premises;
  if (props_[body].kind == PropKind::Implies) {
    premises = premisesOf(props_[body].left);
    conclusion = props_[body].right;
  }

  for (PropId prem : premises) {
    if (!holds(prem)) {
      res.missing = prem;
      return res;  // the step does not go through; nothing is written
    }
  }

  Reason why = Reason::derive(label, line);
  why.binding = std::move(binding);
  for (PropId prem : premises) why.antecedents.push_back(Antecedent{prem});
  tell(conclusion, std::move(why));

  res.ok = true;
  res.conclusion = conclusion;
  return res;
}


// ------------------------------------------------- standalone copies

World::PropTree World::snapshot(PropId id) const {
  const Prop& p = props_[id];
  PropTree t;
  t.kind = p.kind;
  t.subject = p.subject;
  t.column = p.column;
  t.positive = p.positive;
  t.binderName = p.binderName;
  if (p.left != kNoProp) t.kids.push_back(snapshot(p.left));
  if (p.right != kNoProp) t.kids.push_back(snapshot(p.right));
  return t;
}

PropId World::rebuild(const PropTree& t) {
  switch (t.kind) {
    case PropKind::Atom:    return atom(t.subject, t.column, t.positive);
    case PropKind::And:     return conj(rebuild(t.kids[0]), rebuild(t.kids[1]));
    case PropKind::Or:      return disj(rebuild(t.kids[0]), rebuild(t.kids[1]));
    case PropKind::Implies: return implies(rebuild(t.kids[0]), rebuild(t.kids[1]));
    case PropKind::Iff:     return iff(rebuild(t.kids[0]), rebuild(t.kids[1]));
    case PropKind::Not:     return neg(rebuild(t.kids[0]));
    case PropKind::ForAll:  return forAll(t.binderName, rebuild(t.kids[0]));
    case PropKind::Exists:  return exists(t.binderName, rebuild(t.kids[0]));
  }
  throw std::runtime_error("mệnh đề hỏng");
}

void World::objectsIn(PropId id, std::vector<ObjectId>& out) const {
  std::function<void(const Term&)> term = [&](const Term& t) {
    if (t.kind == TermKind::Obj) out.push_back(t.obj);
    for (const Term& e : t.elems) term(e);
  };
  std::function<void(PropId)> go = [&](PropId cur) {
    const Prop& p = props_[cur];
    if (p.kind == PropKind::Atom) {
      term(p.subject);
      term(p.column);
      return;
    }
    if (p.left != kNoProp) go(p.left);
    if (p.right != kNoProp) go(p.right);
  };
  go(id);
}


// ------------------------------------------------- propositions as objects

ObjectId World::tag(const std::string& name) {
  auto it = tags_.find(name);
  if (it != tags_.end()) return it->second;
  ObjectId id = declare(name);
  tags_[name] = id;
  journal_.push_back({Undo::TagCreated, id, {}, true, kNoProp});
  return id;
}

ObjectId World::holdsColumn() {
  if (holds_ == kNoObject) holds_ = tag("Holds");
  return holds_;
}

ObjectId World::columnTag() {
  if (column_ == kNoObject) column_ = tag("Column");
  return column_;
}

ObjectId World::representTerm(const Term& t) {
  switch (t.kind) {
    case TermKind::Obj:
      return t.obj;
    case TermKind::Var:
      return tuple({tag("Var"), numeral(Rational(static_cast<long long>(t.var)))});
    case TermKind::Tuple: {
      std::vector<ObjectId> ids;
      ids.reserve(t.elems.size());
      for (const Term& e : t.elems) ids.push_back(representTerm(e));
      return tuple(ids);
    }
  }
  throw std::runtime_error("term hỏng");
}

ObjectId World::represent(PropId id) {
  auto it = repOf_.find(id);
  if (it != repOf_.end()) return it->second;

  const Prop& p = props_[id];
  ObjectId rep = kNoObject;
  switch (p.kind) {
    case PropKind::Atom: {
      ObjectId s = representTerm(p.subject), c = representTerm(p.column);
      rep = tuple({tag(p.positive ? "Mem" : "NotMem"), s, c});
      break;
    }
    case PropKind::And:     rep = tuple({tag("And"), represent(p.left), represent(p.right)}); break;
    case PropKind::Or:      rep = tuple({tag("Or"), represent(p.left), represent(p.right)}); break;
    case PropKind::Implies: rep = tuple({tag("Implies"), represent(p.left), represent(p.right)}); break;
    case PropKind::Iff:     rep = tuple({tag("Iff"), represent(p.left), represent(p.right)}); break;
    case PropKind::Not:     rep = tuple({tag("Not"), represent(p.left)}); break;
    case PropKind::ForAll:
    case PropKind::Exists:
      // Structured: the body is represented too, with bound variables as `(Var, k)`. So
      // rules that look inside a quantified proposition can be written.
      rep = tuple({tag(p.kind == PropKind::ForAll ? "All" : "Ex"), represent(p.left)});
      break;
  }

  repOf_[id] = rep;
  propOf_[rep] = id;
  journal_.push_back({Undo::RepCreated, rep, {}, true, id});

  // A representative created after its proposition was stored must catch up with Holds
  // immediately.
  if (holds(id)) {
    Reason why = Reason::derive("holds", 0);
    why.antecedents = {Antecedent{id}};
    tellIn(rep, holdsColumn(), std::move(why));
  }
  return rep;
}

PropId World::representedBy(ObjectId rep) const {
  auto it = propOf_.find(rep);
  return it == propOf_.end() ? kNoProp : it->second;
}

void World::syncFromProp(PropId id, const Reason& base) {
  auto it = repOf_.find(id);
  if (it == repOf_.end()) return;           // nobody has named it as an object yet
  if (toldIn(it->second, holdsColumn())) return;  // already in sync
  Reason why = Reason::derive("holds", base.line);
  why.antecedents = {Antecedent{id}};
  tellIn(it->second, holdsColumn(), std::move(why));
}

void World::syncFromHolds(ObjectId rep, const Reason& base) {
  auto it = propOf_.find(rep);
  if (it == propOf_.end()) return;
  if (holds(it->second)) return;
  Reason why = Reason::derive("holds", base.line);
  tell(it->second, std::move(why));
}

}  // namespace v
