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
      : std::runtime_error("dòng " + std::to_string(line) + ": " + msg), line(line) {}
};

// Parser dựng thẳng mệnh đề vào World. Tên bề mặt chỉ sống ở đây: biến buộc
// thành chỉ số de Bruijn, tên đã khai báo thành ObjectId. Runtime không bao
// giờ thấy một cái tên nào.
using AppliedMap = std::map<std::pair<ObjectId, ObjectId>, ObjectId>;

class Parser {
 public:
  struct Token {
    std::string text;
    int line = 0;
    bool word = false;  // tên hoặc từ khoá
    bool number = false;
  };

  Parser(World& w, std::map<std::string, ObjectId>& names, AppliedMap& applied,
         std::map<std::string, PropId>& labels)
      : w_(w), names_(names), applied_(applied), labels_(labels) {}

  // Parse và chạy phải xen kẽ: một câu khai báo tên thì câu sau mới tra được
  // tên đó. Nên parser trả từng câu một chứ không parse cả file trước.
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

  // Mẫu Notation: dãy phần, mỗi phần là một chữ cố định hoặc một lỗ.
  struct NotationPart {
    std::string word;  // rỗng nghĩa là lỗ
    bool hole = false;
  };
  struct Notation {
    std::vector<NotationPart> parts;
    size_t holes = 0;
    PropId tmpl = kNoProp;
    std::string text;  // nguyên văn, để in ra
    // `where A in Vectors` — điều kiện để phân giải quá tải. Chỉ kiểm được
    // khi chỗ điền là một đối tượng cụ thể; biến của luật thì bỏ qua.
    std::vector<std::pair<size_t, ObjectId>> guards;
  };
  PropId tryNotation();

  World& w_;
  std::map<std::string, ObjectId>& names_;
  // Giá trị canonical của một hàm tại một đối số, do `Apply` tạo ra. Parser
  // cần nó để đọc `F(a)`, nên nó nằm chung chỗ với bảng tên.
  AppliedMap& applied_;
  std::map<std::string, PropId>& labels_;
  std::vector<Token> toks_;
  size_t pos_ = 0;
  std::vector<std::string> binders_;  // trong ra ngoài; chỉ số là vị trí
  std::vector<Notation> notations_;
  std::vector<std::string> holeNames_;  // đang mở khi parse vế phải của Notation
};

}  // namespace v
