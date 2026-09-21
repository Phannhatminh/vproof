-- Quy nạp không phải một câu lệnh của cơ chế. Nó là một tiên đề của lý thuyết,
-- và nó lượng từ trên SET — mà set là đối tượng, nên đó là lượng từ bậc nhất.
-- Bản v0.3 đóng cứng quy nạp vào máy; ở đây nó là một dòng người dùng viết.

Let N, P be sets.

Assume (zero): 0 in N.
Rule (succ): for every n, if n in N then n + 1 in N.

Rule (induction): for every S,
    if 0 in S and (for every n, if n in N and n in S then n + 1 in S)
    then (for every n, if n in N then n in S).

-- Giả thiết về P: chứa 0, và đóng dưới phép cộng 1.
Assume (base): 0 in P.
Rule (closed): for every m, if m in P then m + 1 in P.

-- Bước quy nạp chứng minh bằng scope: lấy một n chưa ai nói gì, giả định vế
-- đầu, suy ra vế sau, rồi thoát hai lần.
Take n {
    Suppose (h): n in N and n in P {
        From (h), it follows that n in P as (np).
        By rule (closed) applied to (n), it follows that n + 1 in P.
    }
    Hence (s1): if n in N and n in P then n + 1 in P.
}
Hence (step): for every n, if n in N and n in P then n + 1 in P.

-- Ghép hai vế lại rồi áp tiên đề quy nạp cho chính P.
From (base), (step), it follows that
    0 in P and (for every n, if n in N and n in P then n + 1 in P) as (both).

By rule (induction) applied to (P),
    it follows that for every n, if n in N then n in P.
Therefore for every n, if n in N then n in P.

-- Và dùng nó: 0 thuộc N nên 0 thuộc P; rồi 1 cũng vậy.
By rule (induction) applied to (P), it follows that for every n, if n in N then n in P as (ind).
By rule (ind) applied to (0), it follows that 0 in P.
By rule (succ) applied to (0), it follows that 0 + 1 in N.
By rule (ind) applied to (0 + 1), it follows that 0 + 1 in P.
Therefore 0 + 1 in P.
Why 0 + 1 in P.
