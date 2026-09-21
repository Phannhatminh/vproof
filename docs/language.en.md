# The V language — reference

This document lists what can be written in a `.v` file. Design and rationale live in
[`architecture.md`](../architecture.md) (Vietnamese); this page covers syntax and meaning only.
A Vietnamese version of this page is in [`language.md`](language.md).

Every code block tagged `v` below is a complete program, and `tests/test_examples.cpp` runs all
of them on every build. Untagged blocks are fragments.

## Running

```
cmake -B build -S . && cmake --build build
./build/vproof examples/boss.v
ctest --test-dir build
```

Requires GMP (`brew install gmp`). The program prints the result of each `Therefore`, the
reason tree of each `Why`, and exits non-zero if a check fails or a step cannot go through.

## Shape of a file

- Every statement ends with `.`, except statements that open a scope, which end with `{`.
- Comments start with `--` and run to the end of the line.
- Labels are written in parentheses and may have several words: `(h1)`, `(eq comp)`.
- `∈` / `in` and `∉` / `notin` are interchangeable.
- Statements are read and run in order: a name is usable only after the statement declaring it.

`lib/prelude.v` is loaded before every file. It declares `SET`, `RELATION`, `MAP`, `Domain`,
`Eq`, `Function`, `TotalOn`, `Approx` and their rules.

## Declarations

```
Let alice, bob be entities.
Let A, B be sets.
Let Boss be a relation.
Let F be a map.
Let t = (alice, bob).
```

`Let` always creates a **new** object: declaring the same name twice gives two objects. The
words after `be` do not introduce a type — they record a fact:

| written | also records |
|---|---|
| `be a set` / `be sets` | `x ∈ SET` |
| `be a relation` | `x ∈ RELATION` |
| `be a map` | `x ∈ MAP` |
| `be an entity`, `be objects`, … | nothing |

`Let t = (a, b).` creates `t` and records `(t, (a, b)) ∈ Defined` — **not** `Eq`. The prelude
rule `(eq from definition)` connects it to `Eq` when needed. The components must be names.

## Terms

| written | meaning |
|---|---|
| `alice` | a declared name, or a variable bound by `for every` |
| `3`, `-7`, `1/3`, `0.1` | exact rationals; `0.1` is exactly `1/10` |
| `(a, b)`, `(a, (b, c), 5)` | tuples — same components, same object |
| `a + b`, `a - b`, `a * b`, `a / b`, `x ^ 2` | expressions — a term of their own, **not** evaluated |
| `F(a)` | the value of `F` at `a`, once `Apply F to a.` has run |
| `[A]` | the object representing proposition `A` |
| `[(h)]` | the object representing the proposition labelled `h` |

`2 + 3` is the tuple `(Plus, 2, 3)`, a different object from `5`. Connecting them is the job of
`Compute` or `Simplify`, not of the parser.

## Propositions

```
t in S                     t notin S
a <= b    a < b            a >= b    a > b
A and B                    A or B
if A then B                A iff B
not (A)
for every x, A             for every x, y, z, A
for every x in S, A        for every x, y in S, A
there exists x such that A
there exists x in S such that A
```

Parentheses group. Precedence from loosest to tightest: `iff`, `or`, `and`.

Three things to know:

- `a <= b` is notation for `(a, b) in LessEq`; `a < b` for `(a, b) in Less`; `>` and `>=` are
  written the other way round. There is no separate kind of comparison proposition.
- `for every x in S, A` abbreviates `for every x, if x in S then A`, with `S` a fixed name. To
  quantify over the set too, write `for every x, S, if x in S then A`.
- `t notin S` and `not (t in S)` are different propositions. So are `A or B` and `B or A`.

## Stipulating

```
Assume (h): alice in A.
Rule (r): for every x, if x in A then x in B.
Assume (h).
```

`Assume` and `Rule` do the same thing: store the proposition with the reason *stipulated at
line n*. Every stored proposition can be used as a rule. `Assume (h).` restates nothing — it
stipulates exactly the proposition already labelled `h`, for example a condition produced by
`Simplify … as (h)`.

## Applying a rule

```
By rule (r) applied to (alice), it follows that alice in B.
By rule (qs), it follows that t in S as (s1).
```

Substitutes the arguments for the rule's outermost variables in order, looks up each premise,
then records the conclusion. **Lookup, not search**: if a premise is missing the step does not
go through and the missing premise is reported. Premises joined by `and` are looked up one by
one. The conclusion you write must match the rule's conclusion exactly. Arguments are any
terms, including `[A]` and expressions. `as (label)` labels the conclusion.

```v
Let alice, bob, carol be entities.
Let Boss be a relation.

Rule (join): for every x, y, z,
    if (x, y) in Boss and (y, z) in Boss then (x, z) in Boss.

Assume (h1): (alice, bob) in Boss.
Assume (h2): (bob, carol) in Boss.

By rule (join) applied to (alice, bob, carol), it follows that (alice, carol) in Boss.
Therefore (alice, carol) in Boss.
Why (alice, carol) in Boss.
```

## Steps without a scope

`From` picks the step from the shape of the premises and the conclusion:

| written | step |
|---|---|
| `From (a), (b), it follows that A and B.` | `and` introduction |
| `From (ab), it follows that A.` | `and` elimination — take one side |
| `From (a), it follows that A or B.` | `or` introduction — the other side is free |
| `From (f), (g), it follows that A iff B.` | `iff` introduction from two implications |
| `From (e), it follows that if A then B.` | `iff` elimination — take one direction |
| `From (a), it follows that there exists x such that P(x).` | `there exists` introduction |

For `there exists`, the witness is read off by matching the premise against the body of the
conclusion — no trying objects one by one.

`Absurd from` points out an absurdity inside a scope:

```
Absurd from (p), (np).             -- np is not (p), or the same cell has both flags
Absurd from (or), (na), (nb).      -- A or B, not (A), not (B)
```

Pointing out an absurdity **does not explode**: it only marks the scope so that `not (…)` can be
built on exit. The three-premise form is the only way to use `or` — there is no case split.

## Scopes

```
Suppose (h): A {  …  }  Hence (r): if A then B.
Suppose (h): A {  …  Absurd from …  }  Hence (r): not (A).
Take x {  …  }  Hence (r): for every x, P(x).
Take x with x in S {  …  }  Hence (r): for every x in S, P(x).
Take w from (ex) {  …  }  Hence (r): C.
```

Entering a scope sets a mark; `Hence` checks that the stated proposition is what this scope can
produce, rolls back to the mark, then stores it. Everything written inside — the assumption,
temporary objects, labels, names — disappears. With `Take w from`, the conclusion must not
mention `w`.

```v
Let A, B, C be sets.

Rule (ab): for every x, if x in A then x in B.
Rule (bc): for every y, if y in B then y in C.

Take u with u in A {
    By rule (ab) applied to (u), it follows that u in B.
    By rule (bc) applied to (u), it follows that u in C.
}
Hence (ac): for every x, if x in A then x in C.

Let alice be an entity.
Assume (h): alice in A.
By rule (ac) applied to (alice), it follows that alice in C.
Therefore alice in C.
```

Proof by contradiction, with `or` used as a contradiction trigger:

```v
Let t be an entity.
Let P, Q, R, S be sets.

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

Therefore not (not (t in S)).
```

## Checking and explaining

```
Therefore A.
Why A.
```

`Therefore` prints `ok` if the proposition holds and `SAI` (wrong) if not — and "does not hold"
means nobody has said it, not that it is false. `Why` prints the reason tree: every node is
either *stipulated at line n* or *derived by which rule, at which line, with which bindings,
from which premises*.

## Computation

```
Compute 100 + 101.
Compute 3 <= 5.
Simplify (x + 1) * (x - 1).
Simplify (x^2 - 1) / (x - 1) as (nz).
Expand (a, b, c) as t3.
Apply Domain to F.
Apply F to a.
Instantiate (r) at alice as (inst).
```

| statement | records |
|---|---|
| `Compute e.` | `(e, result, error) ∈ Computed`; division by zero gives `e ∈ NoValue` |
| `Compute a <= b.` | `(a, b) ∈ LessEq` or `∉` — both directions |
| `Simplify e.` | `(e, normal form) ∈ Simplified` |
| `Simplify e as (k).` | as above; if something was cancelled, `(e, normal form, [k]) ∈ SimplifiedIf` and label `k` is the condition |
| `Expand (a, b, c) as t.` | `(1, a)`, `(2, b)`, `(3, c)` into `t`; `((a, b, c), t) ∈ Expanded` |
| `Apply F to a.` | creates `F(a)`, records `(a, F(a)) ∈ F`; needs `a in Domain(F)` |
| `Apply F to a as X.` | stipulating form: `F(a)` is `X`, domain not checked |
| `Instantiate (r) at t.` | `([r], t, [instance]) ∈ Instance` |

None of these relations connects to `Eq` by itself. The connection is a rule you load yourself,
and loading it is a declaration that you trust the machine, or the algebra layer:

```v
Rule (eq comp): for every a, b, if (a, b, 0) in Computed then (a, b) in Eq.

Compute 0.1 + 0.2.
By rule (eq comp) applied to (0.1 + 0.2, 3/10), it follows that (0.1 + 0.2, 3/10) in Eq.
Therefore (0.1 + 0.2, 3/10) in Eq.

Compute 4 <= 3.
Therefore (4, 3) notin LessEq.
```

The condition of a cancellation must be established before it is used:

```v
Let x be an entity.

Rule (eq simp if): for every u, v, k,
    if (u, v, k) in SimplifiedIf and k in Holds then (u, v) in Eq.

Simplify (x^2 - 1) / (x - 1) as (nz).
Assume (nz).

By rule (eq simp if) applied to ((x^2 - 1)/(x - 1), 1 + x, [(nz)]),
    it follows that ((x^2 - 1)/(x - 1), 1 + x) in Eq.
Therefore ((x^2 - 1)/(x - 1), 1 + x) in Eq.
```

Function application:

```v
Let F be a map.
Let a be an entity.
Let Positives be a set.

Apply Domain to F.
Assume (h): a in Domain(F).
Apply F to a.

Assume (p): F(a) in Positives.
Therefore (a, F(a)) in F.
```

`F(x)` with `x` a rule variable cannot be written yet; inside a rule, use `(x, y) in F`.

## Propositions as objects

`[A]` is the object representing proposition `A`, and `A` holds exactly when `[A] in Holds`.
Writing either side makes the other follow. Representatives are structured — `(Mem, t, S)`,
`(Not, a)`, `(All, a)` with bound variables as `(Var, k)` — so rules about propositions are
ordinary rules:

```v
Let t be an entity.
Let S be a set.

Rule (dne): for every p, if (Not, (Not, p)) in Holds then p in Holds.

Assume (h): not (not (t in S)).
Therefore [not (not (t in S))] in Holds.

By rule (dne) applied to ([t in S]), it follows that [t in S] in Holds.
Therefore t in S.
```

A representative only comes into existence when it is named, so `[A]` must be written somewhere
before a rule looks it up.

## Theories

```v
Theory Preorder {
    Let Carrier be a set.
    Let Below be a relation.
    Rule (refl):  for every x, if x in Carrier then (x, x) in Below.
    Rule (trans): for every x, y, z,
        if (x, y) in Below and (y, z) in Below then (x, z) in Below.
}

Let People be a set.
Let Older be a relation.
Import Preorder as Age with (Carrier := People, Below := Older).

Let ann, ben, cam be entities.
Assume (h1): (ann, ben) in Older.
Assume (h2): (ben, cam) in Older.
By rule (Age trans) applied to (ann, ben, cam), it follows that (ann, cam) in Older.
Therefore (ann, cam) in Older.
```

`Theory` only records; it does not run. `Import … as Alias with (X := Y, …)` replays the body:
mapped names point at existing objects, names declared inside and left unmapped become
`Alias_name`, labels become `(Alias label)`. `Import` may appear inside a `Theory`. Importing
the same (theory, mapping) pair again does nothing.

## Notation

```v
Let Boss be a relation.
Let alice, bob, carol be entities.

Notation: "A manages B" means (A, B) in Boss.

Assume (h1): alice manages bob.
Assume (h2): bob manages carol.

Rule (join): for every x, y, z,
    if x manages y and y manages z then x manages z.
By rule (join) applied to (alice, bob, carol), it follows that alice manages carol.
Therefore (alice, carol) in Boss.
```

Template words used by the right-hand side are holes; the others are fixed words. The
right-hand side is a proposition without quantifiers, and the names in it are resolved to
objects when the notation is declared — so a rule variable cannot capture them. Two templates
with the same shape are told apart with `where A in Vectors, B in Vectors`; if both match and
cannot be told apart, it is an error.

## Built-in names

**Mechanism relations** — written by the mechanism, read by theories:
`Holds`, `Column`, `Defined`, `Computed`, `NoValue`, `Less`, `LessEq`, `Simplified`,
`SimplifiedIf`, `Expanded`, `Instance`.

**Tags for representatives and expressions:**
`Mem`, `NotMem`, `And`, `Or`, `Implies`, `Iff`, `Not`, `All`, `Ex`, `Var`,
`Plus`, `Minus`, `Times`, `Div`, `Pow`.

**From the prelude:** `SET`, `RELATION`, `MAP`, `Domain`, `Eq`, `Function`, `TotalOn`, `Approx`,
and the rules `(relation is set)`, `(map is relation)`, `(eq refl)`, `(eq symm)`, `(eq trans)`,
`(eq subst)`, `(eq from definition)`, `(function is relation)`, `(function unique)`,
`(total on)`, `(approx def)`, `(approx sum)`.

## When a step does not go through

Error messages are in Vietnamese.

| message | meaning |
|---|---|
| `bước không đi được, chưa có: …` | a premise has not been established |
| `kết luận viết ra không khớp: luật cho …` | the rule yields a different proposition from the one written |
| `chưa khai báo: x` | the name does not exist, or vanished when its scope closed |
| `chưa có nhãn (h)` | the label does not exist, or belongs to a closed scope |
| `kết luận nhắc tới nhân chứng …` | the conclusion leaving `Take … from` still mentions the witness |
| `` `F(...)` chưa có `` | `F(a)` used before `Apply F to a.` |
| `chưa xác lập a in Domain(F)` | applying a function before the argument is known to be in its domain |
| `nhiều mẫu cùng khớp ở đây` | two notations cannot be told apart |
