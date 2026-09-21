-- Đóng gói một lý thuyết, rồi thể hiện nó hai lần trên hai thứ khác nhau.

Theory Preorder {
    Let Carrier be a set.
    Let Below be a relation.

    Rule (refl):  for every x, if x in Carrier then (x, x) in Below.
    Rule (trans): for every x, y, z,
        if (x, y) in Below and (y, z) in Below then (x, z) in Below.
}

Let People, Tasks be sets.
Let Older, Before be relations.

Import Preorder as Age  with (Carrier := People, Below := Older).
Import Preorder as Plan with (Carrier := Tasks,  Below := Before).

-- Hai thể hiện, hai bộ luật, nhãn mang tên thể hiện.
Let ann, ben, cam be entities.
Assume (h1): (ann, ben) in Older.
Assume (h2): (ben, cam) in Older.

By rule (Age trans) applied to (ann, ben, cam), it follows that (ann, cam) in Older.
Therefore (ann, cam) in Older.

Let t1, t2 be entities.
Assume (h3): t1 in Tasks.
By rule (Plan refl) applied to (t1), it follows that (t1, t1) in Before.
Therefore (t1, t1) in Before.

-- Import lại đúng cặp đó là không làm gì.
Import Preorder as Age with (Carrier := People, Below := Older).

Why (ann, cam) in Older.

-- Tên khai báo bên trong mà không được gán thì thành tên riêng của thể hiện,
-- nên hai lần import không giẫm lên nhau.
Theory Pointed {
    Let Carrier be a set.
    Let basepoint be an entity.
    Assume (has): basepoint in Carrier.
}

Let A, B be sets.
Import Pointed as First  with (Carrier := A).
Import Pointed as Second with (Carrier := B).

Therefore First_basepoint in A.
Therefore Second_basepoint in B.

-- Và hai điểm đó là hai đối tượng khác nhau: không ai nói gì về quan hệ giữa
-- chúng, nên `(First_basepoint, Second_basepoint) in Eq` là chưa biết.

-- Lý thuyết lồng nhau: `Import` viết được bên trong một `Theory`, và tên của
-- thể hiện con mang tên thể hiện ngoài, nên hai lần import không giẫm nhau.
Theory Inner {
    Let Carrier be a set.
    Rule (refl): for every u, if u in Carrier then (u, u) in Eq.
}

Theory Outer {
    Let Base be a set.
    Import Inner as Sub with (Carrier := Base).
}

Let P, Q be sets.
Let p, q be entities.
Import Outer as One with (Base := P).
Import Outer as Two with (Base := Q).

Assume (hp): p in P.
Assume (hq): q in Q.
By rule (One Sub refl) applied to (p), it follows that (p, p) in Eq.
By rule (Two Sub refl) applied to (q), it follows that (q, q) in Eq.
Therefore (p, p) in Eq.
Therefore (q, q) in Eq.
