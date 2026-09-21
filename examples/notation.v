-- Notation: a way of writing, not mechanism. It unfolds into exactly one proposition, and
-- the runtime never knows any notation exists.

Let Boss, Mentor be relations.
Let alice, bob, carol be entities.

Notation: "A manages B" means (A, B) in Boss.

Assume (h1): alice manages bob.
Assume (h2): bob manages carol.

Therefore (alice, bob) in Boss.

Rule (join): for every x, y, z,
    if x manages y and y manages z then x manages z.

By rule (join) applied to (alice, bob, carol), it follows that alice manages carol.
Therefore alice manages carol.
Therefore (alice, carol) in Boss.
Why (alice, carol) in Boss.

-- Overloading: two templates with the same shape, told apart by `where`.
Let Vectors, Scalars be sets.
Let VecSum, NumSum be relations.
Let u, v, s, t be entities.

Assume (hu): u in Vectors.
Assume (hv): v in Vectors.
Assume (hs): s in Scalars.
Assume (ht): t in Scalars.

Notation: "A plus B is C" means (A, B, C) in VecSum where A in Vectors, B in Vectors.
Notation: "A plus B is C" means (A, B, C) in NumSum where A in Scalars, B in Scalars.

Let w, r be entities.
Assume (h3): u plus v is w.
Assume (h4): s plus t is r.

Therefore (u, v, w) in VecSum.
Therefore (s, t, r) in NumSum.
