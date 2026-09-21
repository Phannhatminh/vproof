-- A classical library written over representatives, then used to finish the cases.v
-- example.
-- `[A]` is the object representing proposition A. `Holds` is a mechanism relation.

Let t be an entity.
Let P, Q, R, S be sets.

-- Double negation. One rule, for every proposition.
Rule (dne): for every p, if (Not, (Not, p)) in Holds then p in Holds.

Assume (p):   t in P.
Assume (pqr): if t in P then t in Q or t in R.
Assume (qs):  if t in Q then t in S.
Assume (rs):  if t in R then t in S.

Suppose (ns): not (t in S) {
    Suppose (q): t in Q {
        By rule (qs), it follows that t in S as (s1).
        Absurd from (s1), (ns).
    }
    Hence (nq): not (t in Q).

    Suppose (r): t in R {
        By rule (rs), it follows that t in S as (s2).
        Absurd from (s2), (ns).
    }
    Hence (nr): not (t in R).

    By rule (pqr), it follows that t in Q or t in R as (qr).
    Absurd from (qr), (nq), (nr).
}
Hence (nns): not (not (t in S)).

-- Name the representative of the conclusion just obtained, then apply double negation in
-- one line.
Therefore [not (not (t in S))] in Holds.
By rule (dne) applied to ([t in S]), it follows that [t in S] in Holds.

Therefore t in S.
Why t in S.
