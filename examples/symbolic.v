-- Cửa 2: đại số ký hiệu. Chính xác, không ra số.
-- Dạng chuẩn là phân thức hữu tỉ nhiều biến; "biến" là đối tượng bất kỳ.

Let x, y, a, b, c, d be entities.
Let pi, e be entities.

-- Nạp dòng này là tuyên bố: tôi tin tầng đại số. Không nạp thì Simplified nằm trơ.
Rule (eq simp): for every u, v, if (u, v) in Simplified then (u, v) in Eq.

Simplify (x + 1) * (x - 1).
Simplify 1/3 + 1/6.
Simplify a/b + c/d.
Simplify 2*x + 3*x.
Simplify x*y - y*x.
Simplify (x + y)^2.

-- pi và e là ký hiệu, không phải dãy chữ số. Đại số vẫn chạy trên chúng.
Simplify pi*e + e*pi.
Simplify (pi + e)*(pi - e).

By rule (eq simp) applied to ((x + 1) * (x - 1), -1 + x^2),
    it follows that ((x + 1) * (x - 1), -1 + x^2) in Eq.
Therefore ((x + 1) * (x - 1), -1 + x^2) in Eq.

By rule (eq simp) applied to (x*y - y*x, 0),
    it follows that (x*y - y*x, 0) in Eq.
Therefore (x*y - y*x, 0) in Eq.

-- Triệt ước có chứa biến thì rút gọn được, nhưng điều kiện phải hiện ra chứ
-- không nằm ngầm trong đẳng thức.
Rule (eq simp if): for every u, v, k,
    if (u, v, k) in SimplifiedIf and k in Holds then (u, v) in Eq.

-- `as (nz)` đặt nhãn cho điều kiện, nên khỏi phải chép lại nó ở dạng chuẩn
-- của tầng đại số.
Simplify (x^2 - 1) / (x - 1) as (nz).

Assume (nz).                       -- đặt ra đúng mệnh đề mang nhãn đó
Therefore [(nz)] in Holds.

By rule (eq simp if) applied to ((x^2 - 1)/(x - 1), 1 + x, [(nz)]),
    it follows that ((x^2 - 1)/(x - 1), 1 + x) in Eq.
Therefore ((x^2 - 1)/(x - 1), 1 + x) in Eq.

Why ((x + 1) * (x - 1), -1 + x^2) in Eq.
