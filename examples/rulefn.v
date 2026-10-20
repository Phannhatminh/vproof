-- F(x) inside a rule. A rule variable is an object, so F(x) is an object too: the one the
-- rule speaks of. Applying the rule to a turns it into F(a), made by the rule step itself
-- when it does not exist yet, with the same domain check as `Apply F to a`.

Let F, G be maps.
Let Pos, Good be sets.
Let a be an entity.

Apply Domain to F.
Apply Domain to G.
Assume (ha): a in Domain(G).
Apply G to a.
Assume (hga): G(a) in Domain(F).

Rule (pos): for every x, if x in Domain(F) then F(x) in Pos.
Rule (good): for every x, if F(G(x)) in Pos then x in Good.

-- F(G(a)) does not exist before this step; the step makes it.
By rule (pos) applied to (G(a)), it follows that F(G(a)) in Pos.
By rule (good) applied to (a), it follows that a in Good.
Therefore a in Good.
Therefore (G(a), F(G(a))) in F.

-- The same rule written with another variable is the same proposition, and it is an object
-- like any other: F(x) inside its representative is (App, F, (Var, 0)).
Therefore for every z, if z in Domain(F) then F(z) in Pos.
Therefore [for every x, if x in Domain(F) then F(x) in Pos] in Holds.

-- The function can be a variable too.
Rule (own): for every f, y, if (y, f(y)) in f then f(y) in Good.
By rule (own) applied to (G, a), it follows that G(a) in Good.
Therefore G(a) in Good.

-- Proved with scopes: F(x) for the x taken, made inside the inner scope.
Take x {
    Suppose (h): x in Domain(F) {
        By rule (pos) applied to (x), it follows that F(x) in Pos.
    }
    Hence (s1): if x in Domain(F) then F(x) in Pos.
}
Hence (all): for every x, if x in Domain(F) then F(x) in Pos.

-- there exists, read off through F(...).
Assume (pf): F(G(a)) in Pos.
From (pf), it follows that there exists y such that F(y) in Pos as (ex).
Therefore there exists w such that F(w) in Pos.
