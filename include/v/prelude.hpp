#pragma once

namespace v {

// Nguồn của `lib/prelude.v`, nhúng vào để nhị phân tự đứng được.
// Sửa file kia rồi chạy `tools/embed_prelude.py` để cập nhật chỗ này.
inline const char* kPrelude = R"VPRELUDE(
-- Thư viện tối thiểu. Viết bằng V, nạp trước mọi file.
-- Không có gì ở đây là cơ chế: mỗi dòng là một fact hoặc một luật, y như
-- mọi thứ người dùng tự viết.

-- SET, RELATION, MAP là đối tượng thường. Khai trước, vì `be a set` ghi vào SET.
Let SET, RELATION, MAP be objects.

Assume (set is set):  SET in SET.
Assume (rel is set):  RELATION in SET.
Assume (map is set):  MAP in SET.

Rule (relation is set): for every r, if r in RELATION then r in SET.
Rule (map is relation): for every f, if f in MAP then f in RELATION.

-- Domain là hàm, nên Domain(F) cũng là kết quả của một bước áp hàm. Chuỗi đó
-- phải dừng ở đâu đó: nó dừng ở Domain(Domain) = MAP, đặt ra từ đầu, cùng loại
-- với SET in SET.
Let Domain be objects.
Apply Domain to Domain as MAP.

-- Eq: MỘT quan hệ bằng căn bản, áp lên mọi đối tượng. Nó là nội dung, không
-- phải cấu hình nền.
Let Eq be a relation.

Rule (eq refl):  for every a, (a, a) in Eq.
Rule (eq symm):  for every a, b, if (a, b) in Eq then (b, a) in Eq.
Rule (eq trans): for every a, b, c, if (a, b) in Eq and (b, c) in Eq then (a, c) in Eq.
Rule (eq subst): for every S, a, b, if (a, b) in Eq and a in S then b in S.

-- `Let t = (a, b)` ghi vào Defined; dòng này mới nối nó sang Eq.
Rule (eq from definition): for every a, b, if (a, b) in Defined then (a, b) in Eq.

-- Hàm: một quan hệ mà mỗi đầu vào chỉ một đầu ra. Không cần ngôn ngữ hỗ trợ
-- riêng — nó là một set cộng hai luật.
Let Function be a set.
Let TotalOn be a relation.

Rule (function is relation): for every f, if f in Function then f in RELATION.

Rule (function unique): for every f, x, y, z,
    if f in Function and (x, y) in f and (x, z) in f then (y, z) in Eq.

Rule (total on): for every f, D,
    if (f, D) in TotalOn then (for every x, if x in D then there exists y such that (x, y) in f).

-- Xấp xỉ là một quan hệ, không phải cơ chế. Cơ chế chỉ khai "tôi trả về gì,
-- sai số tôi khai là bao nhiêu"; chữ *xấp xỉ* không xuất hiện ở đó. Định nghĩa
-- dưới đây là một lựa chọn — viết định nghĩa khác thì ra nghĩa khác, và hệ
-- thống không có ý kiến.
Let Approx be a relation.

Rule (approx def): for every u, v, k,
    if (u, v, k) in Computed then (u, v, k) in Approx.

Rule (approx sum): for every u, v, k, p, q, r, s, t,
    if (u, v, k) in Approx and (p, q, r) in Approx
       and (s, u + p, 0) in Computed and (t, v + q, 0) in Computed
    then (s, t, k + r) in Approx.

-- Chú ý: luật nối `Simplified` / `SimplifiedIf` / `Computed` sang `Eq` KHÔNG ở đây. Nạp nó là tuyên bố tin số
-- học của máy, nên nó phải do người viết tự đặt vào file của mình:
--     Rule (eq comp): for every a, b, if (a, b, 0) in Computed then (a, b) in Eq.
-- Luật cổ điển (nổ, bài trung, phủ định kép) cũng vậy.
)VPRELUDE";

}  // namespace v
