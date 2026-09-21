#pragma once
#include <vector>

#include "v/world.hpp"

namespace v {

// Cửa đại số ký hiệu: biến đổi biểu thức, chính xác, không ra số.
//
// Dạng chuẩn là một phân thức hữu tỉ nhiều biến — tử và mẫu đều là đa thức với
// hệ số hữu tỉ chính xác, và "biến" là một đối tượng bất kỳ: một tên đã khai
// báo, `pi`, `e`, hay một giá trị áp hàm. Một dạng chuẩn duy nhất làm được cả
// rút gọn, triển khai, cộng phân số lẫn cộng phân thức.
//
// Triệt ước có chứa biến thì phải kèm điều kiện: `(x^2-1)/(x-1) = x+1` chỉ
// đúng khi `x - 1` khác 0. Mỗi nhân tử bị triệt được trả ra trong `nonzero`,
// và tầng trên có nhiệm vụ biến chúng thành một mệnh đề phải xác lập.
//
// Ném std::runtime_error nếu biểu thức không phải đại số, hoặc mẫu bằng 0.
Term simplify(World& w, const Term& t, std::vector<Term>* nonzero = nullptr);

}  // namespace v
