-- Eq là nội dung của prelude. Chỗ nối `Let` sang Eq đi qua `Defined`.

Let alice, bob be entities.
Let Friends be a relation.

Let t = (alice, bob).

-- `Let t = (alice, bob)` ghi (t, (alice, bob)) vào Defined, không ghi vào Eq.
Therefore (t, (alice, bob)) in Defined.

-- Một bước nữa mới sang Eq.
By rule (eq from definition) applied to (t, (alice, bob)),
    it follows that (t, (alice, bob)) in Eq.
Therefore (t, (alice, bob)) in Eq.

-- Và Eq thay được trong mọi quan hệ, bằng một luật của prelude.
Assume (h): (alice, bob) in Friends.
By rule (eq symm) applied to (t, (alice, bob)),
    it follows that ((alice, bob), t) in Eq.
By rule (eq subst) applied to (Friends, (alice, bob), t),
    it follows that t in Friends.
Therefore t in Friends.
Why t in Friends.
