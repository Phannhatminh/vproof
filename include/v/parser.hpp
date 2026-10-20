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

// The parser builds propositions straight into World. Surface names live only here: a
// variable of a rule becomes a variable object, declared names become ObjectIds. The runtime
// never sees a name.

class Parser {
 public:
  struct Token {
    std::string text;
    int line = 0;
    bool word = false;  // name or keyword
    bool number = false;
  };

  Parser(World& w, std::map<std::string, ObjectId>& names, std::map<std::string, PropId>& labels)
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
  Term atomBase();
  ObjectId applyTerm(const Term& fn, const Term& arg, int line);

  ObjectId lookup(const std::string& name);
  // A name in a term: a variable of an enclosing binder first, then a declared name.
  ObjectId lookupObject(const std::string& name);

  // Notation template: a sequence of parts, each a fixed word or a hole.
  struct NotationPart {
    std::string word;  // empty means hole
    bool hole = false;
  };
  struct Notation {
    std::vector<NotationPart> parts;
    size_t holes = 0;
    PropId tmpl = kNoProp;
    std::vector<ObjectId> holeVars;  // the template's variables, one per hole
    std::string text;  // verbatim, for printing
    // `where A in Vectors` — guard for overload resolution. A concrete filler is checked in
    // the matrix; a variable against what is assumed about it where the notation is used.
    std::vector<std::pair<size_t, ObjectId>> guards;
  };
  PropId tryNotation();

  World& w_;
  std::map<std::string, ObjectId>& names_;
  std::map<std::string, PropId>& labels_;
  std::vector<Token> toks_;
  size_t pos_ = 0;
  std::vector<std::pair<std::string, ObjectId>> binders_;  // innermost first
  std::vector<Notation> notations_;
  std::vector<std::string> holeNames_;  // open while parsing a Notation right-hand side
  std::vector<ObjectId> holeVars_;      // the variable of each of those holes
  // What is assumed where the parser is: the `if` parts around, and the `in S` of the
  // binders around. Notation guards on a variable look here.
  std::vector<PropId> assumed_;
  // While reading the conclusion of `By rule`: F(a) with no value yet is made there, the
  // same as applying the rule makes it.
  bool makeValues_ = false;
};

}  // namespace v
