-- Hai dạng dãy cùng tồn tại; bước khai triển nối chúng, tường minh.
-- Dạng chuẩn là dạng hàm, vì chỉ nó mới phủ được độ dài ký hiệu.

Let a, b, c be entities.

Expand (a, b, c) as t3.

-- t3 là một set các cặp (chỉ số, phần tử) — đúng nghĩa một hàm trên Index(3).
Therefore (1, a) in t3.
Therefore (2, b) in t3.
Therefore (3, c) in t3.

-- Và bước khai triển được ghi lại, nên nối sang Eq là một tuyên bố riêng.
Therefore ((a, b, c), t3) in Expanded.

Rule (eq expand): for every u, v, if (u, v) in Expanded then (u, v) in Eq.
By rule (eq expand) applied to ((a, b, c), t3), it follows that ((a, b, c), t3) in Eq.
Therefore ((a, b, c), t3) in Eq.

-- Dạng liệt kê không bị bỏ: nó vẫn làm khoá cho quan hệ nhiều ngôi.
Let Met be a relation.
Assume (h): (a, b, c) in Met.
Therefore (a, b, c) in Met.

Why (1, a) in t3.
