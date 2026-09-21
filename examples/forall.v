-- Đưa `for every` vào bằng scope: lấy một đối tượng chưa ai nói gì,
-- chứng minh về nó, rồi thoát.

Let A, B, C be sets.

Rule (ab): for every x, if x in A then x in B.
Rule (bc): for every y, if y in B then y in C.

Take u with u in A {
    By rule (ab) applied to (u), it follows that u in B as (t1).
    By rule (bc) applied to (u), it follows that u in C as (t2).
}
Hence (ac): for every x, if x in A then x in C.

Therefore for every x, if x in A then x in C.

-- Và dùng lại nó như một luật bình thường.
Let alice be an entity.
Assume (h): alice in A.
By rule (ac) applied to (alice), it follows that alice in C.
Therefore alice in C.
Why alice in C.
