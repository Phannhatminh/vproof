#pragma once

namespace v {

// Source of `lib/prelude.v`, embedded so the binary stands on its own.
// Edit that file, then run `tools/embed_prelude.py` to refresh this one.
inline const char* kPrelude = R"VPRELUDE(
-- The minimal library. Written in V, loaded before every file.
-- Nothing here is mechanism: every line is a fact or a rule, exactly like anything a user
-- writes.

-- SET, RELATION, MAP are ordinary objects. Declared first, because `be a set` writes into
-- SET.
Let SET, RELATION, MAP be objects.

Assume (set is set):  SET in SET.
Assume (rel is set):  RELATION in SET.
Assume (map is set):  MAP in SET.

Rule (relation is set): for every r, if r in RELATION then r in SET.
Rule (map is relation): for every f, if f in MAP then f in RELATION.

-- Domain is a function, so Domain(F) is also the result of an application step. That chain
-- has to stop somewhere: it stops at Domain(Domain) = MAP, stipulated from the start, of
-- the same kind as SET in SET.
Let Domain be objects.
Apply Domain to Domain as MAP.

-- Eq: ONE basic equality relation, applying to every object. It is content, not a built-in
-- setting.
Let Eq be a relation.

Rule (eq refl):  for every a, (a, a) in Eq.
Rule (eq symm):  for every a, b, if (a, b) in Eq then (b, a) in Eq.
Rule (eq trans): for every a, b, c, if (a, b) in Eq and (b, c) in Eq then (a, c) in Eq.
Rule (eq subst): for every S, a, b, if (a, b) in Eq and a in S then b in S.

-- `Let t = (a, b)` writes into Defined; this rule is what connects it to Eq.
Rule (eq from definition): for every a, b, if (a, b) in Defined then (a, b) in Eq.

-- Function: a relation where every input has only one output. No special language support
-- needed — it is a set plus two rules.
Let Function be a set.
Let TotalOn be a relation.

Rule (function is relation): for every f, if f in Function then f in RELATION.

Rule (function unique): for every f, x, y, z,
    if f in Function and (x, y) in f and (x, z) in f then (y, z) in Eq.

Rule (total on): for every f, D,
    if (f, D) in TotalOn then (for every x, if x in D then there exists y such that (x, y) in f).

-- Approximation is a relation, not mechanism. The mechanism only declares "this is what I
-- return, this is the error I claim"; the word *approximate* does not appear there. The
-- definition below is one choice — write a different definition and you get a different
-- meaning, and the system has no opinion.
Let Approx be a relation.

Rule (approx def): for every u, v, k,
    if (u, v, k) in Computed then (u, v, k) in Approx.

Rule (approx sum): for every u, v, k, p, q, r, s, t,
    if (u, v, k) in Approx and (p, q, r) in Approx
       and (s, u + p, 0) in Computed and (t, v + q, 0) in Computed
    then (s, t, k + r) in Approx.

-- Note: the rules connecting `Simplified` / `SimplifiedIf` / `Computed` to `Eq` are NOT
-- here. Loading one is a declaration of trust in the machine's arithmetic, so it has to be
-- put in the author's own file:
--     Rule (eq comp): for every a, b, if (a, b, 0) in Computed then (a, b) in Eq.
-- The same goes for classical logic (explosion, excluded middle, double negation).
)VPRELUDE";

}  // namespace v
