-- `there exists`: exhibit a witness, then take a witness out.
-- The conclusion leaving the scope must not mention the witness.

Let alice be an entity.
Let P, Q be sets.

Rule (pq): for every z, if z in P then z in Q.
Assume (h): alice in P.

From (h), it follows that there exists z such that z in P as (ex).
Therefore there exists z such that z in P.

Take w from (ex) {
    By rule (pq) applied to (w), it follows that w in Q as (wq).
    From (wq), it follows that there exists z such that z in Q as (exq).
}
Hence (r): there exists z such that z in Q.

Therefore there exists z such that z in Q.
