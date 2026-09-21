-- Propositions are objects, quantified ones included. So rules that look inside a
-- quantified proposition can be written, and they are ordinary rules.

Let A, B be sets.
Let alice be an entity.

Rule (ab): for every x, if x in A then x in B.
Assume (ha): alice in A.

-- The representative of a quantified proposition is structured: the bound variable becomes
-- (Var, 0).
Therefore [for every x, if x in A then x in B] in Holds.

-- ∀-elimination written as a rule over representatives, instead of a mechanism operation.
Rule (all elim): for every p, t, q,
    if p in Holds and (p, t, q) in Instance then q in Holds.

Instantiate (ab) at alice as (inst).
By rule (all elim) applied to ([for every x, if x in A then x in B], alice, [(inst)]),
    it follows that [(inst)] in Holds.
Therefore [(inst)] in Holds.

-- And the real proposition follows too, through the reverse sync of Holds.
Therefore if alice in A then alice in B.
By rule (inst), it follows that alice in B.
Therefore alice in B.
Why alice in B.
