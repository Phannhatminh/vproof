#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

#include "v/parser.hpp"
#include "v/prover.hpp"

namespace v {

// Chạy một chương trình V. Tầng này chỉ nối câu lệnh với thao tác của runtime;
// nó không có logic nào của riêng nó.
class Interp {
 public:
  struct Report {
    int checksRun = 0;
    int checksFailed = 0;
    std::vector<std::string> lines;  // vết chạy, in ra cho người đọc
  };

  Report run(const std::string& source);

  World& world() { return w_; }

 private:
  void exec(const Stmt& s, Report& rep);
  PropId ref(const std::string& label, int line) const;
  void bind(const std::string& label, PropId p);
  void whyChain(PropId p, int depth, Report& rep) const;
  // Cửa số học: đưa biểu thức xuống tầng số, nhận về giá trị. Mọi kết quả đều
  // kèm sai số; chính xác là trường hợp sai số bằng 0.
  Rational evalExpr(const Term& t, int line);
  // Đọc nhân chứng bằng khớp mẫu, không thử từng đối tượng.
  bool matchTerm(const Term& concrete, const Term& pattern, VarId depth,
                 ObjectId& witness) const;
  bool matchWitness(PropId concrete, PropId pattern, VarId depth, ObjectId& witness) const;

  std::map<std::string, ObjectId> names_;
  AppliedMap applied_;
  std::map<std::string, PropId> labels_;
  World w_;
  Prover pv_{w_};
  Parser parser_{w_, names_, applied_, labels_};
  // Nhãn và tên khai báo bên trong một scope chỉ sống trong scope đó: thoát ra
  // thì mệnh đề và đối tượng chúng trỏ tới đã bị gỡ khỏi thế giới.
  struct Frame {
    std::vector<std::string> labels, names;
  };
  std::vector<Frame> frames_;
  bool preludeLoaded_ = false;

  // Thân lý thuyết, giữ ở dạng token. Danh tính một thể hiện là (tên lý
  // thuyết, ánh xạ) — import lại đúng cặp đó là không làm gì.
  std::map<std::string, Stmt> theories_;
  std::set<std::string> imported_;
};

}  // namespace v
