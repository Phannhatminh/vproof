-- Reals are symbols, not strings of digits. `sqrt2` is an object with its own axioms;
-- algebra runs on it as on any other symbol.

Let sqrt2 be an entity.
Assume (sq): (sqrt2 * sqrt2, 2) in Eq.

Rule (eq simp): for every u, v, if (u, v) in Simplified then (u, v) in Eq.

-- Congruence is content: write one rule for each place where substitution is needed.
Rule (plus congr): for every a, b, c, if (a, b) in Eq then (c + a, c + b) in Eq.

-- (sqrt2 + 1)(sqrt2 - 1) = sqrt2^2 - 1 = 2 - 1 = 1, step by step.
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

-- If you want a number, that is a separate, marked layer. The prelude's `Approx` carries
-- the error, and never says sqrt2 EQUALS any rational.
Assume (ap): (sqrt2, 1.4, 0.02) in Approx.
Therefore (sqrt2, 1.4, 0.02) in Approx.
Why ((sqrt2 + 1) * (sqrt2 - 1), 1) in Eq.
