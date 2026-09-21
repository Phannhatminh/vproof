-- Function application. `F(a)` only exists after an `Apply F to a.` step.

Let F be a map.
Let a be an entity.
Let Positives be a set.

-- The domain of F is the result of another application step.
Apply Domain to F.
Assume (h): a in Domain(F).

Apply F to a.
Therefore (a, F(a)) in F.

Assume (p): F(a) in Positives.
Therefore F(a) in Positives.
Why (a, F(a)) in F.
