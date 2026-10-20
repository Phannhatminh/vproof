-- Ways of writing that the language takes. Each says something that could be said before
-- only another way, or not at all.

Let F, G be maps.
Let A, B, T, R, Vectors, Scalars, VecSum, NumSum be sets.
Let a, b, c, n be entities.

Apply Domain to F.
Assume (ha): a in Domain(F).
Apply F to a.

-- `in` takes any term, and a variable of an enclosing binder is that variable.
Rule (by set): for every Q, for every x in Q, (x, Q) in R.
Assume (hb): b in A.
By rule (by set) applied to (A, b), it follows that (b, A) in R.
Therefore (b, A) in R.

Rule (dom): for every x in Domain(F), x in T.
Take x with x in Domain(F) {
  By rule (dom) applied to (x), it follows that x in T.
}
Hence (all dom): for every x, if x in Domain(F) then x in T.

-- Several arguments, a function value applied again, and t[i].
Rule (pairs): for every x, y, F(x, y) in T.
Therefore for every u, v, F((u, v)) in T.
Assume (m): F(a) in MAP.
Apply Domain to F(a).
Assume (hab): b in Domain(F(a)).
Apply F(a) to b.
Rule (curried): for every x, y, F(x)(y) in T.
By rule (curried) applied to (a, b), it follows that F(a)[b] in T.
Therefore F(a)(b) in T.

-- Several variables after one `there exists`, and `not` without parentheses.
Assume (two): there exists x, y such that (x, y) in R.
Therefore there exists u such that there exists v such that (u, v) in R.
Assume (na): not c in A.
Therefore not (c in A).

-- A witness inside a tuple, and a body that does not mention the variable.
Assume (ab): (a, b) in R.
From (ab), it follows that there exists y such that (y, b) in R.
Therefore there exists y such that (y, b) in R.
From (hb), it follows that there exists y such that b in A.

-- Tuples of any terms after `Let t =`, and texts.
Let t = (a, (b, c)).
Therefore (t, (a, (b, c))) in Defined.
Assume (named): (n, "Alice Nguyen") in R.
Therefore (n, "Alice Nguyen") in R.

-- Notation with quantifiers and F(X); `where` on variables, from what is assumed.
Notation: "all of X are in Y" means for every z, if z in X then z in Y.
Assume (sub): all of A are in B.
By rule (sub) applied to (b), it follows that b in B.
Therefore b in B.
Notation: "img X" means F(X) in T.
Assume (img): img a.
Therefore F(a) in T.
Notation: "X plus Y is Z" means (X, Y, Z) in VecSum where X in Vectors, Y in Vectors.
Notation: "X plus Y is Z" means (X, Y, Z) in NumSum where X in Scalars, Y in Scalars.
Rule (vsum): for every u, v, w, if u in Vectors and v in Vectors then u plus v is w.
Therefore for every u, v, w, if u in Vectors and v in Vectors then (u, v, w) in VecSum.

-- A representative written out as a tuple is read off its shape.
Take s {
  Take Q {
    Suppose (h): (Mem, s, Q) in Holds {
      Therefore s in Q.
    }
  }
}
