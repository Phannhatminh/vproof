#include "v/parser.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>

namespace v {

namespace {

bool isWordChar(unsigned char c) { return std::isalnum(c) || c == '_' || c == '\''; }

// Multi-byte characters accepted in both their unicode and ascii forms.
const char* kIn = "∈";     // ∈
const char* kNotIn = "∉";  // ∉

}  // namespace

void Parser::lex(const std::string& src) {
  int line = 1;
  size_t i = 0;
  while (i < src.size()) {
    char c = src[i];
    if (c == '\n') { ++line; ++i; continue; }
    if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
    if (c == '-' && i + 1 < src.size() && src[i + 1] == '-') {  // comment to end of line
      while (i < src.size() && src[i] != '\n') ++i;
      continue;
    }
    if (src.compare(i, 3, kIn) == 0) { toks_.push_back({"in", line, true, false}); i += 3; continue; }
    if (src.compare(i, 3, kNotIn) == 0) { toks_.push_back({"notin", line, true, false}); i += 3; continue; }
    // `-` belongs to the number only when it does not follow a finished term — otherwise `a
    // - 3` would lex as `a` and `-3`.
    bool afterTerm = !toks_.empty() && (toks_.back().word || toks_.back().number ||
                                        toks_.back().text == ")" || toks_.back().text == "]");
    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '-' && !afterTerm && i + 1 < src.size() &&
         std::isdigit(static_cast<unsigned char>(src[i + 1])))) {
      // `.` and `/` belong to the number only when a digit follows — otherwise the full
      // stop ending `Compute 0.2.` would be swallowed.
      size_t j = i + 1;
      while (j < src.size()) {
        unsigned char ch = static_cast<unsigned char>(src[j]);
        if (std::isdigit(ch)) { ++j; continue; }
        if ((ch == '.' || ch == '/') && j + 1 < src.size() &&
            std::isdigit(static_cast<unsigned char>(src[j + 1]))) { j += 2; continue; }
        break;
      }
      toks_.push_back({src.substr(i, j - i), line, false, true});
      i = j;
      continue;
    }
    if (c == '"') {
      size_t j = i + 1;
      while (j < src.size() && src[j] != '"') ++j;
      toks_.push_back({"\"" + src.substr(i + 1, j - i - 1), line, false, false});
      i = j < src.size() ? j + 1 : j;
      continue;
    }
    if (isWordChar(static_cast<unsigned char>(c))) {
      size_t j = i;
      while (j < src.size() && isWordChar(static_cast<unsigned char>(src[j]))) ++j;
      toks_.push_back({src.substr(i, j - i), line, true, false});
      i = j;
      continue;
    }
    if ((c == '<' || c == '>') && i + 1 < src.size() && src[i + 1] == '=') {
      toks_.push_back({src.substr(i, 2), line, false, false});
      i += 2;
      continue;
    }
    toks_.push_back({std::string(1, c), line, false, false});
    ++i;
  }
  toks_.push_back({"", line, false, false});  // sentinel
}

const Parser::Token& Parser::peek(size_t ahead) const {
  size_t k = pos_ + ahead;
  return k < toks_.size() ? toks_[k] : toks_.back();
}

bool Parser::at(const std::string& t) const { return peek().text == t; }
bool Parser::atWord() const { return peek().word; }

Parser::Token Parser::take() {
  Token t = peek();
  if (pos_ < toks_.size() - 1) ++pos_;
  return t;
}

void Parser::expect(const std::string& t) {
  if (!at(t)) err("expected `" + t + "`, got `" + peek().text + "`");
  take();
}

bool Parser::accept(const std::string& t) {
  if (!at(t)) return false;
  take();
  return true;
}

void Parser::err(const std::string& msg) const { throw ParseError(peek().line, msg); }

ObjectId Parser::lookup(const std::string& name) {
  auto it = names_.find(name);
  if (it == names_.end()) err("not declared: " + name);
  return it->second;
}

ObjectId Parser::lookupObject(const std::string& name) {
  for (const auto& [n, var] : binders_)
    if (n == name) return var;
  return lookup(name);
}

// ------------------------------------------------------------------- term

// `2 + 3` is a term of its own: the tuple (Plus, 2, 3). It is a different object from `5`,
// and connecting the two is the job of a `Compute` step, not of the parser.
Term Parser::term() {
  // Leading minus. In front of a number it belongs to the number, so that `-1` written in
  // source and `-1` produced by the algebra layer are the same object.
  if (at("-")) {
    take();
    if (peek().number) {
      Token n = take();
      Term neg = Term::of(w_.numeral(Rational("-" + n.text)));
      Term left = neg;
      while (at("+") || at("-")) {
        std::string op = take().text;
        Term right = mulLevel();
        left = Term::ofTuple({Term::of(w_.tag(op == "+" ? "Plus" : "Minus")), left, right});
      }
      return left;
    }
    Term inner = mulLevel();
    Term left = Term::ofTuple({Term::of(w_.tag("Minus")), Term::of(w_.numeral(Rational(0))), inner});
    while (at("+") || at("-")) {
      std::string op = take().text;
      Term right = mulLevel();
      left = Term::ofTuple({Term::of(w_.tag(op == "+" ? "Plus" : "Minus")), left, right});
    }
    return left;
  }
  Term left = mulLevel();
  while (at("+") || at("-")) {
    std::string op = take().text;
    Term right = mulLevel();
    left = Term::ofTuple({Term::of(w_.tag(op == "+" ? "Plus" : "Minus")), left, right});
  }
  return left;
}

Term Parser::mulLevel() {
  Term left = powLevel();
  while (at("*") || at("/")) {
    std::string op = take().text;
    Term right = powLevel();
    left = Term::ofTuple({Term::of(w_.tag(op == "*" ? "Times" : "Div")), left, right});
  }
  return left;
}

Term Parser::powLevel() {
  Term base = atomTerm();
  if (at("^")) {
    take();
    Term e = atomTerm();
    return Term::ofTuple({Term::of(w_.tag("Pow")), base, e});
  }
  return base;
}

Term Parser::atomTerm() {
  bool number = peek().number;
  Term base = atomBase();
  if (number) return base;
  // Application, after any term: `F(a)`; `F(a, b)`, whose argument is the tuple (a, b);
  // `F(a)(b)`, applying the value F(a); `t[i]`, the same as `t(i)`.
  while (at("(") || at("[")) {
    int line = peek().line;
    Term arg;
    if (accept("[")) {
      arg = term();
      expect("]");
    } else {
      take();
      std::vector<Term> args;
      args.push_back(term());
      while (accept(",")) args.push_back(term());
      expect(")");
      arg = args.size() == 1 ? args[0] : Term::ofTuple(std::move(args));
    }
    base = Term::of(applyTerm(base, arg, line));
  }
  return base;
}

// `F(a)` is function application. With a concrete `a` it points at the object created by an
// `Apply F to a.` step; until that step has run, no object has this name. With a variable in
// F or a it is the object F(x) the rule speaks of; applying the rule puts the value at the
// object in place of x.
ObjectId Parser::applyTerm(const Term& fnTerm, const Term& argTerm, int line) {
  ObjectId fn = w_.resolve(fnTerm), a = w_.resolve(argTerm);
  if (w_.obj(fn).open || w_.obj(a).open) {
    // F(x) is written only where `x ∈ Domain(F)` is assumed — the same check as for F(a).
    try {
      return w_.apply(fn, a, line, false, checkApply_ ? &assumed_ : nullptr);
    } catch (const Unapplied& e) {
      err("`" + w_.show(fn) + "(" + w_.show(a) + ")` cannot be written here — " + e.what());
    }
  }
  ObjectId value = w_.appliedValue(fn, a);
  if (value == kNoObject && makeValues_) value = w_.apply(fn, a, line);
  if (value == kNoObject)
    err("`" + w_.show(fn) + "(...)` does not exist yet — needs `Apply " + w_.show(fn) +
        " to ...` first");
  return value;
}

Term Parser::atomBase() {
  if (at("[")) {
    // `[A]` is the object representing proposition A. Naming it here is exactly what brings
    // it into existence — lazy pairing.
    //
    // `[(label)]` points at a proposition that already has a label, so it need not be
    // copied out.
    take();
    if (at("(")) {
      size_t k = pos_;
      while (k < toks_.size() && toks_[k].text != ")") ++k;
      if (k + 1 < toks_.size() && toks_[k + 1].text == "]") {
        std::string label = labelOpt();
        expect("]");
        auto it = labels_.find(label);
        if (it == labels_.end()) err("no such label (" + label + ")");
        return Term::of(w_.represent(it->second));
      }
    }
    PropId p = proposition();
    expect("]");
    return Term::of(w_.represent(p));
  }
  if (at("(")) {
    take();
    std::vector<Term> elems;
    elems.push_back(term());
    while (accept(",")) elems.push_back(term());
    expect(")");
    if (elems.size() == 1) return elems[0];  // parentheses only group
    return Term::ofTuple(std::move(elems));
  }
  if (peek().number) {
    Token t = take();
    return Term::of(w_.numeral(Rational(t.text)));
  }
  if (!peek().text.empty() && peek().text[0] == '"') {
    // `"Alice"`: a text, an object identified by its characters.
    return Term::of(w_.text(take().text.substr(1)));
  }
  if (!atWord()) err("expected a term, got `" + peek().text + "`");
  Token t = take();
  // Notation holes are looked up first — they are only open while parsing a right-hand
  // side. Then the variables of the binders around, then declared names.
  for (size_t i = 0; i < holeNames_.size(); ++i)
    if (holeNames_[i] == t.text) return Term::of(holeVars_[i]);
  return Term::of(lookupObject(t.text));
}

// -------------------------------------------------------------- propositions

PropId Parser::proposition() { return iffLevel(); }

PropId Parser::iffLevel() {
  PropId left = orLevel();
  if (accept("iff")) return w_.iff(left, iffLevel());
  return left;
}

PropId Parser::orLevel() {
  PropId left = andLevel();
  while (at("or")) {
    take();
    left = w_.disj(left, andLevel());
  }
  return left;
}

PropId Parser::andLevel() {
  PropId left = primary();
  while (at("and")) {
    take();
    // The left side is assumed while reading the right: `x in Domain(F) and F(x) in S`.
    assumed_.push_back(left);
    PropId right;
    try {
      right = primary();
    } catch (...) {
      assumed_.pop_back();
      throw;
    }
    assumed_.pop_back();
    left = w_.conj(left, right);
  }
  return left;
}

// Try to match a Notation template at the current position. Longest match wins; if none
// matches, return kNoProp and leave the cursor where it was.
PropId Parser::tryNotation() {
  size_t save = pos_;
  size_t bestEnd = 0;
  PropId best = kNoProp;
  bool ambiguous = false;

  for (const Notation& n : notations_) {
    pos_ = save;
    std::vector<Term> args;
    bool ok = true;
    for (const NotationPart& part : n.parts) {
      if (part.hole) {
        try {
          args.push_back(term());
        } catch (const ParseError&) {
          ok = false;
          break;
        }
        continue;
      }
      if (!at(part.word)) { ok = false; break; }
      take();
    }
    if (!ok || args.size() != n.holes) continue;

    // `where` guards. A concrete filler is looked up in the matrix. A variable is looked up
    // in what is assumed about it where the notation is used — the `if` part around it, or
    // the `in S` of its binder; with nothing assumed about it, the guard cannot filter.
    bool guarded = true;
    for (const auto& [i, set] : n.guards) {
      if (i >= args.size()) continue;
      if (w_.ground(args[i])) {
        if (!w_.toldIn(w_.resolve(args[i]), set)) { guarded = false; break; }
        continue;
      }
      if (args[i].kind != TermKind::Obj || !w_.obj(args[i].obj).variable) continue;
      ObjectId v = args[i].obj;
      bool known = false, in = false;
      for (PropId a : assumed_)
        for (PropId q : w_.premisesOf(a)) {
          const Prop& p = w_.prop(q);
          if (p.kind != PropKind::Atom || !p.positive || !(p.subject == Term::of(v))) continue;
          known = true;
          if (p.column == Term::of(set)) in = true;
        }
      if (known && !in) { guarded = false; break; }
    }
    if (!guarded) continue;

    if (pos_ > bestEnd) {
      bestEnd = pos_;
      best = w_.fillHoles(n.tmpl, n.holeVars, args);
      ambiguous = false;
    } else if (pos_ == bestEnd && best != kNoProp) {
      PropId other = w_.fillHoles(n.tmpl, n.holeVars, args);
      if (other != best) ambiguous = true;
    }
  }
  if (ambiguous) {
    pos_ = save;
    err("several notations match here; add `where` to tell them apart");
  }
  pos_ = best == kNoProp ? save : bestEnd;
  return best;
}

// A binder whose proposition is already in the store — the same rule written again, maybe
// with other variable names — made a variable object, F(z) with it, and parts nobody uses.
// Rolling back to before the binder removes them; everything the proposition itself
// mentions existed before, so nothing it needs goes.
PropId Parser::primary() {
  bool binder = (at("for") && peek(1).text == "every") || (at("there") && peek(1).text == "exists");
  if (!binder) return primaryInner();
  Mark m = w_.mark();
  size_t before = w_.propCount();
  PropId out = primaryInner();
  if (out < before) w_.rollback(m);
  return out;
}

PropId Parser::primaryInner() {
  if (!notations_.empty() && !at("(")) {
    PropId got = tryNotation();
    if (got != kNoProp) return got;
  }
  if (at("(")) {
    // Either a parenthesised proposition, or a tuple starting an atomic proposition. Try
    // the group first; if that fails, back up and read a term.
    size_t save = pos_;
    take();
    try {
      PropId inner = proposition();
      expect(")");
      return inner;
    } catch (const ParseError&) {
      pos_ = save;
    }
  }
  if (accept("not")) {
    // `not (A)`, or `not` before any one proposition: `not a in A`.
    return w_.neg(primary());
  }
  if (accept("if")) {
    PropId a = proposition();
    expect("then");
    assumed_.push_back(a);
    PropId b;
    try {
      b = proposition();
    } catch (...) {
      assumed_.pop_back();
      throw;
    }
    assumed_.pop_back();
    return w_.implies(a, b);
  }
  if (at("for") && peek(1).text == "every") {
    take(); take();

    // `for every x, y, z, A` and `for every x, alice in A` cannot be told apart at a
    // glance: the comma separates variables and also separates the body. So take as many as
    // possible, then back off until what remains really is `, <body>`.
    size_t save = pos_;
    size_t most = 1;
    {
      size_t k = pos_;
      while (k + 2 < toks_.size() && toks_[k + 1].text == "," && toks_[k + 2].word &&
             toks_[k + 2].text != "if" && toks_[k + 2].text != "not" &&
             toks_[k + 2].text != "for" && toks_[k + 2].text != "there") {
        ++most;
        k += 2;
      }
    }

    // `in` may be followed by any term: a declared set, a variable of a binder around, or
    // something like Domain(F). It is read before the variables of this list exist, so it
    // cannot mean one of them.
    std::vector<std::string> vars;
    Term set;
    bool hasSet = false, settled = false;
    std::unique_ptr<ParseError> lastErr;
    for (size_t n = most; n >= 1 && !settled; --n) {
      pos_ = save;
      vars.clear();
      hasSet = false;
      vars.push_back(take().text);
      for (size_t i = 1; i < n; ++i) {
        expect(",");
        vars.push_back(take().text);
      }
      if (at("in")) {
        take();
        try {
          set = term();
          hasSet = true;
        } catch (const ParseError& e) {
          lastErr = std::make_unique<ParseError>(e);
          continue;
        }
      }
      if (at(",")) {
        take();
        settled = true;
      }
    }
    if (!settled) {
      if (lastErr) throw *lastErr;
      err("expected a comma and then the body after the variable list");
    }

    // Each variable is an object of its own. The first in the list is the outermost binder.
    std::vector<ObjectId> objs;
    for (const std::string& v : vars) {
      objs.push_back(w_.variable(v));
      binders_.insert(binders_.begin(), {v, objs.back()});
    }
    // `for every x, y in S, A` abbreviates nested membership guards.
    std::vector<PropId> guards;
    if (hasSet)
      for (ObjectId o : objs) guards.push_back(w_.atom(Term::of(o), set, true));
    assumed_.insert(assumed_.end(), guards.begin(), guards.end());
    PropId body;
    try {
      body = proposition();
    } catch (...) {
      assumed_.resize(assumed_.size() - guards.size());
      binders_.erase(binders_.begin(), binders_.begin() + vars.size());
      throw;
    }
    assumed_.resize(assumed_.size() - guards.size());
    for (PropId g : guards) body = w_.implies(g, body);
    for (size_t i = 0; i < vars.size(); ++i) {
      body = w_.forAll(binders_.front().second, body);
      binders_.erase(binders_.begin());
    }
    return body;
  }
  if (at("there") && peek(1).text == "exists") {
    // `there exists x, y in S such that A`: nested binders, the first outermost, each with
    // its `in S` joined to what follows by `and`.
    take(); take();
    std::vector<std::string> vars;
    vars.push_back(take().text);
    while (accept(",")) vars.push_back(take().text);
    Term set;
    bool hasSet = false;
    if (accept("in")) {
      set = term();
      hasSet = true;
    }
    expect("such");
    expect("that");
    std::vector<ObjectId> objs;
    std::vector<PropId> guards;
    for (const std::string& v : vars) {
      objs.push_back(w_.variable(v));
      binders_.insert(binders_.begin(), {v, objs.back()});
      if (hasSet) guards.push_back(w_.atom(Term::of(objs.back()), set, true));
    }
    assumed_.insert(assumed_.end(), guards.begin(), guards.end());
    PropId body;
    try {
      body = proposition();
    } catch (...) {
      assumed_.resize(assumed_.size() - guards.size());
      binders_.erase(binders_.begin(), binders_.begin() + vars.size());
      throw;
    }
    assumed_.resize(assumed_.size() - guards.size());
    for (size_t i = objs.size(); i-- > 0;) {
      if (hasSet) body = w_.conj(guards[i], body);
      body = w_.exists(objs[i], body);
      binders_.erase(binders_.begin());
    }
    return body;
  }

  // Atom: term (in | notin) term, or a comparison.
  Term subject = term();

  // `a <= b` is notation for `(a, b) in LessEq`. A comparison is not a separate kind of
  // proposition — it is still a cell, like everything else.
  if (at("<") || at("<=") || at(">") || at(">=")) {
    std::string op = take().text;
    Term rhs = term();
    bool flip = op[0] == '>';
    const char* rel = (op == "<" || op == ">") ? "Less" : "LessEq";
    Term lo = flip ? rhs : subject, hi = flip ? subject : rhs;
    return w_.atom(Term::ofTuple({lo, hi}), Term::of(w_.tag(rel)), true);
  }

  bool positive = true;
  if (accept("in")) positive = true;
  else if (accept("notin")) positive = false;
  else err("expected `in` or `notin`, got `" + peek().text + "`");
  Term column = term();
  return w_.atom(std::move(subject), std::move(column), positive);
}

// ------------------------------------------------------------- statements

// Labels may have several words: `(eq comp)`, `(no vote)`.
std::string Parser::labelOpt() {
  if (!at("(")) return {};
  take();
  std::string name;
  while (!at(")") && !peek().text.empty()) {
    if (!name.empty()) name += " ";
    name += take().text;
  }
  expect(")");
  return name;
}

std::vector<std::string> Parser::refList() {
  // The comma separates labels and also precedes `it follows that`, so only continue when
  // what follows the comma really is another label.
  std::vector<std::string> out;
  out.push_back(labelOpt());
  while (at(",") && peek(1).text == "(") {
    take();
    out.push_back(labelOpt());
  }
  return out;
}

Stmt Parser::statement() {
  Stmt s;
  s.line = peek().line;

  if (accept("}")) {
    s.kind = StmtKind::Close;
    return s;
  }

  if (at("Let")) {
    take();
    s.kind = StmtKind::Let;
    s.names.push_back(take().text);
    while (accept(",")) s.names.push_back(take().text);
    if (accept("=")) {
      s.isTupleLet = true;
      expect("(");
      s.tupleTerms.push_back(term());
      while (accept(",")) s.tupleTerms.push_back(term());
      expect(")");
    } else {
      expect("be");
      // `a set`, `an entity`, `relations` — drop the article, keep the noun.
      while (!at(".") && !peek().text.empty()) {
        std::string word = take().text;
        if (word == "a" || word == "an" || word == "the") continue;
        if (!word.empty() && word.back() == 's') word.pop_back();
        s.letKind = word;
      }
    }
    expect(".");
    return s;
  }

  if (at("Assume") || at("Rule")) {
    s.kind = at("Assume") ? StmtKind::Assume : StmtKind::Rule;
    take();
    s.label = labelOpt();
    // `Assume (c).` does not restate the proposition: it stipulates exactly the proposition
    // already carrying that label. Used for conditions produced by the algebra layer.
    if (accept(".")) {
      auto it = labels_.find(s.label);
      if (it == labels_.end()) err("no such label (" + s.label + ")");
      s.prop = it->second;
      return s;
    }
    expect(":");
    s.prop = proposition();
    expect(".");
    return s;
  }

  if (at("By") && peek(1).text == "rule") {
    take(); take();
    s.kind = StmtKind::ByRule;
    s.refs.push_back(labelOpt());
    if (accept("applied")) {
      expect("to");
      expect("(");
      if (!at(")")) {
        s.args.push_back(term());
        while (accept(",")) s.args.push_back(term());
      }
      expect(")");
    }
    expect(",");
    expect("it"); expect("follows"); expect("that");
    makeValues_ = true;
    try {
      s.prop = proposition();
    } catch (const Unapplied& e) {
      makeValues_ = false;
      // F(a) cannot be made. The statement is still read to its end, so the run can go on;
      // running it says what is missing.
      s.prop = kNoProp;
      s.error = e.what();
      while (!at(".") && !peek().text.empty()) take();
      expect(".");
      return s;
    } catch (...) {
      makeValues_ = false;
      throw;
    }
    makeValues_ = false;
    if (accept("as")) s.label = labelOpt();
    expect(".");
    return s;
  }

  if (at("From")) {
    take();
    s.kind = StmtKind::From;
    s.refs = refList();
    expect(",");
    expect("it"); expect("follows"); expect("that");
    s.prop = proposition();
    if (accept("as")) s.label = labelOpt();
    expect(".");
    return s;
  }

  if (at("Suppose")) {
    take();
    s.kind = StmtKind::Suppose;
    s.label = labelOpt();
    expect(":");
    s.prop = proposition();
    expect("{");
    return s;
  }

  if (at("Take")) {
    take();
    s.names.push_back(take().text);
    if (accept("with")) {
      s.kind = StmtKind::TakeIn;
      take();  // the variable name, repeated
      expect("in");
      s.set = term();
    } else if (accept("from")) {
      s.kind = StmtKind::TakeFrom;
      s.refs.push_back(labelOpt());
    } else {
      s.kind = StmtKind::Take;
    }
    expect("{");
    return s;
  }

  if (at("Hence")) {
    take();
    s.kind = StmtKind::Hence;
    s.label = labelOpt();
    expect(":");
    s.prop = proposition();
    expect(".");
    return s;
  }

  if (at("Absurd")) {
    take();
    s.kind = StmtKind::Absurd;
    expect("from");
    s.refs = refList();
    expect(".");
    return s;
  }

  if (at("Theory")) {
    take();
    s.kind = StmtKind::Theory;
    s.names.push_back(take().text);
    expect("{");
    // The body is kept as tokens: names inside have no meaning until Import binds them to
    // something.
    int depth = 1;
    while (!peek().text.empty()) {
      if (at("{")) ++depth;
      if (at("}")) {
        if (--depth == 0) break;
      }
      Token t = take();
      s.body.push_back({t.text, t.line});
      s.bodyWord.push_back(t.word);
      s.bodyNumber.push_back(t.number);
    }
    expect("}");
    return s;
  }

  if (at("Import")) {
    take();
    s.kind = StmtKind::Import;
    s.names.push_back(take().text);
    expect("as");
    s.alias = take().text;
    if (accept("with")) {
      expect("(");
      do {
        std::string from = take().text;
        expect(":");
        expect("=");
        s.mapping.push_back({from, take().text});
      } while (accept(","));
      expect(")");
    }
    expect(".");
    return s;
  }

  if (at("Apply")) {
    take();
    s.kind = StmtKind::Apply;
    s.fn = atomTerm();
    expect("to");
    s.arg = atomTerm();
    if (accept("as")) s.names.push_back(take().text);
    expect(".");
    return s;
  }

  if (at("Notation")) {
    take();
    expect(":");
    if (peek().text.empty() || peek().text[0] != '"') err("expected a template in double quotes");
    std::string tmplText = take().text.substr(1);

    Notation n;
    n.text = tmplText;

    std::vector<std::string> words;
    {
      std::string word;
      for (char ch : tmplText) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
          if (!word.empty()) words.push_back(word);
          word.clear();
        } else {
          word += ch;
        }
      }
      if (!word.empty()) words.push_back(word);
    }

    expect("means");

    // Which template words are holes can only be known by reading the right-hand side. Pass
    // one treats every word as a hole and sees which ones the right-hand side uses; pass
    // two reads it again with exactly those. Unused words are fixed words.
    //
    // A hole is a variable of the template: a variable object, free in the right-hand side.
    // So the right-hand side can be anything a proposition can be — quantifiers, F(X).
    size_t rhs = pos_;
    // Pass one only finds the holes; what it makes is removed once that is known.
    Mark passOne = w_.mark();
    holeNames_ = words;
    holeVars_.clear();
    for (const std::string& word : words) holeVars_.push_back(w_.variable(word));
    // Pass one only finds the holes; F(X) is checked in pass two, against the `where`.
    checkApply_ = false;
    PropId first;
    try {
      first = proposition();
    } catch (...) {
      checkApply_ = true;
      throw;
    }
    checkApply_ = true;

    const std::vector<ObjectId>& free = w_.prop(first).freeVars;
    std::vector<bool> used(words.size(), false);
    for (size_t i = 0; i < words.size(); ++i)
      used[i] = std::binary_search(free.begin(), free.end(), holeVars_[i]);
    w_.rollback(passOne);

    holeNames_.clear();
    holeVars_.clear();
    for (size_t i = 0; i < words.size(); ++i) {
      bool hole = used[i];
      n.parts.push_back({hole ? std::string() : words[i], hole});
      if (hole) {
        holeNames_.push_back(words[i]);
        holeVars_.push_back(w_.variable(words[i]));
        ++n.holes;
      }
    }

    // `where A in Vectors, B in Vectors` — overload resolution guards. They are what is
    // assumed about the holes, so they are read before the right-hand side: F(X) there needs
    // `X in Domain(F)` among them.
    std::vector<PropId> whereProps;
    size_t end = pos_;
    {
      size_t k = rhs;
      int depth = 0;
      while (k < toks_.size() && !(depth == 0 && (toks_[k].text == "where" || toks_[k].text == "."))) {
        if (toks_[k].text == "(" || toks_[k].text == "[") ++depth;
        if (toks_[k].text == ")" || toks_[k].text == "]") --depth;
        ++k;
      }
      if (k < toks_.size() && toks_[k].text == "where") {
        pos_ = k + 1;
        do {
          std::string hole = take().text;
          expect("in");
          Term setTerm = term();
          ObjectId set = w_.resolve(setTerm);
          size_t idx = holeNames_.size();
          for (size_t i = 0; i < holeNames_.size(); ++i)
            if (holeNames_[i] == hole) idx = i;
          if (idx == holeNames_.size()) err("`" + hole + "` is not a hole of the template");
          n.guards.push_back({idx, set});
          whereProps.push_back(w_.atom(Term::of(holeVars_[idx]), Term::of(set), true));
        } while (accept(","));
        end = pos_;
      }
    }

    pos_ = rhs;
    assumed_.insert(assumed_.end(), whereProps.begin(), whereProps.end());
    try {
      n.tmpl = proposition();
    } catch (...) {
      assumed_.resize(assumed_.size() - whereProps.size());
      throw;
    }
    assumed_.resize(assumed_.size() - whereProps.size());
    n.holeVars = holeVars_;
    if (at("where")) pos_ = end;
    expect(".");
    holeNames_.clear();
    holeVars_.clear();

    notations_.push_back(std::move(n));
    s.kind = StmtKind::Close;  // produces nothing for the runtime
    return s;
  }

  if (at("Instantiate")) {
    take();
    s.kind = StmtKind::Instantiate;
    s.refs.push_back(labelOpt());
    expect("at");
    s.expr = term();
    if (accept("as")) s.label = labelOpt();
    expect(".");
    return s;
  }

  if (at("Expand")) {
    take();
    s.kind = StmtKind::Expand;
    s.expr = term();
    if (accept("as")) s.names.push_back(take().text);
    expect(".");
    return s;
  }

  if (at("Simplify")) {
    take();
    s.kind = StmtKind::Simplify;
    s.expr = term();
    if (accept("as")) s.label = labelOpt();
    expect(".");
    return s;
  }

  if (at("Compute")) {
    take();
    s.kind = StmtKind::Compute;
    s.expr = term();
    if (at("<") || at("<=") || at(">") || at(">=")) {
      s.cmpOp = take().text;
      s.rhs = term();
    }
    expect(".");
    return s;
  }

  if (at("Therefore") || at("Why")) {
    s.kind = at("Therefore") ? StmtKind::Therefore : StmtKind::Why;
    take();
    s.prop = proposition();
    // `Therefore A as (h).` names A when it holds, so a proposition that reached the store
    // without a label (through Holds) can be used by label.
    if (s.kind == StmtKind::Therefore && accept("as")) s.label = labelOpt();
    expect(".");
    return s;
  }

  err("unknown statement: `" + peek().text + "`");
}

void Parser::beginTokens(std::vector<Token> toks) {
  toks_ = std::move(toks);
  toks_.push_back({"", toks_.empty() ? 0 : toks_.back().line, false, false});
  pos_ = 0;
}

void Parser::begin(const std::string& source) {
  toks_.clear();
  pos_ = 0;
  lex(source);
}

bool Parser::more() const { return !peek().text.empty(); }

Stmt Parser::next() { return statement(); }

}  // namespace v
