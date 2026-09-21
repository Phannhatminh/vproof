# Ngôn ngữ V — tài liệu tham khảo

Tài liệu này liệt kê những gì viết được trong một file `.v`. Thiết kế và lý do nằm ở
[`architecture.md`](../architecture.md); ở đây chỉ có cú pháp và nghĩa. Bản tiếng Anh:
[`language.en.md`](language.en.md).

Mọi khối mã đánh dấu `v` dưới đây là một chương trình đầy đủ, và `tests/test_examples.cpp`
chạy hết chúng mỗi lần build. Khối không đánh dấu chỉ là mảnh minh hoạ.

## Chạy

```
cmake -B build -S . && cmake --build build
./build/vproof examples/boss.v
ctest --test-dir build
```

Cần GMP (`brew install gmp`). Chương trình in ra kết quả từng dòng `Therefore`, cây lý do
của từng dòng `Why`, và thoát với mã khác 0 nếu có kiểm tra sai hoặc có bước không đi được.

## Hình dạng một file

- Mỗi câu kết thúc bằng dấu chấm `.`, trừ câu mở scope kết thúc bằng `{`.
- Comment bắt đầu bằng `--` và chạy tới hết dòng.
- Nhãn viết trong ngoặc, có thể nhiều chữ: `(h1)`, `(eq comp)`, `(no vote)`.
- `∈` và `in`, `∉` và `notin` dùng thay nhau.
- Câu được đọc và chạy lần lượt: một tên chỉ dùng được sau câu khai báo nó.

Trước mọi file, `lib/prelude.v` được nạp. Nó khai `SET`, `RELATION`, `MAP`, `Domain`, `Eq`,
`Function`, `TotalOn`, `Approx` cùng các luật của chúng.

## Khai báo

```
Let alice, bob be entities.
Let A, B be sets.
Let Boss be a relation.
Let F be a map.
Let t = (alice, bob).
```

`Let` luôn tạo đối tượng **mới**: khai báo cùng một tên hai lần là hai đối tượng. Chữ sau `be`
không đặt ra loại nào — nó ghi một fact:

| viết | ghi thêm |
|---|---|
| `be a set` / `be sets` | `x ∈ SET` |
| `be a relation` | `x ∈ RELATION` |
| `be a map` | `x ∈ MAP` |
| `be an entity`, `be objects`, … | không gì cả |

`Let t = (a, b).` tạo đối tượng `t` và ghi `(t, (a, b)) ∈ Defined` — **không** ghi vào `Eq`.
Luật `(eq from definition)` của prelude nối nó sang `Eq` khi cần. Các thành phần phải là tên.

## Term

| viết | là gì |
|---|---|
| `alice` | tên đã khai báo, hoặc biến của một `for every` |
| `3`, `-7`, `1/3`, `0.1` | số hữu tỉ chính xác; `0.1` là đúng `1/10` |
| `(a, b)`, `(a, (b, c), 5)` | tuple — cùng thành phần thì cùng đối tượng |
| `a + b`, `a - b`, `a * b`, `a / b`, `x ^ 2` | biểu thức — một term riêng, **không** tự tính |
| `F(a)` | giá trị của hàm `F` tại `a`, sau khi đã có `Apply F to a.` |
| `[A]` | đại diện của mệnh đề `A` |
| `[(h)]` | đại diện của mệnh đề mang nhãn `h` |

`2 + 3` là tuple `(Plus, 2, 3)`, khác đối tượng `5`. Nối hai cái là việc của `Compute` hoặc
`Simplify`, không phải của parser.

## Mệnh đề

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

Ngoặc `( … )` để nhóm. Độ ưu tiên từ lỏng tới chặt: `iff`, `or`, `and`.

Ba chỗ cần biết:

- `a <= b` là cách viết của `(a, b) in LessEq`; `a < b` của `(a, b) in Less`; `>` và `>=`
  viết ngược lại. Không có loại mệnh đề so sánh riêng.
- `for every x in S, A` là viết tắt của `for every x, if x in S then A`, với `S` là một tên
  cố định. Muốn lượng từ trên cả set thì viết `for every x, S, if x in S then A`.
- `t notin S`, `not (t in S)` là hai mệnh đề khác nhau. `A or B` và `B or A` cũng vậy.

## Đặt ra

```
Assume (h): alice in A.
Rule (r): for every x, if x in A then x in B.
Assume (h).
```

`Assume` và `Rule` làm cùng một việc: cất mệnh đề vào thế giới với lý do *đặt ra ở dòng n*.
Mọi mệnh đề đã cất đều dùng được như luật. `Assume (h).` không viết lại gì — nó đặt ra đúng
mệnh đề đã mang nhãn `h`, ví dụ điều kiện do `Simplify … as (h)` sinh ra.

## Áp luật

```
By rule (r) applied to (alice), it follows that alice in B.
By rule (qs), it follows that t in S as (s1).
```

Thế các đối số vào những biến ngoài cùng của luật theo thứ tự, tra từng tiền đề, rồi ghi kết
luận. **Tra chứ không tìm**: thiếu tiền đề nào thì bước không đi được và báo đúng tiền đề đó.
Tiền đề nối bằng `and` được tra từng cái. Kết luận viết ra phải khớp đúng kết luận của luật.
Đối số là term bất kỳ, kể cả `[A]` và biểu thức. `as (nhãn)` đặt nhãn cho kết luận.

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

## Bước không cần scope

`From` chọn bước theo hình dạng của tiền đề và kết luận:

| viết | bước |
|---|---|
| `From (a), (b), it follows that A and B.` | đưa `and` vào |
| `From (ab), it follows that A.` | dùng `and` — lấy một vế |
| `From (a), it follows that A or B.` | đưa `or` vào — vế kia chọn tự do |
| `From (f), (g), it follows that A iff B.` | đưa `iff` vào từ hai chiều `if … then` |
| `From (e), it follows that if A then B.` | dùng `iff` — lấy một chiều |
| `From (a), it follows that there exists x such that P(x).` | đưa `there exists` vào |

Với `there exists`, nhân chứng đọc ra bằng khớp mẫu giữa tiền đề và thân kết luận, không thử
từng đối tượng.

`Absurd from` chỉ ra vô lý bên trong một scope:

```
Absurd from (p), (np).             -- np là not (p), hoặc cùng một ô có cả hai cờ
Absurd from (or), (na), (nb).      -- A or B, not (A), not (B)
```

Chỉ ra vô lý **không nổ**: nó chỉ đánh dấu scope để lúc thoát dựng được `not (…)`. Dạng ba
tiền đề là cách duy nhất để dùng `or` — không có chia trường hợp.

## Scope

```
Suppose (h): A {  …  }  Hence (r): if A then B.
Suppose (h): A {  …  Absurd from …  }  Hence (r): not (A).
Take x {  …  }  Hence (r): for every x, P(x).
Take x with x in S {  …  }  Hence (r): for every x in S, P(x).
Take w from (ex) {  …  }  Hence (r): C.
```

Vào scope thì đóng một mốc; `Hence` kiểm mệnh đề thoát có đúng là thứ scope dựng được không,
quay về mốc, rồi mới cất nó. Mọi thứ ghi trong scope — giả định, đối tượng tạm, nhãn, tên —
đều biến mất. Với `Take w from`, kết luận không được nhắc tới `w`.

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

Phản chứng, với `or` dùng như bộ kích hoạt mâu thuẫn:

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

## Kiểm tra và giải thích

```
Therefore A.
Why A.
```

`Therefore` in `ok` nếu mệnh đề đang có, `SAI` nếu không — và "không có" nghĩa là chưa ai nói,
không nghĩa là sai. `Why` in cây lý do: mỗi nút là *đặt ra ở dòng n* hoặc *suy ra bằng luật
nào, dòng nào, thế gì, từ những tiền đề nào*.

## Tính toán

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

| câu | ghi vào |
|---|---|
| `Compute e.` | `(e, kết quả, sai số) ∈ Computed`; chia cho 0 thì `e ∈ NoValue` |
| `Compute a <= b.` | `(a, b) ∈ LessEq` hoặc `∉` — cả hai chiều |
| `Simplify e.` | `(e, dạng chuẩn) ∈ Simplified` |
| `Simplify e as (k).` | như trên; có triệt thì `(e, dạng chuẩn, [k]) ∈ SimplifiedIf`, nhãn `k` là điều kiện |
| `Expand (a, b, c) as t.` | `(1, a)`, `(2, b)`, `(3, c)` vào `t`; `((a, b, c), t) ∈ Expanded` |
| `Apply F to a.` | tạo `F(a)`, ghi `(a, F(a)) ∈ F`; cần `a in Domain(F)` |
| `Apply F to a as X.` | dạng đặt ra: `F(a)` là `X`, không tra miền |
| `Instantiate (r) at t.` | `([r], t, [thể hiện]) ∈ Instance` |

Không có quan hệ nào ở đây tự nối sang `Eq`. Nối là một luật người viết tự nạp, và nạp là
tuyên bố tin máy hay tin tầng đại số:

```v
Rule (eq comp): for every a, b, if (a, b, 0) in Computed then (a, b) in Eq.

Compute 0.1 + 0.2.
By rule (eq comp) applied to (0.1 + 0.2, 3/10), it follows that (0.1 + 0.2, 3/10) in Eq.
Therefore (0.1 + 0.2, 3/10) in Eq.

Compute 4 <= 3.
Therefore (4, 3) notin LessEq.
```

Điều kiện của một lần triệt ước phải được xác lập trước khi dùng:

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

Áp hàm:

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

`F(x)` với `x` là biến của luật hiện chưa viết được; trong luật hãy dùng `(x, y) in F`.

## Mệnh đề là đối tượng

`[A]` là đối tượng đại diện cho mệnh đề `A`, và `A` đang có khi và chỉ khi `[A] in Holds`.
Ghi được một chiều thì chiều kia theo. Đại diện có cấu trúc — `(Mem, t, S)`, `(Not, a)`,
`(All, a)` với biến buộc là `(Var, k)` — nên luật về mệnh đề viết được như luật thường:

```v
Let t be an entity.
Let S be a set.

Rule (dne): for every p, if (Not, (Not, p)) in Holds then p in Holds.

Assume (h): not (not (t in S)).
Therefore [not (not (t in S))] in Holds.

By rule (dne) applied to ([t in S]), it follows that [t in S] in Holds.
Therefore t in S.
```

Đại diện chỉ sinh ra khi được gọi tên, nên phải viết `[A]` ra ở đâu đó trước khi luật tra nó.

## Lý thuyết

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

`Theory` chỉ ghi lại, không chạy. `Import … as Alias with (X := Y, …)` chạy lại thân: tên
được gán trỏ vào đối tượng có sẵn, tên khai báo bên trong mà không gán thành `Alias_tên`, nhãn
thành `(Alias nhãn)`. `Import` lồng trong `Theory` được. Import lại đúng cặp (lý thuyết, ánh
xạ) là không làm gì.

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

Chữ trong mẫu mà vế phải dùng tới là lỗ; chữ còn lại là chữ cố định. Vế phải là một mệnh đề
không chứa lượng từ, và tên trong nó được quy về đối tượng ngay lúc khai báo — nên không bị
biến của luật bắt mất. Hai mẫu cùng hình dạng phân biệt bằng
`where A in Vectors, B in Vectors`; cùng khớp mà không phân biệt được thì báo lỗi.

## Tên có sẵn

**Của cơ chế** — cơ chế ghi vào, lý thuyết đọc:
`Holds`, `Column`, `Defined`, `Computed`, `NoValue`, `Less`, `LessEq`, `Simplified`,
`SimplifiedIf`, `Expanded`, `Instance`.

**Nhãn dựng đại diện và biểu thức:**
`Mem`, `NotMem`, `And`, `Or`, `Implies`, `Iff`, `Not`, `All`, `Ex`, `Var`,
`Plus`, `Minus`, `Times`, `Div`, `Pow`.

**Của prelude:** `SET`, `RELATION`, `MAP`, `Domain`, `Eq`, `Function`, `TotalOn`, `Approx`,
và các luật `(relation is set)`, `(map is relation)`, `(eq refl)`, `(eq symm)`,
`(eq trans)`, `(eq subst)`, `(eq from definition)`, `(function is relation)`,
`(function unique)`, `(total on)`, `(approx def)`, `(approx sum)`.

## Khi bước không đi được

| thông báo | nghĩa |
|---|---|
| `bước không đi được, chưa có: …` | một tiền đề chưa được xác lập |
| `kết luận viết ra không khớp: luật cho …` | luật cho ra một mệnh đề khác cái bạn viết |
| `chưa khai báo: x` | tên chưa có, hoặc đã biến mất khi scope đóng |
| `chưa có nhãn (h)` | nhãn chưa có, hoặc thuộc về một scope đã đóng |
| `kết luận nhắc tới nhân chứng …` | kết luận thoát `Take … from` còn chứa nhân chứng |
| `` `F(...)` chưa có `` | nhắc `F(a)` trước khi có `Apply F to a.` |
| `chưa xác lập a in Domain(F)` | áp hàm khi chưa biết đối số thuộc miền |
| `nhiều mẫu cùng khớp ở đây` | hai notation không phân biệt được |
