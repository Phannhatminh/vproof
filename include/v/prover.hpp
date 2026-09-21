#pragma once
#include <string>
#include <vector>

#include "v/world.hpp"

namespace v {

// Bộ thao tác bước chứng minh. Có đúng hai hình dạng: cái nào buộc biến hoặc
// rút giả định thì cần scope, còn lại chỉ là tra rồi cất.
//
// Prover không giữ trạng thái logic nào của riêng nó ngoài ngăn xếp scope —
// mọi fact vẫn nằm trong World.
class Prover {
 public:
  explicit Prover(World& w) : w_(w) {}

  struct Result {
    bool ok = false;
    PropId prop = kNoProp;     // mệnh đề vừa cất
    PropId missing = kNoProp;  // tiền đề đầu tiên tra không thấy
    std::string error;
  };

  // --- Đặt ra ---
  Result assume(PropId p, int line);

  // --- Đưa vào, không scope ---
  Result introAnd(PropId a, PropId b, int line);
  Result introOr(PropId held, PropId other, bool heldOnLeft, int line);
  Result introIff(PropId aToB, PropId bToA, int line);
  // Tra A(t), cất `there exists x such that A(x)`: trừu tượng hoá `witness`.
  Result introExists(PropId instance, ObjectId witness, const std::string& name, int line);

  // --- Dùng, không scope ---
  Result elimAnd(PropId conjunction, bool takeLeft, int line);
  Result elimIff(PropId equivalence, bool forward, int line);

  // Chỉ ra vô lý. Không nổ: nó chỉ đánh dấu scope hiện tại, để lúc thoát dựng
  // được `not (...)`.
  Result absurd(PropId p, PropId notP, int line);
  // `or`-dùng: tra `A or B`, `not A`, `not B` — ba mệnh đề riêng biệt — rồi
  // chỉ ra vô lý. Không chia nhánh, không thế giới song song.
  Result absurdFromOr(PropId disjunction, PropId notA, PropId notB, int line);

  // --- Scope ---
  Result suppose(PropId assumption, const std::string& label, int line);
  Result take(const std::string& name, int line);
  Result takeIn(const std::string& name, ObjectId set, int line);
  Result takeFrom(const std::string& name, PropId existential, int line);

  // Thoát: kiểm mệnh đề người viết nêu ra có đúng là thứ scope này dựng được
  // không, rồi quay về mốc và cất nó.
  Result hence(PropId stated, const std::string& label, int line);

  size_t depth() const { return scopes_.size(); }
  ObjectId freshOf() const { return scopes_.empty() ? kNoObject : scopes_.back().fresh; }

 private:
  enum class ScopeKind { Suppose, Take, TakeIn, TakeFrom };
  struct Scope {
    ScopeKind kind = ScopeKind::Suppose;
    Mark mark = 0;
    PropId assumption = kNoProp;
    PropId membership = kNoProp;
    ObjectId fresh = kNoObject;
    std::string name, label;
    bool absurd = false;
    int line = 0;
  };

  Result fail(const std::string& msg) const;
  Result missing(PropId p) const;

  World& w_;
  std::vector<Scope> scopes_;
};

}  // namespace v
