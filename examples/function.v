-- A function is content: a relation plus a uniqueness rule. The prelude provides it.

Let F be a relation.
Let a, u, v be entities.
Let D be a set.

Assume (fn):  F in Function.
Assume (h1):  (a, u) in F.
Assume (h2):  (a, v) in F.

-- Two values at the same input are equal — that is the whole meaning of "function", and it
-- is a rule, not a check performed by the mechanism.
By rule (function unique) applied to (F, a, u, v), it follows that (u, v) in Eq.
Therefore (u, v) in Eq.
Why (u, v) in Eq.

-- And F is also a relation, then a set — two steps, both derivable.
By rule (function is relation) applied to (F), it follows that F in RELATION.
By rule (relation is set) applied to (F), it follows that F in SET.
Therefore F in SET.

-- TotalOn: total on D means every element of D has an image.
Assume (tot): (F, D) in TotalOn.
By rule (total on) applied to (F, D),
    it follows that for every x, if x in D then there exists y such that (x, y) in F.
Therefore for every x, if x in D then there exists y such that (x, y) in F.
