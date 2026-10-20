#include "v/parser.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

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
  if (!atWord()) err("expected a term, got `" + peek().text + "`");
  Token t = take();
  // Notation holes are looked up first — they are only open while parsing a right-hand
  // side. Then bound variables: the index is the position in the binder stack.
  Term name;
  bool found = false;
  for (size_t i = 0; i < holeNames_.size() && !found; ++i)
    if (holeNames_[i] == t.text) {
      name = Term::ofVar(kHole + static_cast<VarId>(i));
      found = true;
    }
  for (size_t i = 0; i < binders_.size() && !found; ++i)
    if (binders_[i] == t.text) {
      name = Term::ofVar(static_cast<VarId>(i));
      found = true;
    }
  if (!found) name = Term::of(lookup(t.text));
  if (!at("(")) return name;

  // `F(a)` is function application. With a concrete F and a it points at the object created
  // by an `Apply F to a.` step; until that step has run, no object has this name. With a
  // variable in F or a it is F(x) as written, checked to be well formed.
  take();
  Term arg = term();
  expect(")");
  if (!w_.ground(name) || !w_.ground(arg)) return applyTerm(name, arg);
  ObjectId value = w_.appliedValue(name.obj, w_.resolve(arg));
  // In the conclusion of `By rule`, F(a) with no value yet is made there, the same as
  // applying the rule makes it.
  if (value == kNoObject && makeValues_) value = w_.apply(name.obj, w_.resolve(arg), t.line, true);
  if (value == kNoObject)
    err("`" + t.text + "(...)` does not exist yet — needs `Apply " + t.text + " to ...` first");
  return Term::of(value);
}

namespace {

// Move the variables of a term assumed `by` binders further out to the indices they have
// here. Holes are not binders and do not move.
Term shifted(const Term& t, size_t by) {
  if (t.kind == TermKind::Var)
    return t.var >= kHole ? t : Term::ofVar(t.var + static_cast<VarId>(by));
  if (t.kind == TermKind::Obj) return t;
  Term out = t;
  for (Term& e : out.elems) e = shifted(e, by);
  return out;
}

}  // namespace

bool Parser::assumedIn(const Term& subject, const Term& column) const {
  for (const Assumed& a : assumed_) {
    size_t by = binders_.size() - a.depth;
    for (PropId q : w_.premisesOf(a.prop)) {
      const Prop& p = w_.prop(q);
      if (p.kind != PropKind::Atom || !p.positive) continue;
      if (shifted(p.subject, by) == subject && shifted(p.column, by) == column) return true;
    }
  }
  return false;
}

Term Parser::applyTerm(const Term& fn, const Term& arg) {
  // The same check as for F(a), done on the form: `x ∈ Domain(F)` must be among what is
  // assumed here. Domain(F) for a concrete F is the value already made; for a variable f it
  // is Domain(f), itself checked — it needs `f ∈ Domain(Domain)`, that is `f ∈ MAP`.
  auto dIt = names_.find("Domain");
  if (dIt == names_.end()) err("`Domain` is not defined");
  ObjectId domain = dIt->second;
  Term dom;
  if (w_.ground(fn)) {
    ObjectId F = w_.resolve(fn);
    ObjectId d = w_.appliedValue(domain, F);
    if (d == kNoObject)
      err("Domain(" + w_.show(F) + ") does not exist yet — needs `Apply Domain to " + w_.show(F) +
          ".` first");
    dom = Term::of(d);
  } else {
    dom = applyTerm(Term::of(domain), fn);
  }
  if (checkApply_ && !assumedIn(arg, dom)) {
    // Shown through a proposition, so variables print with their names.
    PropId shown = w_.atom(arg, dom, true);
    std::string what = w_.showPropAt(shown, binders_, holeNames_);
    err("`F(x)` cannot be written here — not established: `" + what + "`");
  }
  return Term::ofApp(fn, arg);
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
    PropId right = withAssumed({left}, [&] { return primary(); });
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

    // `where` guards: can only filter when the filler is already a concrete object.
    bool guarded = true;
    for (const auto& [i, set] : n.guards) {
      if (i >= args.size() || !w_.ground(args[i])) continue;
      if (!w_.toldIn(w_.resolve(args[i]), set)) { guarded = false; break; }
    }
    if (!guarded) continue;

    if (pos_ > bestEnd) {
      bestEnd = pos_;
      best = w_.fillHoles(n.tmpl, args);
      ambiguous = false;
    } else if (pos_ == bestEnd && best != kNoProp) {
      PropId other = w_.fillHoles(n.tmpl, args);
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

PropId Parser::primary() {
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
    expect("(");
    PropId inner = proposition();
    expect(")");
    return w_.neg(inner);
  }
  if (accept("if")) {
    PropId a = proposition();
    expect("then");
    PropId b = withAssumed({a}, [&] { return proposition(); });
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

    std::vector<std::string> vars;
    std::string setName;
    bool settled = false;
    for (size_t n = most; n >= 1 && !settled; --n) {
      pos_ = save;
      vars.clear();
      setName.clear();
      vars.push_back(take().text);
      for (size_t i = 1; i < n; ++i) {
        expect(",");
        vars.push_back(take().text);
      }
      if (at("in") && peek(1).word && peek(2).text == ",") {
        take();
        setName = take().text;
      }
      if (at(",")) {
        take();
        settled = true;
      }
    }
    if (!settled) err("expected a comma and then the body after the variable list");

    // The first variable in the list is the outermost binder, so it gets the largest index;
    // the last variable is innermost, index 0.
    for (const std::string& v : vars) binders_.insert(binders_.begin(), v);
    // `for every x, y in S, A` abbreviates nested membership guards.
    std::vector<PropId> guards;
    if (!setName.empty()) {
      ObjectId set = lookup(setName);
      for (size_t i = 0; i < vars.size(); ++i) {
        VarId idx = static_cast<VarId>(vars.size() - 1 - i);
        guards.push_back(w_.atom(Term::ofVar(idx), Term::of(set), true));
      }
    }
    PropId body;
    try {
      body = withAssumed(guards, [&] { return proposition(); });
    } catch (...) {
      binders_.erase(binders_.begin(), binders_.begin() + vars.size());
      throw;
    }
    for (PropId g : guards) body = w_.implies(g, body);
    for (size_t i = 0; i < vars.size(); ++i) {
      body = w_.forAll(binders_.front(), body);
      binders_.erase(binders_.begin());
    }
    return body;
  }
  if (at("there") && peek(1).text == "exists") {
    take(); take();
    std::string var = take().text;
    std::string setName;
    if (accept("in")) setName = take().text;
    expect("such");
    expect("that");
    binders_.insert(binders_.begin(), var);
    std::vector<PropId> guards;
    if (!setName.empty()) guards.push_back(w_.atom(Term::ofVar(0), Term::of(lookup(setName)), true));
    PropId body;
    try {
      body = withAssumed(guards, [&] { return proposition(); });
    } catch (...) {
      binders_.erase(binders_.begin());
      throw;
    }
    if (!guards.empty()) body = w_.conj(guards[0], body);
    binders_.erase(binders_.begin());
    return w_.exists(var, body);
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
      s.tupleElems.push_back(take().text);
      while (accept(",")) s.tupleElems.push_back(take().text);
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
      s.setName = take().text;
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
    size_t rhs = pos_;
    holeNames_ = words;
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

    std::vector<bool> used(words.size(), false);
    std::function<void(const Term&)> markTerm = [&](const Term& t) {
      if (t.kind == TermKind::Var && t.var >= kHole) {
        size_t i = t.var - kHole;
        if (i < used.size()) used[i] = true;
      }
      for (const Term& e : t.elems) markTerm(e);
    };
    std::function<void(PropId)> mark = [&](PropId cur) {
      const Prop& p = w_.prop(cur);
      if (p.kind == PropKind::Atom) {
        markTerm(p.subject);
        markTerm(p.column);
        return;
      }
      if (p.left != kNoProp) mark(p.left);
      if (p.right != kNoProp) mark(p.right);
    };
    mark(first);

    holeNames_.clear();
    for (size_t i = 0; i < words.size(); ++i) {
      bool hole = used[i];
      n.parts.push_back({hole ? std::string() : words[i], hole});
      if (hole) {
        holeNames_.push_back(words[i]);
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
      while (k < toks_.size() &&
             !(depth == 0 && (toks_[k].text == "where" || toks_[k].text == "."))) {
        if (toks_[k].text == "(" || toks_[k].text == "[") ++depth;
        if (toks_[k].text == ")" || toks_[k].text == "]") --depth;
        ++k;
      }
      if (k < toks_.size() && toks_[k].text == "where") {
        pos_ = k + 1;
        do {
          std::string hole = take().text;
          expect("in");
          ObjectId set = w_.resolve(term());
          size_t idx = holeNames_.size();
          for (size_t i = 0; i < holeNames_.size(); ++i)
            if (holeNames_[i] == hole) idx = i;
          if (idx == holeNames_.size()) err("`" + hole + "` is not a hole of the template");
          n.guards.push_back({idx, set});
          whereProps.push_back(
              w_.atom(Term::ofVar(kHole + static_cast<VarId>(idx)), Term::of(set), true));
        } while (accept(","));
        end = pos_;
      }
    }

    pos_ = rhs;
    n.tmpl = withAssumed(whereProps, [&] { return proposition(); });
    if (at("where")) pos_ = end;
    expect(".");
    holeNames_.clear();

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
