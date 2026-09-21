-- Thư viện cổ điển viết trên đại diện, rồi dùng để đóng ví dụ cases.v.
-- `[A]` là đối tượng đại diện cho mệnh đề A. `Holds` là quan hệ của cơ chế.

Let t be an entity.
Let P, Q, R, S be sets.

-- Phủ định kép. Một luật, áp cho mọi mệnh đề.
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

-- Gọi tên đại diện của kết luận vừa có, rồi áp phủ định kép một dòng.
Therefore [not (not (t in S))] in Holds.
By rule (dne) applied to ([t in S]), it follows that [t in S] in Holds.

Therefore t in S.
Why t in S.
