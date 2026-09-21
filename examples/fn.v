-- Áp hàm. `F(a)` chỉ tồn tại sau khi có bước `Apply F to a.`

Let F be a map.
Let a be an entity.
Let Positives be a set.

-- Miền của F là kết quả của một bước áp hàm nữa.
Apply Domain to F.
Assume (h): a in Domain(F).

Apply F to a.
Therefore (a, F(a)) in F.

Assume (p): F(a) in Positives.
Therefore F(a) in Positives.
Why (a, F(a)) in F.
