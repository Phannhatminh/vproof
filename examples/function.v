-- Hàm là nội dung: một quan hệ cộng luật duy nhất. Prelude có sẵn.

Let F be a relation.
Let a, u, v be entities.
Let D be a set.

Assume (fn):  F in Function.
Assume (h1):  (a, u) in F.
Assume (h2):  (a, v) in F.

-- Hai giá trị tại cùng một đầu vào thì bằng nhau — đó là toàn bộ nghĩa của
-- "hàm", và nó là một luật chứ không phải một kiểm tra của cơ chế.
By rule (function unique) applied to (F, a, u, v), it follows that (u, v) in Eq.
Therefore (u, v) in Eq.
Why (u, v) in Eq.

-- Và F cũng là một quan hệ, rồi là một set — hai bước, đều suy ra được.
By rule (function is relation) applied to (F), it follows that F in RELATION.
By rule (relation is set) applied to (F), it follows that F in SET.
Therefore F in SET.

-- TotalOn: toàn phần trên D nghĩa là mỗi phần tử của D đều có ảnh.
Assume (tot): (F, D) in TotalOn.
By rule (total on) applied to (F, D),
    it follows that for every x, if x in D then there exists y such that (x, y) in F.
Therefore for every x, if x in D then there exists y such that (x, y) in F.
