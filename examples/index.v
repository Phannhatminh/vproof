-- Index(N) and comparisons. `a <= b` is notation for `(a, b) in LessEq`.

Let Naturals be a set.
Let Index be objects.

Assume (n1): 1 in Naturals.
Assume (n2): 2 in Naturals.
Assume (n3): 3 in Naturals.
Assume (n4): 4 in Naturals.

Apply Domain to Index as Naturals.
Apply Index to 3.

Rule (index def): for every k,
    if k in Naturals and 1 <= k and k <= 3 then k in Index(3).

Compute 1 <= 2.
Compute 2 <= 3.
Compute 1 <= 4.
Compute 4 <= 3.

By rule (index def) applied to (2), it follows that 2 in Index(3).
Therefore 2 in Index(3).

Therefore (4, 3) notin LessEq.
Why 2 in Index(3).
