-- The arithmetic door. The machine always returns an error bound; equality only follows
-- when the error is 0.

-- Loading this rule is a declaration: I trust the machine's arithmetic.
Rule (eq comp): for every a, b, if (a, b, 0) in Computed then (a, b) in Eq.

Compute 100 + 101.
Compute 1/3 + 1/6.
Compute 0.1 + 0.2.
Compute 2 * 3 - 1.
Compute 1 / 0.

By rule (eq comp) applied to (100 + 101, 201), it follows that (100 + 101, 201) in Eq.
Therefore (100 + 101, 201) in Eq.

By rule (eq comp) applied to (1/3 + 1/6, 1/2), it follows that (1/3 + 1/6, 1/2) in Eq.
Therefore (1/3 + 1/6, 1/2) in Eq.

Therefore 1 / 0 in NoValue.
Why (100 + 101, 201) in Eq.
