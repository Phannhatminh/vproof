-- The mechanism checks nothing before writing a cell: `alice in 5` can be written.
-- But it records it, so theories have the data to judge for themselves.

Let A be a set.
Let alice be an entity.

Assume (h1): alice in A.
Therefore A in Column.

-- A meaningless statement can still be written. Nobody blocks it, and nobody hides it.
Assume (h2): alice in 5.
Therefore 5 in Column.

-- The theory writes its own rule. This is content, not mechanism.
Rule (column is set): for every c, if c in Column then c in SET.

-- For A the rule gives what was already known.
By rule (column is set) applied to (A), it follows that A in SET.
Therefore A in SET.

-- For 5 it gives `5 in SET`. For that to be a contradiction, someone has to say `5 notin
-- SET` and walk the chain here — the mechanism does not do it on its own.
By rule (column is set) applied to (5), it follows that 5 in SET.
Therefore 5 in SET.
Why 5 in Column.
