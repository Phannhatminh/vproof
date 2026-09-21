#pragma once
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "v/core.hpp"
#include "v/prop.hpp"

namespace v {

using CellKey = std::pair<ObjectId, ObjectId>;  // (vị trí phần tử, vị trí cột)

// Một tiền đề của bước đã sinh ra kết luận. Tiền đề là một mệnh đề — có thể là
// một cờ ở một ô, có thể là một mục trong kho; cả hai đều là PropId, nên cây lý
// do bắc qua hai chỗ lưu mà không cần phân biệt.
struct Antecedent {
  PropId prop = kNoProp;
};

// Lý do: vì sao một cờ bật. Hai loại, và chỉ hai.
//   đặt ra  — một câu trong chương trình nói thẳng; giữ dòng nguồn.
//   suy ra  — một bước áp luật; giữ luật, binding, và các tiền đề.
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

// Một ô mang hai cờ độc lập. Không có luật nào nối chúng, và không có nhãn
// "conflict": cả hai cùng bật là một trạng thái bình thường.
struct Cell {
  bool in = false, out = false;
  Step inStep = 0, outStep = 0;
  std::vector<Reason> inReasons, outReasons;
};

using Mark = size_t;  // vị trí trong nhật ký

class World {
 public:
  // --- Kho đối tượng ---
  ObjectId declare(const std::string& name);              // luôn ra đối tượng mới
  ObjectId tuple(const std::vector<ObjectId>& elems);     // danh tính theo thành phần
  ObjectId numeral(const Rational& value);                // danh tính theo giá trị

  const Object& obj(ObjectId id) const { return objects_[id]; }
  size_t objectCount() const { return objects_.size(); }
  std::string show(ObjectId id) const;

  // --- Ma trận membership ---
  // Ghi luôn thành công. Cờ chỉ bật, không tắt. Bật lại một cờ đã bật thì
  // chỉ thêm một lý do nữa — giữ hết, không thay thế.
  void tellIn(ObjectId subject, ObjectId column, Reason why);
  void tellOut(ObjectId subject, ObjectId column, Reason why);

  bool toldIn(ObjectId subject, ObjectId column) const;
  bool toldOut(ObjectId subject, ObjectId column) const;
  const std::vector<Reason>& reasons(ObjectId subject, ObjectId column, bool positive) const;
  Step stepOf(ObjectId subject, ObjectId column, bool positive) const;

  size_t cellCount() const { return cells_.size(); }

  // --- Kho mệnh đề ---
  // Mệnh đề được intern theo cấu trúc: dựng cùng một hình dạng hai lần thì ra
  // cùng một PropId. Vì biến buộc lưu theo chỉ số, alpha-đổi-tên là miễn phí.
  PropId atom(Term subject, Term column, bool positive);
  PropId conj(PropId a, PropId b);
  PropId disj(PropId a, PropId b);
  PropId implies(PropId a, PropId b);
  PropId iff(PropId a, PropId b);
  PropId neg(PropId a);
  PropId forAll(const std::string& name, PropId body);
  PropId exists(const std::string& name, PropId body);

  const Prop& prop(PropId id) const { return props_[id]; }
  size_t propCount() const { return props_.size(); }
  std::string showProp(PropId id) const;

  // Cất và tra. Nguyên tử ground đi vào ma trận; còn lại vào kho.
  void tell(PropId id, Reason why);
  bool holds(PropId id);
  const std::vector<Reason>& propReasons(PropId id) const;

  // --- Áp luật ---
  // Thế đối tượng vào các biến buộc ngoài cùng, tra từng tiền đề, ghi kết luận.
  // Tra chứ không tìm: tiền đề nào chưa có thì bước không đi được.
  struct StepResult {
    bool ok = false;
    PropId conclusion = kNoProp;
    PropId missing = kNoProp;  // tiền đề đầu tiên tra không thấy
  };
  StepResult applyRule(PropId rule, const std::vector<ObjectId>& args,
                       const std::string& label, int line);

  // Thế một đối tượng vào biến buộc ngoài cùng của một mệnh đề có binder.
  PropId instantiate(PropId binderProp, ObjectId with);
  // Điền term vào các lỗ của một mẫu Notation.
  PropId fillHoles(PropId templateProp, const std::vector<Term>& args);

  // Tách một tiền đề ghép bằng `and` ở tầng ngoài thành danh sách tiền đề.
  std::vector<PropId> premisesOf(PropId antecedent) const;

  // Bản sao đứng một mình của một mệnh đề. Thoát scope phải dựng mệnh đề trước,
  // quay về mốc, rồi mới cất — nên mệnh đề đó phải sống qua được cú quay về,
  // trong khi mọi PropId dựng trong scope thì bị gỡ.
  struct PropTree {
    PropKind kind = PropKind::Atom;
    Term subject, column;
    bool positive = true;
    std::vector<PropTree> kids;
    std::string binderName;
  };
  PropTree snapshot(PropId id) const;
  PropId rebuild(const PropTree& t);

  // Đối tượng nào xuất hiện trong một mệnh đề — để kiểm kết luận thoát scope
  // không nhắc tới nhân chứng đã biến mất.
  void objectsIn(PropId id, std::vector<ObjectId>& out) const;

  // Term ground quy về một đối tượng; tuple term dựng ra đối tượng tuple.
  bool ground(const Term& t) const;
  ObjectId resolve(const Term& t);

  // --- Mệnh đề là đối tượng ---
  // Cơ chế cung cấp đúng một thứ: cái ghép. Ghép lười — đại diện chỉ sinh ra
  // khi bài chứng minh gọi tên nó.
  //
  // Đại diện là term có cấu trúc dựng từ chính đại diện con, nên nhìn vào là
  // đọc được hình dạng. `Holds` là một ô như mọi ô, nên nó theo scope.
  ObjectId represent(PropId id);
  // Đại diện của một term, dùng khi đi vào thân một binder: biến buộc ở chỉ số
  // k thành `(Var, k)`. Dùng chỉ số chứ không dùng tên là bắt buộc — hai mệnh
  // đề chỉ khác tên biến buộc là cùng một mệnh đề, nên đại diện phải trùng.
  ObjectId representTerm(const Term& t);
  PropId representedBy(ObjectId rep) const;
  ObjectId holdsColumn();
  // Quan hệ ghi lại việc một đối tượng đã được dùng làm cột. Cơ chế không tra
  // gì trước khi ghi vào một ô — `alice ∈ 5` viết được — nhưng nó ghi lại, để
  // lý thuyết có đủ dữ kiện mà tự phán xét.
  ObjectId columnTag();
  ObjectId tag(const std::string& name);

  // --- Nhật ký ---
  Mark mark() const { return journal_.size(); }
  void rollback(Mark m);

 private:
  enum class Undo {
    ObjectCreated, FlagRaised, ReasonAdded, PropCreated, PropTold, RepCreated, TagCreated
  };
  struct Entry {
    Undo what;
    ObjectId object = kNoObject;  // ObjectCreated
    CellKey cell{kNoObject, kNoObject};
    bool positive = true;
    PropId prop = kNoProp;  // PropCreated / PropTold
  };

  struct PropLess {
    bool operator()(const Prop& a, const Prop& b) const { return a.shapeLess(b); }
  };

  PropId intern(Prop p);
  // Giữ `Holds` đồng bộ với kho, cả hai chiều. Đây là nghĩa vụ của cơ chế và
  // là diện tích tin cậy của toàn bộ tầng phản chiếu.
  void syncFromProp(PropId id, const Reason& why);
  void syncFromHolds(ObjectId rep, const Reason& why);

  Cell& cellFor(CellKey k);
  void tell(ObjectId subject, ObjectId column, bool positive, Reason why);

  std::deque<Object> objects_;  // deque: tham chiếu không hỏng khi kho lớn lên
  std::map<CellKey, Cell> cells_;
  std::map<std::vector<ObjectId>, ObjectId> tuples_;
  std::map<Rational, ObjectId> numerals_;
  std::deque<Prop> props_;
  std::map<Prop, PropId, PropLess> propIndex_;
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
