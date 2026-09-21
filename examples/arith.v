-- Cửa số học. Máy luôn trả kèm sai số; đẳng thức chỉ rơi ra khi sai số bằng 0.

-- Nạp dòng này là tuyên bố: tôi tin số học của máy.
Rule (eq comp): for every a, b, if (a, b, 0) in Computed then (a, b) in Eq.

Compute 100 + 101.
Compute 1/3 + 1/6.
Compute 0.1 + 0.2.
Compute 2 * 3 - 1.
Compute 1 / 0.

By rule (eq comp) applied to (100 + 101, 201), it follows that (100 + 101, 201) in Eq.
Therefore (100 + 101, 201) in Eq.

By rule (eq comp) applied to (1/3 + 1/6, 1/2), it follows that (1/3 + 1/6, 1/2) in Eq.
Therefore (1/3 + 1/6, 1/2) in Eq.

Therefore 1 / 0 in NoValue.
Why (100 + 101, 201) in Eq.
