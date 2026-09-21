#include "v/interp.hpp"

#include "v/prelude.hpp"
#include "v/symbolic.hpp"

#include <algorithm>

namespace v {

PropId Interp::ref(const std::string& label, int line) const {
  auto it = labels_.find(label);
  if (it == labels_.end()) throw ParseError(line, "no such label (" + label + ")");
  return it->second;
}

void Interp::bind(const std::string& label, PropId p) {
  if (label.empty()) return;
  labels_[label] = p;
  if (!frames_.empty()) frames_.back().labels.push_back(label);
}

void Interp::whyChain(PropId p, int depth, Report& rep) const {
  std::string pad(depth * 2 + 2, ' ');
  const auto& rs = w_.propReasons(p);
  if (rs.empty()) {
    rep.lines.push_back(pad + "(no reason recorded)");
    return;
  }
  for (const Reason& r : rs) {
    if (r.stipulated) {
      rep.lines.push_back(pad + "stipulated at line " + std::to_string(r.line));
      continue;
    }
    std::string head = pad + "derived by " + r.rule + ", line " + std::to_string(r.line);
    if (!r.binding.empty()) {
      head += " [";
      for (size_t i = 0; i < r.binding.size(); ++i)
        head += (i ? ", " : "") + r.binding[i].first + " := " + w_.show(r.binding[i].second);
      head += "]";
    }
    rep.lines.push_back(head);
    if (depth < 6)
      for (const Antecedent& a : r.antecedents) {
        rep.lines.push_back(pad + "  because " + w_.showProp(a.prop));
        whyChain(a.prop, depth + 2, rep);
      }
  }
}

// Match a concrete term against a term with a bound variable at index `depth`.
bool Interp::matchTerm(const Term& concrete, const Term& pattern, VarId depth,
                       ObjectId& witness) const {
  if (pattern.kind == TermKind::Var) {
    if (pattern.var != depth) return concrete.kind == TermKind::Var && concrete.var == pattern.var;
    if (concrete.kind != TermKind::Obj) return false;
    if (witness != kNoObject && witness != concrete.obj) return false;  // must be the same witness
    witness = concrete.obj;
    return true;
  }
  if (concrete.kind != pattern.kind) return false;
  if (pattern.kind == TermKind::Obj) return concrete.obj == pattern.obj;
  if (concrete.elems.size() != pattern.elems.size()) return false;
  for (size_t i = 0; i < pattern.elems.size(); ++i)
    if (!matchTerm(concrete.elems[i], pattern.elems[i], depth, witness)) return false;
  return true;
}

bool Interp::matchWitness(PropId concrete, PropId pattern, VarId depth,
                          ObjectId& witness) const {
  const Prop& a = w_.prop(concrete);
  const Prop& b = w_.prop(pattern);
  if (a.kind != b.kind) return false;
  if (b.kind == PropKind::Atom)
    return a.positive == b.positive && matchTerm(a.subject, b.subject, depth, witness) &&
           matchTerm(a.column, b.column, depth, witness);
  VarId inner = (b.kind == PropKind::ForAll || b.kind == PropKind::Exists) ? depth + 1 : depth;
  if (b.left != kNoProp && !matchWitness(a.left, b.left, inner, witness)) return false;
  if (b.right != kNoProp && !matchWitness(a.right, b.right, inner, witness)) return false;
  return true;
}

Rational Interp::evalExpr(const Term& t, int line) {
  if (t.kind == TermKind::Obj) {
    const Object& o = w_.obj(t.obj);
    if (o.kind != Kind::Numeral)
      throw ParseError(line, "not a number: " + o.name);
    return o.value;
  }
  if (t.kind != TermKind::Tuple || t.elems.size() != 3 || t.elems[0].kind != TermKind::Obj)
    throw ParseError(line, "not a numeric expression");

  const std::string& op = w_.obj(t.elems[0].obj).name;
  Rational a = evalExpr(t.elems[1], line), b = evalExpr(t.elems[2], line);
  if (op == "Plus") return a + b;
  if (op == "Minus") return a - b;
  if (op == "Times") return a * b;
  if (op == "Div") return a / b;  // throws RationalDivByZero, caught by the caller
  throw ParseError(line, "unknown operation: " + op);
}

void Interp::exec(const Stmt& s, Report& rep) {
  switch (s.kind) {
    case StmtKind::Let: {
      if (s.isTupleLet) {
        std::vector<Term> elems;
        for (const std::string& e : s.tupleElems) {
          auto it = names_.find(e);
          if (it == names_.end()) throw ParseError(s.line, "not declared: " + e);
          elems.push_back(Term::of(it->second));
        }
        ObjectId t = w_.declare(s.names[0]);
        names_[s.names[0]] = t;
        ObjectId built = w_.resolve(Term::ofTuple(elems));
        // `Let t = (a, b)` writes into a mechanism relation, not into Eq.
        w_.tellIn(w_.tuple({t, built}), w_.tag("Defined"), Reason::stipulate(s.line));
      } else {
        // `be a set` / `be a relation` is content, not a mechanism type: it records a fact
        // in SET or RELATION, like any other fact.
        ObjectId column = kNoObject;
        if (s.letKind == "set") column = names_["SET"];
        else if (s.letKind == "relation") column = names_["RELATION"];
        else if (s.letKind == "map") column = names_["MAP"];
        for (const std::string& n : s.names) {
          ObjectId o = w_.declare(n);
          names_[n] = o;
          if (column != kNoObject)
            w_.tellIn(o, column, Reason::stipulate(s.line));
        }
      }
      return;
    }

    case StmtKind::Assume:
    case StmtKind::Rule: {
      pv_.assume(s.prop, s.line);
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::ByRule: {
      PropId rule = ref(s.refs[0], s.line);
      std::vector<ObjectId> args;
      for (const Term& a : s.args) args.push_back(w_.resolve(a));
      auto r = w_.applyRule(rule, args, s.refs[0], s.line);
      if (!r.ok)
        throw ParseError(s.line, "step does not go through, missing: " + w_.showProp(r.missing));
      if (r.conclusion != s.prop)
        throw ParseError(s.line, "stated conclusion does not match: the rule gives `" +
                                     w_.showProp(r.conclusion) + "`");
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::From: {
      std::vector<PropId> ps;
      for (const std::string& r : s.refs) ps.push_back(ref(r, s.line));
      const Prop& out = w_.prop(s.prop);
      Prover::Result res;

      if (ps.size() == 1 && w_.prop(ps[0]).kind == PropKind::And &&
          (w_.prop(ps[0]).left == s.prop || w_.prop(ps[0]).right == s.prop)) {
        res = pv_.elimAnd(ps[0], w_.prop(ps[0]).left == s.prop, s.line);
      } else if (ps.size() == 1 && w_.prop(ps[0]).kind == PropKind::Iff &&
                 out.kind == PropKind::Implies) {
        res = pv_.elimIff(ps[0], w_.prop(ps[0]).left == out.left, s.line);
      } else if (out.kind == PropKind::And && ps.size() == 2) {
        res = pv_.introAnd(ps[0], ps[1], s.line);
      } else if (out.kind == PropKind::Or && ps.size() == 1) {
        bool left = out.left == ps[0];
        res = pv_.introOr(ps[0], left ? out.right : out.left, left, s.line);
      } else if (out.kind == PropKind::Iff && ps.size() == 2) {
        res = pv_.introIff(ps[0], ps[1], s.line);
      } else if (out.kind == PropKind::Exists && ps.size() == 1) {
        // The witness is read off by one pass of matching the premise against the body of
        // the conclusion: wherever the body has the bound variable the premise has an
        // object, and all those places must hold the same object. Lookup, not trial.
        ObjectId witness = kNoObject;
        if (!matchWitness(ps[0], out.left, 0, witness) || witness == kNoObject)
          throw ParseError(s.line, "cannot read a witness off the premise");
        res = pv_.introExists(ps[0], witness, out.binderName, s.line);
      } else {
        throw ParseError(s.line, "unrecognised step");
      }

      if (!res.ok)
        throw ParseError(s.line, res.missing != kNoProp
                                     ? "missing: " + w_.showProp(res.missing)
                                     : res.error);
      if (res.prop != s.prop)
        throw ParseError(s.line, "conclusion does not match: the step gives `" + w_.showProp(res.prop) + "`");
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::Suppose: {
      pv_.suppose(s.prop, s.label, s.line);
      frames_.push_back({});
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::Take: {
      pv_.take(s.names[0], s.line);
      frames_.push_back({});
      names_[s.names[0]] = pv_.freshOf();
      frames_.back().names.push_back(s.names[0]);
      return;
    }

    case StmtKind::TakeIn: {
      auto it = names_.find(s.setName);
      if (it == names_.end()) throw ParseError(s.line, "not declared: " + s.setName);
      pv_.takeIn(s.names[0], it->second, s.line);
      frames_.push_back({});
      names_[s.names[0]] = pv_.freshOf();
      frames_.back().names.push_back(s.names[0]);
      return;
    }

    case StmtKind::TakeFrom: {
      auto r = pv_.takeFrom(s.names[0], ref(s.refs[0], s.line), s.line);
      if (!r.ok) throw ParseError(s.line, r.error);
      frames_.push_back({});
      names_[s.names[0]] = pv_.freshOf();
      frames_.back().names.push_back(s.names[0]);
      return;
    }

    case StmtKind::Close:
      return;  // `}` only closes the body; `Hence` does the work

    case StmtKind::Hence: {
      auto r = pv_.hence(s.prop, s.label, s.line);
      if (!r.ok)
        throw ParseError(s.line, r.missing != kNoProp
                                     ? "missing: " + w_.showProp(r.missing)
                                     : r.error);
      if (!frames_.empty()) {
        for (const std::string& l : frames_.back().labels) labels_.erase(l);
        for (const std::string& n : frames_.back().names) names_.erase(n);
        frames_.pop_back();
      }
      bind(s.label, r.prop);  // the Hence label belongs to the enclosing scope
      return;
    }

    case StmtKind::Absurd: {
      std::vector<PropId> ps;
      for (const std::string& r : s.refs) ps.push_back(ref(r, s.line));
      Prover::Result r;
      if (ps.size() == 2) r = pv_.absurd(ps[0], ps[1], s.line);
      else if (ps.size() == 3) r = pv_.absurdFromOr(ps[0], ps[1], ps[2], s.line);
      else throw ParseError(s.line, "Absurd needs two or three propositions");
      if (!r.ok) throw ParseError(s.line, r.error);
      return;
    }

    case StmtKind::Apply: {
      ObjectId F = w_.resolve(s.fn), a = w_.resolve(s.arg);

      if (!s.names.empty()) {
        // The `as` form: stipulates the canonical value without checking the domain. This
        // is where the chain is cut — `Apply Domain to Domain as MAP.` is of the same kind
        // as `SET ∈ SET`.
        auto it = names_.find(s.names[0]);
        if (it == names_.end()) throw ParseError(s.line, "not declared: " + s.names[0]);
        applied_[{F, a}] = it->second;
        w_.tellIn(w_.tuple({a, it->second}), F, Reason::stipulate(s.line));
        return;
      }

      // Function application is not a mechanism operation. In meaning it is a domain lookup
      // followed by taking a witness; here it is done directly, with the domain check being
      // an ordinary cell lookup. The domain is itself the result of another application.
      // `Domain` is a name declared by the prelude, not a mechanism tag.
      auto domIt = names_.find("Domain");
      if (domIt == names_.end()) throw ParseError(s.line, "`Domain` is not defined");
      auto d = applied_.find({domIt->second, F});
      if (d == applied_.end())
        throw ParseError(s.line, "Domain(" + w_.show(F) + ") does not exist yet — needs `Apply Domain to " +
                                     w_.show(F) + ".` first");
      if (!w_.toldIn(a, d->second))
        throw ParseError(s.line, "not established: `" + w_.show(a) + " in " + w_.show(d->second) + "`");

      auto have = applied_.find({F, a});
      if (have != applied_.end()) return;  // canonical value already exists

      ObjectId value = w_.declare(w_.show(F) + "(" + w_.show(a) + ")");
      applied_[{F, a}] = value;
      Reason why = Reason::derive("application", s.line);
      w_.tellIn(w_.tuple({a, value}), F, std::move(why));
      rep.lines.push_back("  appl line " + std::to_string(s.line) + ": " + w_.show(value));
      return;
    }

    case StmtKind::Theory: {
      theories_[s.names[0]] = s;  // record only, do not run
      return;
    }

    case StmtKind::Import: {
      auto it = theories_.find(s.names[0]);
      if (it == theories_.end()) throw ParseError(s.line, "no such theory: " + s.names[0]);
      const Stmt& th = it->second;

      // Instance identity: theory name plus mapping. Re-importing is a no-op.
      std::string key = s.names[0];
      for (const auto& m : th.mapping) (void)m;
      for (const auto& m : s.mapping) key += "|" + m.first + "=" + m.second;
      if (!imported_.insert(key).second) return;

      std::map<std::string, std::string> rename;
      for (const auto& m : s.mapping) rename[m.first] = m.second;

      // Names declared inside and left unmapped are renamed to names private to the
      // instance, so two imports do not step on each other.
      for (size_t i = 0; i + 2 < th.body.size(); ++i) {
        if (th.body[i].first != "Let") continue;
        size_t j = i + 1;
        while (j < th.body.size() && th.body[j].first != "be" && th.body[j].first != "=") {
          if (th.bodyWord[j] && !rename.count(th.body[j].first))
            rename[th.body[j].first] = s.alias + "_" + th.body[j].first;
          ++j;
        }
      }

      // Labels come right after label-opening words; they also carry the instance name.
      auto opensLabel = [](const std::string& w) {
        return w == "Rule" || w == "Assume" || w == "Suppose" || w == "Hence" ||
               w == "rule" || w == "from" || w == "From";
      };

      // Nested theories: the names of child instances and of theories declared inside must
      // carry the outer instance name too, or importing the same theory twice would make
      // them collide.
      auto nestedName = [&](size_t i) {
        if (i >= 1 && th.body[i - 1].first == "Theory") return true;
        if (i >= 3 && th.body[i - 1].first == "as" && th.body[i - 3].first == "Import")
          return true;
        return false;
      };

      std::vector<Parser::Token> out;
      for (size_t i = 0; i < th.body.size(); ++i) {
        // Keep the original line numbers of the theory body, so errors inside it point at
        // the right place.
        Parser::Token t{th.body[i].first, th.body[i].second, th.bodyWord[i], th.bodyNumber[i]};
        if (t.word) {
          auto r = rename.find(t.text);
          if (r != rename.end()) t.text = r->second;
          else if (nestedName(i)) t.text = s.alias + " " + t.text;
          else if (i >= 2 && th.body[i - 1].first == "(" && opensLabel(th.body[i - 2].first))
            t.text = s.alias + " " + t.text;
        }
        out.push_back(std::move(t));
      }

      Parser replay(w_, names_, applied_, labels_);
      replay.beginTokens(std::move(out));
      while (replay.more()) exec(replay.next(), rep);
      return;
    }

    case StmtKind::Instantiate: {
      PropId all = ref(s.refs[0], s.line);
      const Prop& p = w_.prop(all);
      if (p.kind != PropKind::ForAll && p.kind != PropKind::Exists)
        throw ParseError(s.line, "needs a quantified proposition");
      ObjectId t = w_.resolve(s.expr);

      // Substitution on representatives is not reimplemented: it decodes, uses the very
      // `instantiate` that rule application uses, and encodes again. So the two
      // substitutions cannot drift apart.
      PropId inst = w_.instantiate(all, t);
      w_.tellIn(w_.tuple({w_.represent(all), t, w_.represent(inst)}), w_.tag("Instance"),
                Reason::derive("instance", s.line));
      bind(s.label, inst);
      rep.lines.push_back("  inst line " + std::to_string(s.line) + ": " + w_.showProp(all) +
                          "  at " + w_.show(t) + "  ->  " + w_.showProp(inst));
      return;
    }

    case StmtKind::Expand: {
      if (s.expr.kind != TermKind::Tuple)
        throw ParseError(s.line, "Expand needs a listed sequence");
      ObjectId listed = w_.resolve(s.expr);

      // The canonical form of a sequence is the function form: a set of (index, element)
      // pairs. The listed form is not dropped — both objects exist, and this step is where
      // they are joined, explicitly.
      std::string name = s.names.empty() ? "fn" + w_.show(listed) : s.names[0];
      ObjectId fn = w_.declare(name);
      if (!s.names.empty()) names_[s.names[0]] = fn;
      for (size_t i = 0; i < s.expr.elems.size(); ++i) {
        ObjectId idx = w_.numeral(Rational(static_cast<long long>(i + 1)));
        ObjectId elem = w_.resolve(s.expr.elems[i]);
        w_.tellIn(w_.tuple({idx, elem}), fn, Reason::derive("expansion", s.line));
      }
      w_.tellIn(w_.tuple({listed, fn}), w_.tag("Expanded"), Reason::derive("expansion", s.line));
      rep.lines.push_back("  expd line " + std::to_string(s.line) + ": " + w_.show(listed) +
                          "  ->  " + name);
      return;
    }

    case StmtKind::Simplify: {
      ObjectId expr = w_.resolve(s.expr);
      std::vector<Term> nonzero;
      ObjectId result;
      try {
        result = w_.resolve(simplify(w_, s.expr, &nonzero));
      } catch (const std::exception& e) {
        throw ParseError(s.line, std::string("cannot simplify: ") + e.what());
      }

      std::string shown = "  simp line " + std::to_string(s.line) + ": " + w_.show(expr) +
                          "  ->  " + w_.show(result);
      if (nonzero.empty()) {
        // Nothing containing a variable was cancelled, so the equality carries no
        // condition.
        w_.tellIn(w_.tuple({expr, result}), w_.tag("Simplified"),
                  Reason::derive("algebra", s.line));
      } else {
        // Something was cancelled: the condition goes outside as a proposition that has to
        // be established, instead of hiding inside the equality.
        ObjectId Eq = names_["Eq"], zero = w_.numeral(Rational(0));
        PropId cond = kNoProp;
        for (const Term& t : nonzero) {
          PropId one = w_.atom(Term::ofTuple({t, Term::of(zero)}), Term::of(Eq), false);
          cond = cond == kNoProp ? one : w_.conj(cond, one);
        }
        w_.tellIn(w_.tuple({expr, result, w_.represent(cond)}), w_.tag("SimplifiedIf"),
                  Reason::derive("algebra", s.line));
        shown += "   provided  " + w_.showProp(cond);
        // Lets the condition be referred to by label instead of copied by hand.
        bind(s.label, cond);
      }
      rep.lines.push_back(shown);
      return;
    }

    case StmtKind::Compute: {
      if (!s.cmpOp.empty()) {
        // The machine decides the comparison and records both directions: true raises the ∈
        // flag, false raises the ∉ flag. The reason says it was machine-computed.
        Rational a = evalExpr(s.expr, s.line), b = evalExpr(s.rhs, s.line);
        bool flip = s.cmpOp[0] == '>';
        bool strict = s.cmpOp.size() == 1;
        const Rational& lo = flip ? b : a;
        const Rational& hi = flip ? a : b;
        bool truth = strict ? lo < hi : lo <= hi;
        ObjectId pair = w_.tuple({w_.numeral(lo), w_.numeral(hi)});
        ObjectId rel = w_.tag(strict ? "Less" : "LessEq");
        Reason why = Reason::derive("machine-computed", s.line);
        if (truth) w_.tellIn(pair, rel, std::move(why));
        else w_.tellOut(pair, rel, std::move(why));
        rep.lines.push_back("  calc line " + std::to_string(s.line) + ": " + w_.show(pair) +
                            (truth ? " ∈ " : " ∉ ") + w_.show(rel));
        return;
      }
      ObjectId expr = w_.resolve(s.expr);
      try {
        Rational value = evalExpr(s.expr, s.line);
        // The machine always returns an error bound. For the four operations on rationals
        // the error is 0.
        ObjectId result = w_.numeral(value);
        ObjectId error = w_.numeral(Rational(0));
        w_.tellIn(w_.tuple({expr, result, error}), w_.tag("Computed"),
                  Reason::derive("machine-computed", s.line));
        rep.lines.push_back("  calc line " + std::to_string(s.line) + ": " + w_.show(expr) +
                            " = " + w_.show(result) + " (error " + w_.show(error) + ")");
      } catch (const RationalDivByZero&) {
        // No error, no conventional value: record a fact in a mechanism relation.
        w_.tellIn(expr, w_.tag("NoValue"), Reason::derive("division by zero", s.line));
        rep.lines.push_back("  calc line " + std::to_string(s.line) + ": " + w_.show(expr) +
                            " has no value");
      }
      return;
    }

    case StmtKind::Therefore: {
      ++rep.checksRun;
      bool ok = w_.holds(s.prop);
      if (!ok) ++rep.checksFailed;
      rep.lines.push_back(std::string(ok ? "  ok   " : "  FAIL ") + "line " +
                          std::to_string(s.line) + ": " + w_.showProp(s.prop));
      return;
    }

    case StmtKind::Why: {
      rep.lines.push_back("  why " + w_.showProp(s.prop) + ":");
      whyChain(s.prop, 0, rep);
      return;
    }
  }
}

Interp::Report Interp::run(const std::string& source) {
  Report rep;
  // Mechanism tags are usable as ordinary names, so rules over representatives can be
  // written.
  for (const char* t : {"Mem", "NotMem", "And", "Or", "Implies", "Iff", "Not", "All", "Ex",
                        "Defined", "Computed", "NoValue",
                        "Plus", "Minus", "Times", "Div", "Less", "LessEq", "Pow", "Simplified", "SimplifiedIf", "Expanded", "Var", "Instance"})
    names_[t] = w_.tag(t);
  names_["Holds"] = w_.holdsColumn();
  names_["Column"] = w_.columnTag();

  // The minimal library is content written in V, not C++ code. It is loaded by the same
  // machinery that runs the user's file.
  if (!preludeLoaded_) {
    preludeLoaded_ = true;
    Report ignored;
    Parser pre(w_, names_, applied_, labels_);
    pre.begin(kPrelude);
    while (pre.more()) exec(pre.next(), ignored);
  }
  parser_.begin(source);
  while (parser_.more()) exec(parser_.next(), rep);
  return rep;
}

}  // namespace v
