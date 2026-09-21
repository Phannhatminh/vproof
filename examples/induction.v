-- Induction is not a mechanism statement. It is an axiom of a theory, and it quantifies
-- over SET — sets are objects, so that is first-order quantification. v0.3 hard-wired
-- induction into the machine; here it is one line the user writes.

Let N, P be sets.

Assume (zero): 0 in N.
Rule (succ): for every n, if n in N then n + 1 in N.

Rule (induction): for every S,
    if 0 in S and (for every n, if n in N and n in S then n + 1 in S)
    then (for every n, if n in N then n in S).

-- Assumptions about P: it contains 0, and is closed under adding 1.
Assume (base): 0 in P.
Rule (closed): for every m, if m in P then m + 1 in P.

-- The induction step is proved with scopes: take an n nobody has said anything about,
-- assume the antecedent, derive the consequent, then leave twice.
Take n {
    Suppose (h): n in N and n in P {
        From (h), it follows that n in P as (np).
        By rule (closed) applied to (n), it follows that n + 1 in P.
    }
    Hence (s1): if n in N and n in P then n + 1 in P.
}
Hence (step): for every n, if n in N and n in P then n + 1 in P.

-- Put the two parts together, then apply the induction axiom to P itself.
From (base), (step), it follows that
    0 in P and (for every n, if n in N and n in P then n + 1 in P) as (both).

By rule (induction) applied to (P),
    it follows that for every n, if n in N then n in P.
Therefore for every n, if n in N then n in P.

-- And use it: 0 is in N so 0 is in P; then 1 as well.
By rule (induction) applied to (P), it follows that for every n, if n in N then n in P as (ind).
By rule (ind) applied to (0), it follows that 0 in P.
By rule (succ) applied to (0), it follows that 0 + 1 in N.
By rule (ind) applied to (0 + 1), it follows that 0 + 1 in P.
Therefore 0 + 1 in P.
Why 0 + 1 in P.
