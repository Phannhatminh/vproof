-- Both forms of a sequence exist; the expansion step joins them, explicitly.
-- The canonical form is the function form, because only it covers symbolic lengths.

Let a, b, c be entities.

Expand (a, b, c) as t3.

-- t3 is a set of (index, element) pairs — exactly a function on Index(3).
Therefore (1, a) in t3.
Therefore (2, b) in t3.
Therefore (3, c) in t3.

-- And the expansion step is recorded, so connecting it to Eq is a separate declaration.
Therefore ((a, b, c), t3) in Expanded.

Rule (eq expand): for every u, v, if (u, v) in Expanded then (u, v) in Eq.
By rule (eq expand) applied to ((a, b, c), t3), it follows that ((a, b, c), t3) in Eq.
Therefore ((a, b, c), t3) in Eq.

-- The listed form is not dropped: it still serves as the key of an n-ary relation.
Let Met be a relation.
Assume (h): (a, b, c) in Met.
Therefore (a, b, c) in Met.

Why (1, a) in t3.
