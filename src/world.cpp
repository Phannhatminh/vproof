#include "v/world.hpp"

#include <algorithm>
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

ObjectId World::text(const std::string& value) {
  auto it = texts_.find(value);
  if (it != texts_.end()) return it->second;  // same characters, same object

  Object o;
  o.id = static_cast<ObjectId>(objects_.size());
  o.kind = Kind::Text;
  o.name = value;
  objects_.push_back(std::move(o));
  texts_[value] = objects_.back().id;
  journal_.push_back({Undo::ObjectCreated, objects_.back().id, {}, true});
  return objects_.back().id;
}

ObjectId World::variable(const std::string& name) {
  ObjectId id = declare(name);
  objects_[id].variable = true;
  objects_[id].open = true;
  return id;
}

ObjectId World::tuple(const std::vector<ObjectId>& elems) {
  auto it = tuples_.find(elems);
  if (it != tuples_.end()) return it->second;  // same components, same object

  Object o;
  o.id = static_cast<ObjectId>(objects_.size());
  o.kind = Kind::Tuple;
  o.elems = elems;
  for (ObjectId e : elems)
    if (objects_[e].open) o.open = true;
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
  if (id == kNoObject) return "?";
  // A proposition's representative prints as the proposition in brackets, wherever it sits.
  if (auto it = propOf_.find(id); it != propOf_.end()) return "[" + showProp(it->second) + "]";
  const Object& o = objects_[id];
  if (o.kind == Kind::Text) return "\"" + o.name + "\"";
  if (o.kind == Kind::Tuple && !o.elems.empty()) {
    std::string s = "(";
    for (size_t i = 0; i < o.elems.size(); ++i) s += (i ? ", " : "") + show(o.elems[i]);
    return s + ")";
  }
  return o.name;
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
    c2.inReasons.push_back(Reason::derive("used as a column", line));
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
  if (m > journal_.size()) throw std::runtime_error("invalid mark");

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
        propIndex_.erase(keys_.back());
        keys_.pop_back();
        props_.pop_back();
        break;
      }
      case Undo::ValueMade: {
        auto it = values_.find(e.cell);
        if (it != values_.end()) {
          madeFrom_.erase(it->second);
          values_.erase(it);
        }
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
        if (o.id != e.object) throw std::runtime_error("journal out of step with the object store");
        if (o.kind == Kind::Tuple) tuples_.erase(o.elems);
        if (o.kind == Kind::Numeral) numerals_.erase(o.value);
        if (o.kind == Kind::Text) texts_.erase(o.name);
        objects_.pop_back();
        break;
      }
    }
  }
}


// ---------------------------------------------------------------- propositions

// The variables an object is built from: itself if it is one, else those of its parts.
void World::varsOf(ObjectId o, std::vector<ObjectId>& out) const {
  const Object& ob = objects_[o];
  if (!ob.open) return;
  if (ob.variable) {
    out.push_back(o);
    return;
  }
  for (ObjectId e : ob.elems) varsOf(e, out);
  auto m = madeFrom_.find(o);
  if (m != madeFrom_.end()) {
    varsOf(m->second.first, out);
    varsOf(m->second.second, out);
  }
}

void World::varsOf(const Term& t, std::vector<ObjectId>& out) const {
  if (t.kind == TermKind::Obj) varsOf(t.obj, out);
  for (const Term& e : t.elems) varsOf(e, out);
}

// Key codes. A closed proposition is written as its PropId: interning already made it the
// one proposition of its shape.
enum : uint64_t {
  kObj, kBoundAt, kTupleObj, kValue, kTupleTerm, kClosedProp, kOpenProp
};

void World::keyObj(ObjectId o, std::vector<ObjectId>& ctx, Key& k) const {
  const Object& ob = objects_[o];
  if (ob.open && ob.variable) {
    auto at = std::find(ctx.begin(), ctx.end(), o);
    if (at != ctx.end()) {
      k.push_back(kBoundAt);
      k.push_back(static_cast<uint64_t>(at - ctx.begin()));
      return;
    }
  } else if (ob.open && ob.kind == Kind::Tuple) {
    k.push_back(kTupleObj);
    k.push_back(ob.elems.size());
    for (ObjectId e : ob.elems) keyObj(e, ctx, k);
    return;
  } else if (ob.open) {
    auto m = madeFrom_.find(o);
    if (m != madeFrom_.end()) {
      k.push_back(kValue);
      keyObj(m->second.first, ctx, k);
      keyObj(m->second.second, ctx, k);
      return;
    }
  }
  k.push_back(kObj);
  k.push_back(o);
}

void World::keyTerm(const Term& t, std::vector<ObjectId>& ctx, Key& k) const {
  switch (t.kind) {
    case TermKind::Obj:
      keyObj(t.obj, ctx, k);
      return;
    case TermKind::Tuple:
      k.push_back(kTupleTerm);
      k.push_back(t.elems.size());
      for (const Term& e : t.elems) keyTerm(e, ctx, k);
      return;
  }
}

void World::keyProp(PropId id, std::vector<ObjectId>& ctx, Key& k) const {
  const Prop& p = props_[id];
  if (p.freeVars.empty()) {
    k.push_back(kClosedProp);
    k.push_back(id);
    return;
  }
  k.push_back(kOpenProp);
  keyShape(p, ctx, k);
}

void World::keyShape(const Prop& p, std::vector<ObjectId>& ctx, Key& k) const {
  k.push_back(static_cast<uint64_t>(p.kind));
  switch (p.kind) {
    case PropKind::Atom:
      k.push_back(p.positive);
      keyTerm(p.subject, ctx, k);
      keyTerm(p.column, ctx, k);
      return;
    case PropKind::And:
    case PropKind::Or:
    case PropKind::Implies:
    case PropKind::Iff:
      keyProp(p.left, ctx, k);
      keyProp(p.right, ctx, k);
      return;
    case PropKind::Not:
      keyProp(p.left, ctx, k);
      return;
    case PropKind::ForAll:
    case PropKind::Exists:
      ctx.insert(ctx.begin(), p.bound);
      keyProp(p.left, ctx, k);
      ctx.erase(ctx.begin());
      return;
  }
}

PropId World::intern(Prop p) {
  std::vector<ObjectId>& fv = p.freeVars;
  fv.clear();
  if (p.kind == PropKind::Atom) {
    varsOf(p.subject, fv);
    varsOf(p.column, fv);
  } else {
    for (PropId kid : {p.left, p.right})
      if (kid != kNoProp) fv.insert(fv.end(), props_[kid].freeVars.begin(), props_[kid].freeVars.end());
    if (p.kind == PropKind::ForAll || p.kind == PropKind::Exists)
      fv.erase(std::remove(fv.begin(), fv.end(), p.bound), fv.end());
  }
  std::sort(fv.begin(), fv.end());
  fv.erase(std::unique(fv.begin(), fv.end()), fv.end());

  Key key;
  std::vector<ObjectId> ctx;
  keyShape(p, ctx, key);
  auto it = propIndex_.find(key);
  if (it != propIndex_.end()) return it->second;

  PropId id = static_cast<PropId>(props_.size());
  props_.push_back(std::move(p));
  keys_.push_back(key);
  propIndex_.emplace(std::move(key), id);
  journal_.push_back({Undo::PropCreated, kNoObject, {}, true, id});
  return id;
}

PropId World::atom(Term subject, Term column, bool positive) {
  // Ground terms resolve to an object right away, so an atomic proposition has exactly one
  // shape. `(a, b) ∈ R` cannot be written two ways that give two different PropIds pointing
  // at the same cell.
  // A term with a variable in it keeps its tuple shape, written out as a tuple term even
  // when it arrives as an open tuple object, so it too has exactly one shape.
  std::function<Term(const Term&)> spread = [&](const Term& t) -> Term {
    if (ground(t)) return Term::of(resolve(t));
    if (t.kind == TermKind::Obj) {
      const Object& o = objects_[t.obj];
      if (!o.open || o.kind != Kind::Tuple) return t;
      std::vector<Term> elems;
      for (ObjectId e : o.elems) elems.push_back(spread(Term::of(e)));
      return Term::ofTuple(std::move(elems));
    }
    std::vector<Term> elems;
    for (const Term& e : t.elems) elems.push_back(spread(e));
    return Term::ofTuple(std::move(elems));
  };
  auto normalize = [&](Term t) { return spread(t); };
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

PropId World::forAll(ObjectId var, PropId body) {
  Prop p;
  p.kind = PropKind::ForAll;
  p.left = body;
  p.bound = var;
  return intern(std::move(p));
}

PropId World::exists(ObjectId var, PropId body) {
  Prop p;
  p.kind = PropKind::Exists;
  p.left = body;
  p.bound = var;
  return intern(std::move(p));
}

bool World::ground(const Term& t) const {
  switch (t.kind) {
    case TermKind::Obj: return !objects_[t.obj].open;
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
    case TermKind::Tuple: {
      std::vector<ObjectId> ids;
      ids.reserve(t.elems.size());
      for (const Term& e : t.elems) ids.push_back(resolve(e));
      return tuple(ids);
    }
  }
  throw std::runtime_error("malformed term");
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
  std::function<std::string(PropId)> go = [&](PropId cur) -> std::string {
    const Prop& p = props_[cur];
    std::function<std::string(const Term&)> term = [&](const Term& t) -> std::string {
      switch (t.kind) {
        case TermKind::Obj: return show(t.obj);
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
        const std::string& name = objects_[p.bound].name;
        std::string head = p.kind == PropKind::ForAll ? "for every " + name + ", "
                                                      : "there exists " + name + " such that ";
        return head + go(p.left);
      }
    }
    return "?";
  };
  return go(id);
}


// ---------------------------------------------------------------- rule application

// ---------------------------------------------------------------- function application

ObjectId World::appliedValue(ObjectId fn, ObjectId arg) const {
  auto it = values_.find({fn, arg});
  return it == values_.end() ? kNoObject : it->second;
}

bool World::appliedFrom(ObjectId value, ObjectId& fn, ObjectId& arg) const {
  auto it = madeFrom_.find(value);
  if (it == madeFrom_.end()) return false;
  fn = it->second.first;
  arg = it->second.second;
  return true;
}

void World::stipulateValue(ObjectId fn, ObjectId arg, ObjectId value, int line) {
  values_[{fn, arg}] = value;
  journal_.push_back({Undo::ValueMade, value, {fn, arg}, true});
  tellIn(tuple({arg, value}), fn, Reason::stipulate(line));
}

ObjectId World::makeValue(ObjectId fn, ObjectId arg, bool open) {
  ObjectId value = declare(show(fn) + "(" + show(arg) + ")");
  objects_[value].open = open;
  values_[{fn, arg}] = value;
  madeFrom_[value] = {fn, arg};
  journal_.push_back({Undo::ValueMade, value, {fn, arg}, true});
  return value;
}

bool World::established(ObjectId subject, ObjectId column, const std::vector<PropId>* context) {
  if (toldIn(subject, column)) return true;
  if (!context) return false;
  for (PropId a : *context)
    for (PropId q : premisesOf(a)) {
      const Prop& p = props_[q];
      if (p.kind != PropKind::Atom || !p.positive) continue;
      Term s = p.subject, c = p.column;
      if (resolve(s) == subject && resolve(c) == column) return true;
    }
  return false;
}

ObjectId World::apply(ObjectId fn, ObjectId arg, int line, bool establish,
                      const std::vector<PropId>* context) {
  bool open = objects_[fn].open || objects_[arg].open;
  if (open && context) {
    // Written in a rule: the same domain check as for F(a), against what is assumed there.
    // Domain(F) for a concrete F is the value already made; for a variable f it is Domain(f),
    // checked in turn.
    if (domain_ == kNoObject) throw Unapplied("`Domain` is not defined", kNoProp);
    ObjectId dom = objects_[fn].open ? apply(domain_, fn, line, false, context)
                                     : appliedValue(domain_, fn);
    if (dom == kNoObject)
      throw Unapplied("Domain(" + show(fn) + ") does not exist yet — needs `Apply Domain to " +
                          show(fn) + ".` first",
                      kNoProp);
    if (!established(arg, dom, context))
      throw Unapplied("not established: `" + show(arg) + " in " + show(dom) + "`",
                      atom(Term::of(arg), Term::of(dom), true));
  }

  ObjectId have = appliedValue(fn, arg);
  if (have != kNoObject && (!establish || toldIn(tuple({arg, have}), fn))) return have;

  // F(x) with x a variable is an object too: the one a rule speaks of. Nothing is stated
  // about it; putting an object in place of x turns it into that object's value.
  if (open) return makeValue(fn, arg, true);

  // Function application is not a mechanism operation. In meaning it is a domain lookup
  // followed by taking a witness; here it is done directly, with the domain check being
  // an ordinary cell lookup. The domain is itself the result of another application.
  if (domain_ == kNoObject) throw Unapplied("`Domain` is not defined", kNoProp);
  ObjectId dom = appliedValue(domain_, fn);
  if (dom == kNoObject)
    throw Unapplied("Domain(" + show(fn) + ") does not exist yet — needs `Apply Domain to " +
                        show(fn) + ".` first",
                    kNoProp);
  if (!toldIn(arg, dom))
    throw Unapplied("not established: `" + show(arg) + " in " + show(dom) + "`",
                    atom(Term::of(arg), Term::of(dom), true));

  ObjectId value = have != kNoObject ? have : makeValue(fn, arg, false);
  tellIn(tuple({arg, value}), fn, Reason::derive("application", line));
  return value;
}


// ---------------------------------------------------------------- rule application

// Whether `x` occurs in `o`: is it, is a part of it, or is what it was applied to.
bool World::mentions(ObjectId o, ObjectId x) const {
  if (o == x) return true;
  const Object& ob = objects_[o];
  if (objects_[x].variable && !ob.open) return false;
  for (ObjectId e : ob.elems)
    if (mentions(e, x)) return true;
  auto m = madeFrom_.find(o);
  return m != madeFrom_.end() && (mentions(m->second.first, x) || mentions(m->second.second, x));
}

// `top` is a whole term of an atom. There a closed tuple is left as it is; inside F(...) the
// argument is rebuilt, so F((x, b)) follows x.
ObjectId World::substObj(ObjectId o, ObjectId from, ObjectId to, bool top, int line, bool app) {
  if (o == from) return to;
  if (!mentions(o, from)) return o;
  ObjectId fn, arg;
  if (appliedFrom(o, fn, arg)) {
    fn = substObj(fn, from, to, false, line, app);
    arg = substObj(arg, from, to, false, line, app);
    if (app || objects_[fn].open || objects_[arg].open) return apply(fn, arg, line);
    ObjectId have = appliedValue(fn, arg);
    return have != kNoObject ? have : makeValue(fn, arg, false);
  }
  const Object& ob = objects_[o];
  if (ob.kind == Kind::Tuple && (ob.open || !top)) {
    std::vector<ObjectId> elems = ob.elems;
    for (ObjectId& e : elems) e = substObj(e, from, to, false, line, app);
    return tuple(elems);
  }
  return o;
}

Term World::substTerm(const Term& t, ObjectId from, ObjectId to, int line, bool app) {
  switch (t.kind) {
    case TermKind::Obj:
      return Term::of(substObj(t.obj, from, to, true, line, app));
    case TermKind::Tuple: {
      std::vector<Term> out;
      out.reserve(t.elems.size());
      for (const Term& e : t.elems) out.push_back(substTerm(e, from, to, line, app));
      return Term::ofTuple(std::move(out));
    }
  }
  return t;
}

PropId World::substitute(PropId id, ObjectId from, ObjectId to, int line, bool app) {
  {
    const Prop& p = props_[id];
    if (objects_[from].variable &&
        !std::binary_search(p.freeVars.begin(), p.freeVars.end(), from))
      return id;  // `from` does not occur free in it
  }
  // Copied first: building new propositions while holding a reference is avoided.
  Prop p = props_[id];
  auto sub = [&](PropId q) { return substitute(q, from, to, line, app); };
  switch (p.kind) {
    case PropKind::Atom:
      return atom(substTerm(p.subject, from, to, line, app),
                  substTerm(p.column, from, to, line, app), p.positive);
    case PropKind::And:     return conj(sub(p.left), sub(p.right));
    case PropKind::Or:      return disj(sub(p.left), sub(p.right));
    case PropKind::Implies: return implies(sub(p.left), sub(p.right));
    case PropKind::Iff:     return iff(sub(p.left), sub(p.right));
    case PropKind::Not:     return neg(sub(p.left));
    case PropKind::ForAll:
      if (p.bound == from) return id;
      return forAll(p.bound, sub(p.left));
    case PropKind::Exists:
      if (p.bound == from) return id;
      return exists(p.bound, sub(p.left));
  }
  throw std::runtime_error("malformed proposition");
}

PropId World::instantiate(PropId binderProp, ObjectId with, int line, bool app) {
  const Prop& b = props_[binderProp];
  if (b.kind != PropKind::ForAll && b.kind != PropKind::Exists)
    throw std::runtime_error("this proposition has no bound variable to substitute");
  ObjectId var = b.bound;
  PropId body = b.left;
  return substitute(body, var, with, line, app);
}

PropId World::fillHoles(PropId templateProp, const std::vector<ObjectId>& holes,
                        const std::vector<Term>& args) {
  if (args.size() < holes.size()) throw std::runtime_error("template is missing an argument");
  // A hole is a variable of the template, so filling it is putting an object in its place —
  // the same substitution as applying a rule, without making F(a) for a concrete a.
  PropId out = templateProp;
  for (size_t i = 0; i < holes.size(); ++i)
    out = substitute(out, holes[i], resolve(args[i]), 0, false);
  return out;
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
  try {
    for (ObjectId a : args) {
      const Prop& p = props_[body];
      if (p.kind != PropKind::ForAll)
        throw std::runtime_error("rule applied to more arguments than it has variables");
      binding.emplace_back(objects_[p.bound].name, a);
      body = instantiate(body, a, line);
    }
  } catch (const Unapplied& e) {
    // F(a) has no value because `a ∈ Domain(F)` is not there: a premise of the step is
    // missing, like any other.
    if (e.missing == kNoProp) throw;
    res.missing = e.missing;
    return res;
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
      return res;  // the step does not go through; its conclusion is not written
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

World::PropTree World::snapshot(PropId id, ObjectId from) const {
  std::map<ObjectId, Remade> remade;
  std::function<void(ObjectId)> keep = [&](ObjectId o) {
    if (o < from || remade.count(o)) return;
    const Object& ob = objects_[o];
    Remade d;
    d.kind = ob.kind;
    d.name = ob.name;
    d.variable = ob.variable;
    d.elems = ob.elems;
    d.value = ob.value;
    auto m = madeFrom_.find(o);
    if (m != madeFrom_.end()) {
      d.fn = m->second.first;
      d.arg = m->second.second;
    }
    // A declared object made in the scope, other than a variable or F(a), cannot be made
    // again: it is left as it is.
    if (ob.kind == Kind::Declared && !ob.variable && d.fn == kNoObject) return;
    remade[o] = d;
    for (ObjectId e : d.elems) keep(e);
    if (d.fn != kNoObject) {
      keep(d.fn);
      keep(d.arg);
    }
  };
  std::function<void(const Term&)> keepTerm = [&](const Term& t) {
    if (t.kind == TermKind::Obj) keep(t.obj);
    for (const Term& e : t.elems) keepTerm(e);
  };
  std::function<PropTree(PropId)> copy = [&](PropId cur) {
    const Prop& p = props_[cur];
    PropTree t;
    t.kind = p.kind;
    t.subject = p.subject;
    t.column = p.column;
    t.positive = p.positive;
    t.bound = p.bound;
    if (p.kind == PropKind::Atom) {
      keepTerm(p.subject);
      keepTerm(p.column);
    }
    if (p.bound != kNoObject) keep(p.bound);
    if (p.left != kNoProp) t.kids.push_back(copy(p.left));
    if (p.right != kNoProp) t.kids.push_back(copy(p.right));
    return t;
  };
  PropTree root = copy(id);
  root.remade = std::move(remade);
  return root;
}

PropId World::rebuild(const PropTree& root) {
  // Each object made inside the scope is made again, once, the way it was made.
  std::map<ObjectId, ObjectId> made;
  std::function<ObjectId(ObjectId)> again = [&](ObjectId o) -> ObjectId {
    auto d = root.remade.find(o);
    if (d == root.remade.end()) return o;
    auto done = made.find(o);
    if (done != made.end()) return done->second;
    const Remade& how = d->second;
    ObjectId out;
    if (how.variable) {
      out = variable(how.name);
    } else if (how.fn != kNoObject) {
      // F(a) made in the scope, where `a ∈ Domain(F)` may have held only by assumption: the
      // object is made again so the copy can name it, and `(a, F(a)) ∈ F` is not — it was
      // part of what the scope had.
      ObjectId fn = again(how.fn), arg = again(how.arg);
      out = appliedValue(fn, arg);
      if (out == kNoObject) out = makeValue(fn, arg, objects_[fn].open || objects_[arg].open);
    } else if (how.kind == Kind::Numeral) {
      out = numeral(how.value);
    } else if (how.kind == Kind::Text) {
      out = text(how.name);
    } else {
      std::vector<ObjectId> elems = how.elems;
      for (ObjectId& e : elems) e = again(e);
      out = tuple(elems);
    }
    made[o] = out;
    return out;
  };
  std::function<Term(const Term&)> term = [&](const Term& t) -> Term {
    if (t.kind == TermKind::Obj) return Term::of(again(t.obj));
    if (t.kind != TermKind::Tuple) return t;
    std::vector<Term> elems;
    for (const Term& e : t.elems) elems.push_back(term(e));
    return Term::ofTuple(std::move(elems));
  };
  std::function<PropId(const PropTree&)> go = [&](const PropTree& t) -> PropId {
    switch (t.kind) {
      case PropKind::Atom:    return atom(term(t.subject), term(t.column), t.positive);
      case PropKind::And:     return conj(go(t.kids[0]), go(t.kids[1]));
      case PropKind::Or:      return disj(go(t.kids[0]), go(t.kids[1]));
      case PropKind::Implies: return implies(go(t.kids[0]), go(t.kids[1]));
      case PropKind::Iff:     return iff(go(t.kids[0]), go(t.kids[1]));
      case PropKind::Not:     return neg(go(t.kids[0]));
      case PropKind::ForAll: {
        ObjectId var = again(t.bound);
        return forAll(var, go(t.kids[0]));
      }
      case PropKind::Exists: {
        ObjectId var = again(t.bound);
        return exists(var, go(t.kids[0]));
      }
    }
    throw std::runtime_error("malformed proposition");
  };
  return go(root);
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

// Inside a representative a variable is written `(Var, k)`, k its position among the binders
// around it, innermost 0. Positions rather than the variable objects: two propositions
// differing only in which variable objects they bind are the same proposition, so their
// representatives must coincide.
ObjectId World::representObj(ObjectId o, const std::vector<ObjectId>& ctx) {
  const Object& ob = objects_[o];
  if (!ob.open) return o;
  if (ob.variable) {
    auto at = std::find(ctx.begin(), ctx.end(), o);
    if (at == ctx.end()) return o;
    return tuple({tag("Var"), numeral(Rational(static_cast<long long>(at - ctx.begin())))});
  }
  if (ob.kind == Kind::Tuple) {
    std::vector<ObjectId> ids = ob.elems;
    for (ObjectId& e : ids) e = representObj(e, ctx);
    return tuple(ids);
  }
  // F(x): `(App, F, x)`, with x written as above, so the representative is closed.
  ObjectId fn, arg;
  if (appliedFrom(o, fn, arg)) return tuple({tag("App"), representObj(fn, ctx), representObj(arg, ctx)});
  return o;
}

ObjectId World::representTerm(const Term& t, const std::vector<ObjectId>& ctx) {
  switch (t.kind) {
    case TermKind::Obj:
      return representObj(t.obj, ctx);
    case TermKind::Tuple: {
      std::vector<ObjectId> ids;
      ids.reserve(t.elems.size());
      for (const Term& e : t.elems) ids.push_back(representTerm(e, ctx));
      return tuple(ids);
    }
  }
  throw std::runtime_error("malformed term");
}

ObjectId World::represent(PropId id) {
  std::vector<ObjectId> ctx;
  return represent(id, ctx);
}

ObjectId World::represent(PropId id, std::vector<ObjectId>& ctx) {
  // Only a closed proposition is paired with its representative: an open one, the body of a
  // binder, is written differently depending on the binders around it.
  bool closed = props_[id].freeVars.empty();
  if (closed) {
    auto it = repOf_.find(id);
    if (it != repOf_.end()) return it->second;
  }

  const Prop p = props_[id];
  ObjectId rep = kNoObject;
  switch (p.kind) {
    case PropKind::Atom: {
      ObjectId s = representTerm(p.subject, ctx), c = representTerm(p.column, ctx);
      rep = tuple({tag(p.positive ? "Mem" : "NotMem"), s, c});
      break;
    }
    case PropKind::And:     rep = tuple({tag("And"), represent(p.left, ctx), represent(p.right, ctx)}); break;
    case PropKind::Or:      rep = tuple({tag("Or"), represent(p.left, ctx), represent(p.right, ctx)}); break;
    case PropKind::Implies: rep = tuple({tag("Implies"), represent(p.left, ctx), represent(p.right, ctx)}); break;
    case PropKind::Iff:     rep = tuple({tag("Iff"), represent(p.left, ctx), represent(p.right, ctx)}); break;
    case PropKind::Not:     rep = tuple({tag("Not"), represent(p.left, ctx)}); break;
    case PropKind::ForAll:
    case PropKind::Exists: {
      // Structured: the body is represented too, with bound variables as `(Var, k)`. So
      // rules that look inside a quantified proposition can be written.
      ctx.insert(ctx.begin(), p.bound);
      ObjectId body = represent(p.left, ctx);
      ctx.erase(ctx.begin());
      rep = tuple({tag(p.kind == PropKind::ForAll ? "All" : "Ex"), body});
      break;
    }
  }
  if (!closed) return rep;

  repOf_[id] = rep;
  propOf_[rep] = id;
  journal_.push_back({Undo::RepCreated, rep, {}, true, id});

  // A representative created after its proposition was stored must catch up with Holds
  // immediately — and the other way round: `[A] ∈ Holds` written before [A] was paired
  // with A stores A now.
  if (holds(id)) {
    Reason why = Reason::derive("holds", 0);
    why.antecedents = {Antecedent{id}};
    tellIn(rep, holdsColumn(), std::move(why));
  } else if (toldIn(rep, holdsColumn())) {
    syncFromHolds(rep, Reason::derive("holds", 0));
  }
  return rep;
}

// Tags are only looked up here, never made: reading must not add objects that are not there.
static bool isTag(const std::map<std::string, ObjectId>& tags, ObjectId o, const char* name) {
  auto t = tags.find(name);
  return t != tags.end() && t->second == o;
}

bool World::decodeTerm(ObjectId o, std::vector<ObjectId>& ctx, std::vector<PropId>& assumed,
                       ObjectId& out) {
  const Object& ob = objects_[o];
  if (ob.kind != Kind::Tuple) {
    out = o;
    return true;
  }
  const std::vector<ObjectId> e = ob.elems;
  if (e.size() == 2 && isTag(tags_, e[0], "Var") && objects_[e[1]].kind == Kind::Numeral) {
    const Rational& k = objects_[e[1]].value;
    for (size_t i = 0; i < ctx.size(); ++i)
      if (k == Rational(static_cast<long long>(i))) {
        out = ctx[i];
        return true;
      }
  }
  if (e.size() == 3 && isTag(tags_, e[0], "App")) {
    // Only F(x) with a variable is written (App, F, x); a concrete F(a) is the value object.
    ObjectId fn, arg;
    if (!decodeTerm(e[1], ctx, assumed, fn) || !decodeTerm(e[2], ctx, assumed, arg)) return false;
    if (!objects_[fn].open && !objects_[arg].open) return false;
    try {
      out = apply(fn, arg, 0, false, &assumed);
    } catch (const Unapplied&) {
      return false;  // `x ∈ Domain(F)` is not assumed around it: not a proposition
    }
    return true;
  }
  std::vector<ObjectId> parts(e.size());
  for (size_t i = 0; i < e.size(); ++i)
    if (!decodeTerm(e[i], ctx, assumed, parts[i])) return false;
  out = tuple(parts);
  return true;
}

PropId World::decode(ObjectId rep, std::vector<ObjectId>& ctx, std::vector<PropId>& assumed) {
  auto it = propOf_.find(rep);
  if (it != propOf_.end()) return it->second;
  const Object& ob = objects_[rep];
  if (ob.kind != Kind::Tuple || ob.elems.empty()) return kNoProp;
  const std::vector<ObjectId> e = ob.elems;
  auto is = [&](const char* name) { return isTag(tags_, e[0], name); };
  if ((is("Mem") || is("NotMem")) && e.size() == 3) {
    ObjectId s, c;
    if (!decodeTerm(e[1], ctx, assumed, s) || !decodeTerm(e[2], ctx, assumed, c)) return kNoProp;
    return atom(Term::of(s), Term::of(c), is("Mem"));
  }
  if ((is("And") || is("Or") || is("Implies") || is("Iff")) && e.size() == 3) {
    PropId a = decode(e[1], ctx, assumed);
    if (a == kNoProp) return kNoProp;
    // The left side of `if … then` and of `and` is assumed while reading the right side.
    bool carries = is("And") || is("Implies");
    if (carries) assumed.push_back(a);
    PropId b = decode(e[2], ctx, assumed);
    if (carries) assumed.pop_back();
    if (b == kNoProp) return kNoProp;
    if (is("And")) return conj(a, b);
    if (is("Or")) return disj(a, b);
    if (is("Implies")) return implies(a, b);
    return iff(a, b);
  }
  if (is("Not") && e.size() == 2) {
    PropId a = decode(e[1], ctx, assumed);
    return a == kNoProp ? kNoProp : neg(a);
  }
  if ((is("All") || is("Ex")) && e.size() == 2) {
    // As in the parser: a proposition already in the store leaves its new variable unused,
    // so what was made for it is removed.
    Mark m = mark();
    size_t before = props_.size();
    ObjectId var = variable("x");
    ctx.insert(ctx.begin(), var);
    PropId body = decode(e[1], ctx, assumed);
    ctx.erase(ctx.begin());
    if (body == kNoProp) return kNoProp;
    PropId out = is("All") ? forAll(var, body) : exists(var, body);
    if (out < before) rollback(m);
    return out;
  }
  return kNoProp;
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
  if (it == propOf_.end()) {
    // Not paired yet. It may still be a representative by its shape — written as a tuple,
    // or built by a rule — and then pairing it now stores what it stands for.
    std::vector<ObjectId> ctx;
    std::vector<PropId> assumed;
    PropId p = decode(rep, ctx, assumed);
    if (p != kNoProp && props_[p].freeVars.empty()) represent(p);  // catches up if it is `rep`
    return;
  }
  if (holds(it->second)) return;
  Reason why = Reason::derive("holds", base.line);
  // Points back at `[A] ∈ Holds`, so the reason tree goes on to whatever put A there.
  why.antecedents = {Antecedent{atom(Term::of(rep), Term::of(holds_), true)}};
  tell(it->second, std::move(why));
}

}  // namespace v
