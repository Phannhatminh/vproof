-- Bắc cầu trên một quan hệ, viết bằng luật và áp luật.

Let alice, bob, carol be entities.
Let Boss be a relation.

Rule (join): for every x, y, z,
    if (x, y) in Boss and (y, z) in Boss then (x, z) in Boss.

Assume (h1): (alice, bob) in Boss.
Assume (h2): (bob, carol) in Boss.

By rule (join) applied to (alice, bob, carol), it follows that (alice, carol) in Boss.

Therefore (alice, carol) in Boss.
Why (alice, carol) in Boss.
