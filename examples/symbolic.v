-- Door 2: symbolic algebra. Exact, produces no numbers.
-- The normal form is a multivariate rational function; a "variable" is any object.

Let x, y, a, b, c, d be entities.
Let pi, e be entities.

-- Loading this rule is a declaration: I trust the algebra layer. Without it, Simplified
-- stays inert.
Rule (eq simp): for every u, v, if (u, v) in Simplified then (u, v) in Eq.

Simplify (x + 1) * (x - 1).
Simplify 1/3 + 1/6.
Simplify a/b + c/d.
Simplify 2*x + 3*x.
Simplify x*y - y*x.
Simplify (x + y)^2.

-- pi and e are symbols, not strings of digits. Algebra still runs on them.
Simplify pi*e + e*pi.
Simplify (pi + e)*(pi - e).

By rule (eq simp) applied to ((x + 1) * (x - 1), -1 + x^2),
    it follows that ((x + 1) * (x - 1), -1 + x^2) in Eq.
Therefore ((x + 1) * (x - 1), -1 + x^2) in Eq.

By rule (eq simp) applied to (x*y - y*x, 0),
    it follows that (x*y - y*x, 0) in Eq.
Therefore (x*y - y*x, 0) in Eq.

-- Cancelling a factor that contains a variable does simplify, but the condition has to be
-- visible instead of hiding inside the equality.
Rule (eq simp if): for every u, v, k,
    if (u, v, k) in SimplifiedIf and k in Holds then (u, v) in Eq.

-- `as (nz)` labels the condition, so it need not be copied out in the algebra layer's
-- normal form.
Simplify (x^2 - 1) / (x - 1) as (nz).

Assume (nz).                       -- stipulates exactly the proposition with that label
Therefore [(nz)] in Holds.

By rule (eq simp if) applied to ((x^2 - 1)/(x - 1), 1 + x, [(nz)]),
    it follows that ((x^2 - 1)/(x - 1), 1 + x) in Eq.
Therefore ((x^2 - 1)/(x - 1), 1 + x) in Eq.

Why ((x + 1) * (x - 1), -1 + x^2) in Eq.
