-- Mệnh đề là đối tượng, kể cả mệnh đề có lượng từ. Nên luật nhìn vào ruột
-- một mệnh đề có lượng từ viết được, và nó là luật thường.

Let A, B be sets.
Let alice be an entity.

Rule (ab): for every x, if x in A then x in B.
Assume (ha): alice in A.

-- Đại diện của một mệnh đề có lượng từ có cấu trúc: biến buộc thành (Var, 0).
Therefore [for every x, if x in A then x in B] in Holds.

-- ∀-elim viết thành luật trên đại diện, thay vì là một thao tác của cơ chế.
Rule (all elim): for every p, t, q,
    if p in Holds and (p, t, q) in Instance then q in Holds.

Instantiate (ab) at alice as (inst).
By rule (all elim) applied to ([for every x, if x in A then x in B], alice, [(inst)]),
    it follows that [(inst)] in Holds.
Therefore [(inst)] in Holds.

-- Và mệnh đề thật cũng có theo, nhờ đồng bộ ngược của Holds.
Therefore if alice in A then alice in B.
By rule (inst), it follows that alice in B.
Therefore alice in B.
Why alice in B.
