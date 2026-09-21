-- Số thực là ký hiệu, không phải dãy chữ số. `sqrt2` là một đối tượng với
-- tiên đề của nó; đại số chạy trên nó như trên mọi ký hiệu khác.

Let sqrt2 be an entity.
Assume (sq): (sqrt2 * sqrt2, 2) in Eq.

Rule (eq simp): for every u, v, if (u, v) in Simplified then (u, v) in Eq.

-- Congruence là nội dung, viết một luật cho mỗi chỗ cần thay.
Rule (plus congr): for every a, b, c, if (a, b) in Eq then (c + a, c + b) in Eq.

-- (sqrt2 + 1)(sqrt2 - 1) = sqrt2^2 - 1 = 2 - 1 = 1, đi từng bước.
Simplify sqrt2 * sqrt2.
By rule (eq simp) applied to (sqrt2 * sqrt2, sqrt2^2),
    it follows that (sqrt2 * sqrt2, sqrt2^2) in Eq.
By rule (eq symm) applied to (sqrt2 * sqrt2, sqrt2^2),
    it follows that (sqrt2^2, sqrt2 * sqrt2) in Eq.
By rule (eq trans) applied to (sqrt2^2, sqrt2 * sqrt2, 2),
    it follows that (sqrt2^2, 2) in Eq.

By rule (plus congr) applied to (sqrt2^2, 2, -1),
    it follows that (-1 + sqrt2^2, -1 + 2) in Eq.

Simplify (sqrt2 + 1) * (sqrt2 - 1).
By rule (eq simp) applied to ((sqrt2 + 1) * (sqrt2 - 1), -1 + sqrt2^2),
    it follows that ((sqrt2 + 1) * (sqrt2 - 1), -1 + sqrt2^2) in Eq.

Simplify -1 + 2.
By rule (eq simp) applied to (-1 + 2, 1), it follows that (-1 + 2, 1) in Eq.

By rule (eq trans) applied to ((sqrt2 + 1) * (sqrt2 - 1), -1 + sqrt2^2, -1 + 2),
    it follows that ((sqrt2 + 1) * (sqrt2 - 1), -1 + 2) in Eq.
By rule (eq trans) applied to ((sqrt2 + 1) * (sqrt2 - 1), -1 + 2, 1),
    it follows that ((sqrt2 + 1) * (sqrt2 - 1), 1) in Eq.

Therefore ((sqrt2 + 1) * (sqrt2 - 1), 1) in Eq.

-- Muốn con số thì đó là tầng riêng, có đánh dấu. `Approx` của prelude mang
-- theo sai số, và không bao giờ nói sqrt2 BẰNG một số hữu tỉ nào.
Assume (ap): (sqrt2, 1.4, 0.02) in Approx.
Therefore (sqrt2, 1.4, 0.02) in Approx.
Why ((sqrt2 + 1) * (sqrt2 - 1), 1) in Eq.
