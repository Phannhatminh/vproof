#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "v/syntax.hpp"
#include "v/world.hpp"

namespace v {

struct ParseError : std::runtime_error {
  int line;
  ParseError(int line, const std::string& msg)
      : std::runtime_error("line " + std::to_string(line) + ": " + msg), line(line) {}
};

// The parser builds propositions straight into World. Surface names live only here: bound
// variables become de Bruijn indices, declared names become ObjectIds. The runtime never
// sees a name.
class Parser {
 public:
  struct Token {
    std::string text;
    int line = 0;
    bool word = false;  // name or keyword
    bool number = false;
  };

  Parser(World& w, std::map<std::string, ObjectId>& names,
         std::map<std::string, PropId>& labels)
      : w_(w), names_(names), labels_(labels) {}

  // Parsing and running must interleave: a statement that declares a name must run before
  // later statements can look it up. So the parser hands out one statement at a time
  // instead of parsing the whole file first.
  void begin(const std::string& source);
  void beginTokens(std::vector<Token> toks);
  bool more() const;
  Stmt next();

 private:
  void lex(const std::string& source);
  const Token& peek(size_t ahead = 0) const;
  bool at(const std::string& t) const;
  bool atWord() const;
  Token take();
  void expect(const std::string& t);
  bool accept(const std::string& t);
  [[noreturn]] void err(const std::string& msg) const;

  Stmt statement();
  std::string labelOpt();
  std::vector<std::string> refList();

  PropId proposition();
  PropId iffLevel();
  PropId orLevel();
  PropId andLevel();
  PropId primary();
  Term term();       // + -
  Term mulLevel();   // * /
  Term powLevel();   // ^
  Term atomTerm();

  ObjectId lookup(const std::string& name);

  // Notation template: a sequence of parts, each a fixed word or a hole.
  struct NotationPart {
    std::string word;  // empty means hole
    bool hole = false;
  };
  struct Notation {
    std::vector<NotationPart> parts;
    size_t holes = 0;
    PropId tmpl = kNoProp;
    std::string text;  // verbatim, for printing
    // `where A in Vectors` — guard for overload resolution. Only checkable when the filler
    // is a concrete object; rule variables are skipped.
    std::vector<std::pair<size_t, ObjectId>> guards;
  };
  PropId tryNotation();

  World& w_;
  std::map<std::string, ObjectId>& names_;
  std::map<std::string, PropId>& labels_;
  std::vector<Token> toks_;
  size_t pos_ = 0;
  std::vector<std::string> binders_;  // innermost first; the index is the position
  std::vector<Notation> notations_;
  std::vector<std::string> holeNames_;  // open while parsing a Notation right-hand side

  // What is assumed where the parser is: the `if` parts around, the left sides of `and`
  // before it, the `in S` of the binders around, the `where` of a Notation. Each is kept
  // with the number of binders open when it was assumed, so its variables can be shifted to
  // the indices they have here.
  struct Assumed {
    PropId prop;
    size_t depth;
  };
  std::vector<Assumed> assumed_;
  // Off only while reading a Notation template the first time, to find its holes.
  bool checkApply_ = true;
  // While reading the conclusion of `By rule`: F(a) with no value yet is made there, the
  // same as applying the rule makes it.
  bool makeValues_ = false;
  // F(x) with a variable in F or x: a term kept as written. It is well formed only where
  // `x ∈ Domain(F)` is assumed — checked on the form, the way a type is.
  Term applyTerm(const Term& fn, const Term& arg);
  bool assumedIn(const Term& subject, const Term& column) const;
  // Run `f` with `p` assumed.
  template <class F>
  auto withAssumed(const std::vector<PropId>& ps, F f) -> decltype(f()) {
    for (PropId p : ps) assumed_.push_back({p, binders_.size()});
    try {
      auto out = f();
      assumed_.resize(assumed_.size() - ps.size());
      return out;
    } catch (...) {
      assumed_.resize(assumed_.size() - ps.size());
      throw;
    }
  }
};

}  // namespace v
