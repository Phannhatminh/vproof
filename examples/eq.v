-- Eq is prelude content. The link from `Let` to Eq goes through `Defined`.

Let alice, bob be entities.
Let Friends be a relation.

Let t = (alice, bob).

-- `Let t = (alice, bob)` writes (t, (alice, bob)) into Defined, not into Eq.
Therefore (t, (alice, bob)) in Defined.

-- One more step to reach Eq.
By rule (eq from definition) applied to (t, (alice, bob)),
    it follows that (t, (alice, bob)) in Eq.
Therefore (t, (alice, bob)) in Eq.

-- And Eq substitutes in any relation, by a prelude rule.
Assume (h): (alice, bob) in Friends.
By rule (eq symm) applied to (t, (alice, bob)),
    it follows that ((alice, bob), t) in Eq.
By rule (eq subst) applied to (Friends, (alice, bob), t),
    it follows that t in Friends.
Therefore t in Friends.
Why t in Friends.
