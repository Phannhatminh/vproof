-- F(x) inside a rule. A rule is kept as written: F(x) with x a variable is a term of the
-- rule, not an object. It is well formed only where `x in Domain(F)` is assumed — the `if`
-- part around it, the left side of an `and` before it, the `in S` of its binder — the same
-- check as for F(a), done on the form. Applying the rule to a turns F(x) into the object
-- F(a), made by the rule step when it does not exist yet, with the same domain check as
-- `Apply F to a`.

Let F, G be maps.
Let Pos, Good be sets.
Let a be an entity.

Apply Domain to F.
Apply Domain to G.
Assume (ha): a in Domain(G).
Apply G to a.
Assume (hga): G(a) in Domain(F).

Rule (pos): for every x, if x in Domain(F) then F(x) in Pos.
Rule (good): for every x,
    if x in Domain(G) and G(x) in Domain(F) and F(G(x)) in Pos then x in Good.

-- F(G(a)) does not exist before this step; the step makes it.
By rule (pos) applied to (G(a)), it follows that F(G(a)) in Pos.
By rule (good) applied to (a), it follows that a in Good.
Therefore a in Good.
Therefore (G(a), F(G(a))) in F.

-- The same rule written with another variable is the same proposition. In its
-- representative F(x) is (App, F, (Var, 0)).
Therefore for every z, if z in Domain(F) then F(z) in Pos.
Therefore [for every x, if x in Domain(F) then F(x) in Pos] in Holds.

-- The function can be a variable too. Domain(f) needs `f in MAP`, the same check one level
-- up: Domain(Domain) is MAP.
Rule (own): for every f, y, if f in MAP and y in Domain(f) and (y, f(y)) in f then f(y) in Good.
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
From (hga), (pf), it follows that G(a) in Domain(F) and F(G(a)) in Pos as (both).
From (both), it follows that there exists y such that y in Domain(F) and F(y) in Pos as (ex).
Therefore there exists w such that w in Domain(F) and F(w) in Pos.

-- Taking the witness out: F(w) is the object for the witness taken, with nothing stated
-- about it until `Apply F to w`.
Take w from (ex) {
    Therefore w in Domain(F) and F(w) in Pos as (wb).
    From (wb), it follows that w in Domain(F) as (wd).
    Apply F to w.
    Therefore (w, F(w)) in F as (wf).
    From (wd), (wf), it follows that w in Domain(F) and (w, F(w)) in F as (wboth).
    From (wboth), it follows that there exists v such that v in Domain(F) and (v, F(v)) in F as (some).
}
Hence (some): there exists v such that v in Domain(F) and (v, F(v)) in F.
