#pragma once
#include <string>

namespace v {

// Số hữu tỉ chính xác: tử và mẫu không giới hạn độ lớn, luôn tối giản.
// GMP nằm sau lớp này và không lộ ra header nào khác, để đổi backend được.
class Rational {
 public:
  Rational();
  Rational(long long n);
  Rational(long long num, long long den);
  // Đọc "3", "-7/4", "0.1" — thập phân đọc chính xác, 0.1 là 1/10.
  explicit Rational(const std::string& text);

  Rational(const Rational&);
  Rational(Rational&&) noexcept;
  Rational& operator=(const Rational&);
  Rational& operator=(Rational&&) noexcept;
  ~Rational();

  Rational operator+(const Rational&) const;
  Rational operator-(const Rational&) const;
  Rational operator*(const Rational&) const;
  // Chia cho 0 ném RationalDivByZero; tầng trên biến nó thành fact
  // "không có giá trị", không phải lỗi của chương trình.
  Rational operator/(const Rational&) const;

  bool operator==(const Rational&) const;
  bool operator!=(const Rational& o) const { return !(*this == o); }
  bool operator<(const Rational&) const;
  bool operator<=(const Rational& o) const { return *this < o || *this == o; }

  bool isZero() const;
  std::string str() const;  // "3", "-7/4"

 private:
  struct Impl;
  Impl* p_;
};

struct RationalDivByZero {};

}  // namespace v
