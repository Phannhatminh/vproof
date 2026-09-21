# Kiến trúc vproof

Tài liệu ngôn ngữ — viết được những câu nào — nằm ở [`docs/language.md`](docs/language.md)
(tiếng Anh: [`docs/language.en.md`](docs/language.en.md)). File này là thiết kế và lý do.

Ghi 2026-09-20.

## Mở đầu

V là một ngôn ngữ lập trình; vproof là hệ thống chạy nó. Có mã nguồn, có parser,
có runtime thực thi. Viết một bài chứng minh trong V là viết một chương trình;
chạy nó là thực thi chương trình đó.

Cái được thực thi là một chuỗi suy nghĩ. V không phải công cụ kiểm tra xem điều gì
đúng. Nó định nghĩa một bước nghĩ là gì: nghĩ từ đâu, nghĩ ra cái gì. Chạy V là mô
phỏng việc nghĩ theo đúng định nghĩa đó, và "kiểm tra một bước" trong V chỉ có một
nghĩa — bước này có phải một bước nghĩ theo định nghĩa không.

Hai điều đó quyết định toàn bộ kiến trúc. Vì là ngôn ngữ, chữ phải tách khỏi cơ chế.
Vì mô phỏng việc nghĩ, cơ chế không được làm thay người nghĩ: không tự suy ra thứ
chưa ai nghĩ tới, không tự đoán, không tự phán xét, không giấu bước, không chặn một
suy nghĩ chỉ vì nó sai.

Doc này mô tả kiến trúc đó.

## Core và lý thuyết

Core gồm hai phần: **ngôn ngữ** và **runtime**. Đó là toàn bộ phần có sẵn.

**Lý thuyết** không phải phần thứ ba ngang hàng. Nó được dựng ra từ core — viết bằng
ngôn ngữ, sống trong runtime.

## Runtime

Runtime là một cái kho có trạng thái, cộng một bộ thao tác trên kho đó.

### Trạng thái nó giữ

1. **Đối tượng** — sinh ra bởi khai báo (mỗi lần một cái mới, không thành phần) hoặc
   bởi term có cấu trúc (tuple đồng nhất theo thành phần, số theo giá trị). Không có
   đường thứ ba, và không có gì làm hai đối tượng thành một.

2. **Ma trận membership** — một ô ứng với một cặp (vị trí phần tử, vị trí set), cả
   hai là đối tượng bất kỳ. Mỗi ô mang hai cờ độc lập: đã được nói ∈, đã được nói ∉.
   Cờ chỉ bật, không tắt, và bật ở bước nào thì mang dấu bước đó. Mỗi cờ mang lý do
   riêng của nó, giữ hết mọi lý do.

3. **Kho mệnh đề** — chỗ giữ mệnh đề đã được đặt ra và mệnh đề đã suy ra được. Ma
   trận membership là chỗ lưu có chỉ mục cho mệnh đề nguyên tử; những mệnh đề còn lại
   nằm ở kho này.

4. **Nhật ký** — danh sách mọi thao tác ghi đã xảy ra, theo đúng thứ tự xảy ra. Đây là
   thứ cho phép quay ngược lại một trạng thái cũ.

### Thao tác nó nhận

- Tạo một đối tượng.
- Ghi một cờ vào một ô, kèm lý do. Luôn thành công.
- Hỏi một cờ: đã bật chưa.
- Cất một mệnh đề, kèm lý do. Luôn thành công.
- Tra một mệnh đề: kho có đúng mệnh đề này không.
- Áp một luật: thế biến vào luật, tra từng tiền đề, ghi kết luận kèm lý do.
- Đóng một mốc trong nhật ký.
- Quay về một mốc: gỡ ngược mọi thao tác đã ghi sau mốc đó.

## Ma trận membership materialize được cái gì

Ma trận đánh chỉ mục theo cặp đối tượng, và mỗi ô chỉ có chỗ cho hai cờ. Muốn ghi thẳng
vào đó, một mệnh đề phải chỉ đúng một ô và phải dứt khoát về ô đó. Đúng hai hình dạng
mệnh đề thỏa điều kiện ấy:

- `t ∈ S` bật cờ ∈ của ô `(t, S)`.
- `t ∉ S` bật cờ ∉ của ô `(t, S)`.

Mọi mệnh đề ghép đều trượt khỏi điều kiện đó, mỗi cái vì một lý do riêng. `A or B` không
dứt khoát về ô nào — cả hai ô vẫn có thể chưa ai nói gì. `if A then B` không nói gì về
một ô cụ thể. `for every x, A` không chỉ một ô, và những ô nó nói tới có thể còn chưa tồn
tại. `there exists x such that A` có nói dứt khoát, nhưng không nói là ô nào. Phủ định
của một mệnh đề ghép thì không quy về một ô.

Trượt khỏi ma trận không có nghĩa là không bao giờ tới được ma trận. Nó chỉ có nghĩa là
không tới thẳng được. Mọi mệnh đề ghép đều tới được ma trận qua một bước biến nó thành
nguyên tử: `A or B` cộng với một bước loại trừ được một vế thì vế còn lại ghi được;
`if A then B` cộng với `A` đã có thì `B` ghi được; `for every x, A` cộng với một đối
tượng cụ thể thì cho ra một thể hiện nguyên tử; `there exists x such that A` cộng với
một bước chỉ ra nhân chứng thì cho ra mệnh đề nguyên tử về nhân chứng đó.

Thứ tới được ma trận không phải bản thân mệnh đề ghép, mà là cái mệnh đề đó sinh ra.
`A or B` không bao giờ nằm trong ma trận. Nó nằm trong kho mệnh đề, và ở đó nó làm nhiệm
vụ sinh ra ô.

## Loại

Biến không có loại. Mọi biến chạy trên tất cả đối tượng, và cùng một biến đứng được ở vị
trí phần tử lẫn vị trí set. Miền là một, và là bậc nhất: phần tử, set, quan hệ, tuple, số
đều là đối tượng, nên lượng từ trên set vẫn là lượng từ bậc nhất.

Loại không có sẵn trong runtime, vì loại cũng được dựng ra từ core. `SET` là một đối tượng
bình thường, không phải một hạng mục cơ chế biết trước. "S là một set" không phải thuộc
tính của S — nó là một fact, cờ ∈ của ô `(S, SET)`. `SET` cũng thuộc `SET`: ô `(SET, SET)`
bật cờ ∈, đặt ra từ đầu, và đó là chỗ cắt vòng để cột `SET` tồn tại được.

`Let A be a set.` không đặt ra một loại nào — nó ghi fact `A ∈ SET`. Tương tự `be a
relation` ghi `∈ RELATION`, `be a map` ghi `∈ MAP`. `be an entity` hay `be objects` chỉ
khai báo đối tượng, không ghi gì thêm, nên ô `(alice, SET)` vẫn là chưa ai nói gì — không
phải "alice không phải set".

Ma trận tự nó không hiểu cột của nó là set. Một cột được đánh chỉ mục bằng một đối tượng
bất kỳ, và runtime không tra gì trước khi ghi — `alice ∈ 5` viết được, đúng như mọi câu
ghép đúng cú pháp khác.

Cái runtime làm là **ghi lại**: mỗi lần một đối tượng được dùng làm cột, cơ chế ghi một
fact vào một quan hệ của riêng nó, `Column`. Nhờ đó lý thuyết có đủ dữ kiện để tự phán
xét — viết được luật kiểu "đã làm cột thì phải thuộc SET", và một bước chứng minh chỉ ra
chỗ vi phạm. Runtime cung cấp dữ kiện, lý thuyết cầm phán xét.

## Tuple và áp hàm

Cấu trúc duy nhất mà cơ chế bắt buộc phải dựng sẵn là **cặp** `(x, y)`. Lý do hẹp và cụ
thể: quan hệ là một set chứa các cặp, và hàm là một quan hệ, nên không có cặp thì không
dựng được gì ở trên. Cặp lồng được, vì thành phần của cặp là đối tượng bất kỳ.

Tuple liệt kê `(a, b, c)` có danh tính theo danh sách thành phần: cùng một dãy thành phần
thì cùng một đối tượng, viết ở đâu cũng vậy. Đây là chỗ nó khác đối tượng khai báo, vốn
mỗi lần khai báo lại ra một cái mới. Hệ quả cụ thể: `Let tuple1 = (alice, carol).` và
`Let tuple2 = (alice, carol).` tạo ba đối tượng — `tuple1`, `tuple2`, và tuple
`(alice, carol)`.

### Tuple độ dài N không phải primitive

`Tuple(N)` — tuple có độ dài là một đối tượng `N` — không nằm trong cơ chế. Nó là **hàm**
từ tập chỉ số vào đối tượng, và được dựng ra bằng nội dung, theo một chuỗi mà mỗi bước chỉ
dùng thứ đã dựng ở bước trước:

```
-- set
Assume SET ∈ SET.

-- quan hệ: set chứa cặp
Rule (relation is set): for every R in Relation, R ∈ SET.

-- hàm: quan hệ mà mỗi đầu vào chỉ một đầu ra
Rule (function is relation): for every f in Function, f ∈ Relation.
Rule (function unique): for every f in Function, for every x, y1, y2,
    if (x, y1) ∈ f and (x, y2) ∈ f then (y1, y2) ∈ Eq.
Rule (total on): for every f, D,
    if (f, D) ∈ TotalOn then (for every x in D, there exists y such that (x, y) ∈ f).

-- chỉ số
Rule (index def): for every N in Naturals, for every k,
    k ∈ Index(N) iff k ∈ Naturals and 1 <= k and k <= N.

-- tuple độ dài N
Rule (tuple def): for every N in Naturals, for every t,
    t ∈ Tuple(N) iff t ∈ Function
                 and (t, Index(N)) ∈ TotalOn
                 and (for every x, y, if (x, y) ∈ t then x ∈ Index(N)).
```

Hàm không cần ngôn ngữ hỗ trợ riêng. Nó là một set cộng hai luật, và cả hai luật là nội
dung người dùng nạp vào.

### Áp hàm

`Tuple(N)` là áp hàm `Tuple` vào `N`. `x(i)` là áp hàm `x` vào `i`, và lấy phần tử thứ `i`
của một tuple `t(i)` cũng vậy, vì tuple là hàm. (Cú pháp `t[i]` từng được bàn nhưng chưa cài;
hiện chỉ có dạng `F(a)`.)

Áp hàm **không** phải thao tác của cơ chế. Hàm, quan hệ, set đều dựng từ primitive, nên áp
hàm cũng vậy: nó là **cú pháp**, và nó dịch xuống các bước thường dùng luật của thư viện.

Tạo giá trị của một hàm là một bước nghĩ, nên nó phải là một bước viết ra. `F(a)` chỉ là
**tên** của đối tượng mà bước đó tạo ra; chưa có bước thì chưa có đối tượng nào mang tên ấy.

```
Apply F to a.
```

Về nghĩa, câu này là một lần áp luật cộng một lần lấy nhân chứng: từ `F ∈ Function` và
`(F, Domain(F)) ∈ TotalOn` suy ra `there exists y such that (a, y) ∈ F`, rồi lấy nhân chứng
ra. Về cài đặt, tầng ngôn ngữ làm thẳng mà không đi qua hai luật đó: nó tra xem `Domain(F)`
đã có chưa, tra ô `(a, Domain(F))` có cờ ∈ chưa — tra như tra mọi tiền đề — rồi tạo đối tượng
và ghi `(a, F(a)) ∈ F` với lý do *áp hàm*. Runtime không có thao tác áp hàm nào và không
biết `Domain` là gì; `Domain` là tên do prelude khai báo.

Đi được thì có một đối tượng mới, `(a, F(a)) ∈ F` được ghi kèm lý do, và đối tượng đó là giá
trị canonical của `F` tại `a`. `F(a)` là tên canonical của chính nó, không có một `F(a)` nào
khác đứng bên cạnh, và viết `F(a)` lần sau trả về đúng nó.

Thiếu tiền đề thì bước không đi được: không đối tượng nào được tạo, không fact nào được
ghi, và lỗi chỉ đúng chỗ thiếu. Nhắc `F(a)` ở bất cứ đâu trước khi bước áp xảy ra cũng
không đi được.

Giới hạn hiện có: `F(x)` với `x` là biến của một luật **chưa** viết được, vì `F(a)` được quy
về đối tượng ngay lúc parse, mà biến thì chưa có đối tượng. Trong luật phải nói qua quan hệ:
`(x, y) ∈ F` thay cho `F(x) = y`.

```
Let F be a map.
Let a be an entity.
Apply Domain to F.
Apply F to a.            -- không đi được: chưa xác lập `a in Domain(F)`
```

Chỗ này không phá "ghi luôn thành công": thao tác ghi không xảy ra, chứ không phải xảy ra
rồi bị từ chối. Nó cũng không mâu thuẫn với việc ma trận nhận mọi đối tượng làm cột, vì hai
chỗ làm hai việc khác nhau: ghi vào một ô là ghi lại một điều đã được nói, nên không có gì
phải tra trước; còn áp hàm thì tạo ra một đối tượng mới và khẳng định nó là *giá trị* của
hàm tại đó, và một bước nghĩ thì cần tiền đề.

`Domain` cũng là hàm, nên `Domain(F)` đi đúng quy trình ấy và đòi `F ∈ Domain(Domain)`.
Chuỗi này dừng ở `Domain(Domain) = MAP`, đặt ra từ đầu, cùng loại với `SET ∈ SET`.

Nếu `(a, y) ∈ F` đã có sẵn với một `y` nào đó thì bước áp hàm vẫn tạo đối tượng mới. Cơ chế
không tự ghi `(F(a), y) ∈ Eq` và cũng không chặn gì. Tính chất của MAP chỉ có nghĩa thế
này: nếu có người ghi `(F(a), y) ∉ Eq` rồi đi một chuỗi suy luận hợp lệ dùng luật duy nhất
của MAP để tới `(F(a), y) ∈ Eq`, thì lúc đó mâu thuẫn mới lộ ra. Không ai đi chuỗi đó thì
hai cờ cứ nằm yên.

Hệ quả đáng giữ: `ChildOf` không thuộc MAP, nên `Domain(ChildOf)` không hình thành được,
nên `ChildOf(lan)` không hình thành được. Cách viết áp hàm tự nó không dựng ra được mâu
thuẫn giả. Muốn nói về *một* đứa con của Lan thì dùng nhân chứng mới.

### Hai dạng dãy, và dạng canonical

Cả hai dạng cùng tồn tại trong thế giới. `(a, b, c)` là một đối tượng term, danh tính theo
danh sách thành phần. Hàm `{(1, a), (2, b), (3, c)}` thuộc `Tuple(3)` là một đối tượng
khác, dựng bằng nội dung. Cơ chế không gộp chúng, và không tự nối chúng.

Dạng **canonical là dạng hàm**. Lý do hẹp và quyết định: dạng liệt kê chỉ tồn tại khi arity
là một số cụ thể, nên một tuple độ dài ký hiệu không có dạng liệt kê nào cả — lấy liệt kê
làm canonical thì có những tuple không có canonical form, tức là nó không phải canonical.
Chiều ngược lại thì kín: mọi tuple liệt kê đều có dạng hàm tương ứng.

Cặp `(x, y)` là ngoại lệ, và ngoại lệ này bắt buộc. Hàm là một set chứa cặp, nên dạng hàm
của một cặp lại chứa cặp. Cặp dừng ở chính nó; nó là sàn.

| Viết | Dạng canonical |
|---|---|
| `(x, y)` | chính nó — sàn, không khai triển |
| `(a, b, c)` và dài hơn | hàm `{(1, a), (2, b), (3, c)}` thuộc `Tuple(3)` |
| `(x(1), …, x(N))` | đã là dạng hàm |

Bước khai triển là một bước phải viết ra, không tự chạy:

```
Expand (a, b, c) as t3.
```

Nó tạo đối tượng `t3`, ghi `(1, a)`, `(2, b)`, `(3, c)` vào `t3`, và ghi
`((a, b, c), t3) ∈ Expanded`. Viết `(a, b, c) ∈ R` thì ô đó dùng đúng đối tượng liệt kê;
chỉ khi có bước khai triển thì mới có fact nối nó với dạng hàm, và nối tiếp sang `Eq` là một
luật người dùng tự nạp. Đây đúng là
khuôn của `2 + 3` với `5`: hai term khác nhau, một bước nhận ra chúng chỉ một thứ.

Canonical ở đây có nghĩa là *dạng để quy về khi hai dạng gặp nhau*, không có nghĩa là dạng
runtime lưu mặc định. Quan hệ nhiều ngôi viết theo dạng liệt kê vẫn rẻ và vẫn khớp mẫu
thẳng theo danh sách thành phần; dạng liệt kê không bị bỏ, nó chỉ thôi làm chuẩn quy về.

Quyết định này không đụng vào trần diễn đạt, vì trần nằm ở hình dạng mệnh đề mà kho luật
chứa được, còn đây chỉ là chuyện hai cách viết dãy quy về nhau ra sao. Và dạng hàm còn nới
thêm một chỗ mà dạng liệt kê không với tới: lượng từ trên arity. `for every t, N, if t ∈
Tuple(N) then …` viết được, trong khi không có mẫu liệt kê nào khớp mọi arity.

## Kho mệnh đề

Kho mệnh đề giữ mọi mệnh đề mà ma trận không giữ được, tức là mọi mệnh đề không nguyên tử.

Ghi vào kho là thao tác **cất**: nó nhận một mệnh đề cộng một lý do, và kho có thêm mệnh
đề đó. Cất luôn thành công, đúng như bật cờ luôn thành công. Thao tác ngược lại là **tra**:
kho có đúng mệnh đề này không. Tra chứ không tìm — runtime trả lời câu hỏi được đặt ra,
nó không đi lùng.

Bốn chỗ cất vào kho:

- `Rule (r): for every x, if x ∈ A then x ∈ B.` — đặt ra một luật chính là cất một mệnh đề.
- `Assume (h): A or B.` — một giả định có hình dạng ghép cũng cất vào đúng chỗ đó.
- Một bước áp luật mà kết luận là mệnh đề ghép.
- Một bước thoát scope, dựng ra mệnh đề ghép mới (xem dưới).

Kho luật và kho mệnh đề là một chỗ, không phải hai. Một luật là một mệnh đề được đặt ra.
Mệnh đề `for every x, if x ∈ A then x ∈ B` viết ra bằng `Rule`, hay dựng ra khi thoát một
scope, thì cũng là cùng một mệnh đề nằm cùng một chỗ, và áp được y như nhau. Khác biệt
duy nhất giữa chúng nằm ở lý do đi kèm.

Kho giữ cái **đã được nói**, không giữ cái đã được chứng minh. Đây là chỗ song song với
ma trận, nơi cờ ghi "đã được nói ∈" chứ không ghi "là phần tử". Vì thế một mệnh đề nằm
trong kho là một mệnh đề áp được ngay, không cần thêm điều kiện nào: điều kiện kích hoạt
của một luật là nó có trong kho và tiền đề của nó tra thấy, chứ không phải là nó đúng.

## Lý do

Mỗi lần một cờ bật hay một mệnh đề được cất, runtime ghi kèm một lý do. Có đúng hai loại.

**Đặt ra** — nó vào thế giới vì một câu trong chương trình nói thẳng ra. Lý do giữ dòng
nào trong file nguồn.

**Suy ra** — nó vào thế giới vì một bước áp luật. Lý do giữ ba thứ: luật nào, biến của
luật được thế bằng gì, và những tiền đề nào đã được tra.

Tiền đề của một bước không nhất thiết là một cờ. Nó có thể là một cờ ở một ô, hoặc một
mệnh đề trong kho. Cả hai đều mang lý do, cả hai đều tra được, nên cây lý do bắc qua cả
hai chỗ lưu chứ không sống riêng trong ma trận.

Vì tiền đề cũng có lý do của nó, đi ngược được: từ một kết luận lần về các tiền đề, rồi
về tiền đề của tiền đề, cho tới khi chạm những chỗ *đặt ra*. Đi hết thì được một cái cây,
và lá của cây là toàn bộ những chỗ đặt ra mà kết luận này dựa vào.

Giữ hết mọi lý do. Một kết luận đi tới được theo mấy đường thì mang mấy lý do; không lý
do nào thay thế lý do nào.

## Giả định và scope

Một giả định là một thứ đã được nói, nên nó vào thẳng chỗ lưu tương ứng với hình dạng của
nó. `Assume (h): x ∈ S.` bật cờ trong ma trận. `Assume (q): if S then R.` cất vào kho.
Không có trạng thái trung gian nào, và không có chỗ nào khác cho chúng.

Từ lúc đó, giả định dùng được y như một luật. `By rule (q)` chạy được: tra `(q)` trong
kho, tra tiền đề `S`, ghi `R`. Không cần bọc nó trong một luật meta kiểu `(S → R) → …`,
vì việc cất vào kho chính là việc coi nó là đúng tại thời điểm đó.

Giả định sống trong một **scope**. Vào scope thì runtime đóng một mốc trong nhật ký. Ra
scope thì quay về mốc đó: giả định bị xóa khỏi kho, và mọi thứ đã ghi trong scope cũng
bị gỡ theo. Thế giới trở lại đúng trạng thái lúc vào.

Thứ duy nhất một scope để lại là mệnh đề nó dựng ra lúc thoát. Thoát scope làm hai việc
theo thứ tự: dựng mệnh đề ghép mới từ giả định và kết luận, rồi mới quay về mốc. Mệnh đề
đó được cất sau khi đã quay về, nên nó nằm ngoài vùng bị gỡ.

Hình dạng mệnh đề dựng ra tùy theo scope mở bằng cái gì:

- Mở bằng một giả định `T`, trong scope suy ra `X` — thoát ra được `if T then X`.
- Mở bằng một đối tượng mới chưa ai nói gì về nó, trong scope suy ra `A(x)` — thoát ra
  được `for every x, A(x)`.
- Mở bằng một đối tượng mới cộng đúng một stipulation `x ∈ S`, trong scope suy ra `A(x)`
  — thoát ra được `for every x in S, A(x)`.

Scope lồng nhau thì thoát từ trong ra ngoài, mỗi lần thoát dựng một mệnh đề và quay về
mốc của chính nó. Giả định `R` ngoài, giả định `U` trong, suy ra `S`: thoát scope trong
cho `if U then S`, thoát scope ngoài cho `if R then (if U then S)`.

Một chi tiết về việc quay lại: nếu trong scope có một cờ vốn đã bật từ trước scope được
bật lại, thao tác đó chỉ thêm một lý do mới cho cờ ấy. Quay về mốc gỡ lý do vừa thêm
nhưng không tắt cờ, vì cờ đó đã có chỗ dựa riêng từ trước khi vào scope.

## Luật

Một luật **là** một mệnh đề được đặt ra. `Rule` và `Assume` đặt ra cùng một loại mệnh đề và
làm cùng một việc lên runtime; khác nhau chỉ ở ý định người viết. Mệnh đề chỉ có một cú
pháp: không phân biệt "vế đầu" với "vế sau", vì `if A then B` chỉ là một cách ghép và `A`,
`B` là mệnh đề bất kỳ. Ghép lồng tùy ý, không có giới hạn hình dạng nào.

### Nguyên tử

```
t ∈ S
t ∉ S
```

Cả hai vị trí nhận term bất kỳ — tên, biến, số, tuple, áp hàm. `t ∈ Tuple(N)`,
`k ∈ Index(N)`, `x ∈ (a, b)` đều hợp lệ. Unicode và ASCII thay nhau được: `∈`/`in`,
`∉`/`notin`.

### Ghép

```
A and B
A or B
if A then B
A iff B
not (A)
for every x, A
for every x, y, z, A
there exists x such that A
```

Mọi `A`, `B` lại là bất kỳ dòng nào ở trên, và ngoặc `( … )` dùng để nhóm.

Bên cạnh đó có dạng gọn buộc biến vào một set, và dạng đầy đủ tương ứng. Giữ cả hai, vì
chúng nói hai điều khác nhau:

```
for every x in S, A                  -- S là một tên, hoặc biến đã buộc ở ngoài
for every x, S, if x ∈ S then A      -- S là biến buộc ngay tại đây

for every x, y in S, A               -- và: for every x, y, S, if x ∈ S and y ∈ S then A
for every x in S such that B, A      -- và: for every x, S, if x ∈ S and B then A
there exists x in S such that A      -- và: there exists x, S such that x ∈ S and A
```

Dạng gọn `for every x in S, A` là cách viết tắt của `for every x, if x ∈ S then A`, nói về
một set cụ thể. Dạng đầy đủ nói về mọi set.

### Từng dạng, viết ra

```
Rule (join):       for every x, y, z, if (x, y) ∈ Boss and (y, z) ∈ Boss then (x, z) ∈ Boss.
Rule (set var):    for every x, S, if x ∈ S then S ∈ Inhabited.
Rule (two roles):  for every x, S, P, if S ∈ P and x ∈ S then (x, P) ∈ DeepMember.

Rule (meet):       for every x, if x ∈ A and x ∈ B and x ∉ C then x ∈ D.
Rule (staff):      for every x, if x ∈ Staff then x ∈ Payroll and x ∈ BuildingAccess.
Rule (tax):        for every x, if x ∈ Citizens or x ∈ PR then x ∈ TaxLiable.
Rule (split):      for every x in People, if x ∈ Residents then x ∈ Citizens or x ∈ PR.

Rule (subset in):  for every S, T, if (for every x, if x ∈ S then x ∈ T) then (S, T) ∈ Subset.
Rule (subset out): for every S, T, if (S, T) ∈ Subset then (for every x, if x ∈ S then x ∈ T).
Rule (adult def):  for every x, x ∈ Adults iff x ∈ People and x ∈ Over18.

Rule (no vote):    for every x, if x ∈ Minors then x ∉ Voters.
Rule (violation):  for every x, if x ∉ Safety then x ∈ Violators.
Rule (disjoint):   for every x, not (x ∈ Odd and x ∈ Even).
Rule (separate):   for every x, if not (x ∈ A and x ∈ B) then x ∈ Separate.

Rule (empty):      for every U, if U ∈ Empty then (for every x, x ∉ U).
Rule (ext):        for every S, T, if (for every x, x ∈ S iff x ∈ T) then (S, T) ∈ Eq.
Rule (inverse):    for every x in G, there exists y such that y ∈ G and (x, y, e) ∈ Op.
Rule (parent):     for every x, if there exists y such that (x, y) ∈ ParentOf then x ∈ Parents.

Rule (sat both l): for every x, p, q, if (x, (both, p, q)) ∈ Sat then (x, p) ∈ Sat.
Rule (rank):       for every x, y, if (x, y) ∈ Boss then (x, (above, y)) ∈ Rank.
```

### Nghĩa của phủ định và hoặc

`x ∉ S` là ô `(x, S)` bật cờ ∉. Ô chưa ai nói gì **không** phải `x ∉ S`; nó là chưa biết.

`not (A)` với `A` ghép thì không tự đẩy vào trong: `not (x ∈ A and x ∈ B)` không tự thành
`x ∉ A or x ∉ B`. Chiều đó là logic cổ điển, tức là thư viện.

Có `A` và có `not (A)` thì chỉ ra được vô lý, nhưng từ vô lý **không** tự suy ra mọi thứ.
Muốn nổ thì nạp luật nổ. Phủ định kép và bài trung cũng vậy.

`A or B` không bật cờ ở ô nào; cả hai ô có thể vẫn chưa ai nói gì. Có `A or B` và có
`not (A)` thì suy ra `B` **chỉ khi** đã nạp luật tương ứng. Không nạp thì không đi được —
hành vi paraconsistent, và nó do thư viện quyết chứ không do runtime.

### Ba chỗ không ngầm định

`not (t ∈ S)` và `t ∉ S` là hai mệnh đề khác nhau, không phải một cách viết của cùng một
thứ. Có `A` và có `B` không tự có `A and B`; muốn có thì phải đi qua một bước. Biến tự do
không tự hiểu thành "với mọi"; phải viết `for every` ra.

Cả ba đều theo cùng một lựa chọn: không có gì tự xảy ra mà không ai viết ra.

## Bước chứng minh

Có đúng hai hình dạng bước. Cái nào **buộc biến hoặc rút giả định** thì cần scope; còn lại
chỉ là tra rồi cất.

### Đưa vào

| | làm gì | scope |
|---|---|---|
| `and` | tra `A`, tra `B` → cất `A and B` | không |
| `or` | tra `A` → cất `A or B`, vế kia chọn tự do | không |
| `iff` | tra hai chiều → cất `A iff B` | không |
| `there exists` | tra `A(t)` với `t` cụ thể → cất `there exists x such that A(x)` | không |
| `if … then` | giả định `A`, suy ra `B`, thoát | có |
| `for every` | đối tượng mới, suy ra `A(x)`, thoát | có |
| `not` | giả định `A`, chỉ ra vô lý, thoát | có |

```
From (h1), (h2), it follows that A and B.
From (h1), it follows that A or B.
From (h1), (h2), it follows that A iff B.
From (h1), it follows that there exists x such that A(x).
```

```
Suppose (h): A {
    ...
}
Hence (r): if A then B.

Take x {
    ...
}
Hence (r): for every x, A(x).

Take x with x ∈ S {
    ...
}
Hence (r): for every x in S, A(x).

Suppose (h): A {
    ...
    Absurd from (p), (q).
}
Hence (r): not (A).
```

Thân scope nằm trong `{ }`. Mệnh đề dựng ra lúc thoát vẫn viết rõ bằng `Hence` ngay sau
ngoặc đóng, và vẫn có nhãn — ngoặc lo phần thân, `Hence` lo phần kết quả.

Mệnh đề nguyên tử không có bước đưa vào theo nghĩa suy luận: nó vào thế giới bằng `Assume`,
hoặc bằng kết luận của một lần áp luật.

### Dùng

**`and`** — tra `A and B`, cất `A`. Nếu `A` nguyên tử thì cất nghĩa là bật cờ ở ô; nếu ghép
thì vào kho.

**`if … then` và `for every`** — là áp luật, thao tác vốn đã có.

**`not`** — `Absurd from (p), (q)` với `q` là phủ định của `p`, hoặc trỏ vào một ô đã có cả
hai cờ. Bước này chỉ đánh dấu scope hiện tại là vô lý, để lúc thoát dựng được `not (…)`. Nó
**không nổ**: chỉ ra vô lý không cho phép suy ra thứ khác, muốn thế thì nạp luật nổ.

**`iff`** — tra `A iff B`, cất một chiều: `if A then B`, hoặc `if B then A`.

```
From (h), it follows that if A then B.
```

**`there exists`** — cần scope, và đối xứng với `for every`-đưa-vào.

```
Take w from (h): there exists x such that A(x) {
    ...
}
Hence (r): C.
```

Vào scope thì tạo một đối tượng mới `w` và ghi `A(w)`, lý do trỏ vào `(h)`. Thoát thì cất
`C` rồi quay về mốc, nên `w` biến mất cùng mọi thứ ghi trong scope.

Ràng buộc: **`C` không được nhắc tới `w`**. Nó không phải một luật thêm vào — sau khi quay
về mốc thì `w` không còn, nên một mệnh đề trỏ vào nó là mệnh đề trỏ vào chỗ trống. Nhật ký
biết scope đã tạo những đối tượng nào nên cơ chế kiểm được ngay. Đây đúng là điều kiện
eigenvariable của ∃-elim, rơi ra từ việc quay về mốc chứ không cần ai phát biểu.

Chỗ khác với `for every`-đưa-vào: ở đó kết luận **buộc** biến, nên lúc thoát đối tượng được
thay bằng biến buộc và mệnh đề sống sót hợp lệ. Ở đây không buộc gì, nên `C` phải sạch từ
đầu.

Và nhân chứng **luôn mới**: hai lần `Take` từ cùng một mệnh đề tồn tại cho hai đối tượng
khác nhau. Lan có hai con thì "Lan có con" dùng hai lần không được coi là cùng một đứa.
Muốn dùng lại một nhân chứng thì phải chứng minh tính duy nhất rồi đi đường áp hàm, nơi
`F(a)` là tên canonical.

**`or`** — đây là chỗ khác hẳn các hệ quen thuộc. `or`-dùng **không** phải chia trường hợp.
Nó là bộ kích hoạt mâu thuẫn: tra `A or B`, tra `not A`, tra `not B` — ba mệnh đề riêng
biệt, không phải `not A and not B` — rồi chỉ ra vô lý.

```
Absurd from (or), (na), (nb).
```

Không có chia nhánh, không có thế giới song song, không có nhánh nào bị vứt. Một bước tra ba
thứ, đúng hình dạng mọi bước khác.

Muốn lấy `A` ra từ `A or B` và `not B` thì đi qua phản chứng:

```
Suppose (h): not A {
    Absurd from (or), (h), (nb).
}
Hence (na): not (not A).
```

rồi `not (not A)` → `A` bằng luật phủ định kép của thư viện. Nên cơ chế không có chia trường
hợp, và mọi đường dùng `or` đều xuyên qua logic cổ điển. Đó là một lựa chọn đã biết, không
phải một chỗ sót.

### Bảng dùng

| | làm gì | scope |
|---|---|---|
| `and` | tra `A and B` → cất `A` (hoặc `B`) | không |
| `iff` | tra `A iff B` → cất một chiều | không |
| `if … then` | áp luật | không |
| `for every` | áp luật với binding | không |
| `not` | `Absurd from (p), (q)` | không |
| `or` | `Absurd from (or), (na), (nb)` | không |
| `there exists` | `Take w from (h)`, làm, thoát; `C` không nhắc `w` | có |

## Mệnh đề là đối tượng

Mỗi mệnh đề trong kho ghép được với một đối tượng trong kho đối tượng. Cơ chế cung cấp đúng
cái ghép đó, và không gì khác.

Ghép **lười**, đúng nguyên tắc chung: đối tượng đại diện chỉ sinh ra khi bài chứng minh gọi
tên nó. Không ai nhắc tới một mệnh đề như một đối tượng thì nó không có đại diện.

Cơ chế giữ hai chiều:

**Cấu trúc.** Đại diện là term có cấu trúc, dựng từ chính các đại diện con:

| mệnh đề | đại diện |
|---|---|
| `t ∈ S` | `(Mem, t, S)` |
| `t ∉ S` | `(NotMem, t, S)` |
| `A and B` | `(And, a, b)` — tương tự `Or`, `Implies`, `Iff` |
| `not (A)` | `(Not, a)` |
| `for every x, A` | `(All, a)` |
| `there exists x such that A` | `(Ex, a)` |

Biến buộc bên trong thân thành `(Var, k)` với `k` là chỉ số de Bruijn, không phải tên. Bắt
buộc phải thế: hai mệnh đề chỉ khác tên biến buộc là cùng một mệnh đề, nên đại diện của
chúng phải trùng, và chỉ số làm chuyện đó tự đúng. Nhìn vào là đọc được hình dạng, vì tuple
đọc được.

Trong nguồn, `[A]` là đại diện của mệnh đề `A`, và `[(h)]` là đại diện của mệnh đề mang nhãn
`h`. Viết nó ra chính là hành động làm nó sinh ra.

**`Holds`.** `p ∈ Holds` khi và chỉ khi mệnh đề mà `p` đại diện đang có trong kho. Đây là
một ô như mọi ô, nên nó theo scope: vào scope giả định `A` thì đại diện của `A` vào `Holds`,
ra scope thì rụng theo.

Phần còn lại là thư viện, viết bằng V như mọi thứ khác:

```
Rule (holds and):     for every p, q, (And, p, q) ∈ Holds iff p ∈ Holds and q ∈ Holds.
Rule (holds implies): for every p, q, if (Implies, p, q) ∈ Holds and p ∈ Holds then q ∈ Holds.
Rule (dne):           for every p, if (Not, (Not, p)) ∈ Holds then p ∈ Holds.
```

Phép thế trên đại diện cũng có, và nó không viết lại: `Instantiate (h) at t.` giải mã đại
diện về mệnh đề thật, dùng đúng hàm `instantiate` mà áp luật đang dùng, rồi mã hoá lại, và
ghi `(đại diện của h, t, đại diện của thể hiện) ∈ Instance`. Cùng một hàm, nên hai phép thế
không thể lệch nhau. Nhờ đó ∀-elim viết được thành một luật thường:

```
Rule (all elim): for every p, t, q, if p ∈ Holds and (p, t, q) ∈ Instance then q ∈ Holds.
```

### Vì sao chuyện này đáng giá

Đại diện là đối tượng, nên lượng từ trên mệnh đề là **lượng từ bậc nhất**. Không phải sửa cơ
chế áp luật, không phải ghép cây mệnh đề vào chỗ biến, không tốn tầng nào.

Từ đó, một lối lập luận lặp đi lặp lại chứng minh **một lần** rồi áp một dòng mãi mãi:

```
Rule (chain): for every p, q, r, s,
    if p ∈ Holds
       and (Implies, p, (Or, q, r)) ∈ Holds
       and (Implies, q, s) ∈ Holds
       and (Implies, r, s) ∈ Holds
    then s ∈ Holds.

Rule (cases): for every p, q, c,
    if (Or, p, q) ∈ Holds and (Implies, p, c) ∈ Holds and (Implies, q, c) ∈ Holds
    then c ∈ Holds.
```

`(cases)` chứng minh một lần bằng khối phản chứng dài ở trên, rồi mọi lần chia trường hợp về
sau là một dòng. Giá thực tế của việc cơ chế không có chia nhánh do đó bằng không.

Và xa hơn: **định lý về lời giải** viết được. Luật nói về luật, phép biến đổi chứng minh,
"chứng minh được X theo cách này thì chứng minh được Y theo cách kia" — tất cả là luật
thường, áp bằng đúng một thao tác, có lý do, đọc ngược được. Cái mà các hệ khác đặt ở một
siêu ngôn ngữ riêng bên ngoài logic, ở đây nằm trong cùng thế giới.

### Cái phải trả

Đây là một diện tích tin cậy mới, cùng loại với việc tin số học của máy nhưng lớn hơn nhiều:
cơ chế phải giữ `Holds` đồng bộ với kho, và nếu nó lệch thì toàn bộ tầng phản chiếu nói dối.
Không chứng minh được từ bên trong, chỉ tin vào cài đặt.

Và đây là chỗ tự quy chiếu vào ở. Mệnh đề nói về mệnh đề, kể cả về chính nó, viết được — câu
nói dối cũng viết được. Theo thiết kế đã chốt thì nó không nổ: ô có hai cờ, mỗi cờ mang lý
do, và hết.

## Số học và tính toán

Một sự thật như `100 + 101 = 201` vào được thế giới theo ba đường, và người viết chọn
đường. Hệ thống không chọn hộ, và không cấm đường nào.

**Cửa 1 — suy ra bằng luật.** Tiên đề Peano cộng định nghĩa đệ quy của phép cộng, rồi áp
luật từng bước. Dài khủng khiếp, và hoàn toàn hợp lệ. Ai muốn chứng minh `100 + 101 = 201`
theo kiểu đó thì cứ làm, hệ thống không cản. Đây là mức duy nhất không phải tin gì ngoài
các luật đã nạp.

**Cửa 2 — đại số ký hiệu.** Biến đổi biểu thức mà không ra số: rút gọn, triển khai, cộng
phân số, cộng phân thức, quy đồng, gom số hạng. `π` và `e` sống ở đây như ký hiệu có tiên
đề. Chính xác tuyệt đối, và phủ được những thứ mà số không phủ được.

**Cửa 3 — số học của máy.** Cơ chế đưa biểu thức xuống bộ xử lý số học và nhận kết quả về.
Numeral là hữu tỉ chính xác, tử mẫu không giới hạn độ lớn, luôn tối giản.

`2 + 3` là một term riêng — tuple `(Plus, 2, 3)` — khác hẳn đối tượng `5`. Cửa 2 và cửa 3 là
hai cách khác nhau để có một bước nối hai term đó; parser không tự nối.

### Cửa 2: dạng chuẩn và điều kiện

`Simplify` đưa biểu thức về một dạng chuẩn duy nhất: **phân thức hữu tỉ nhiều biến** — tử và
mẫu là đa thức, hệ số hữu tỉ chính xác, và "biến" là một đối tượng bất kỳ (một tên, `pi`,
một giá trị áp hàm). Một dạng chuẩn đó làm được cả rút gọn, triển khai, cộng phân số lẫn
cộng phân thức. Toán tử có: `+ - * /` và `^` với số mũ nguyên không âm.

Kết quả ghi vào một trong hai quan hệ:

- `(u, v) ∈ Simplified` — đẳng thức không kèm điều kiện.
- `(u, v, k) ∈ SimplifiedIf` — đẳng thức chỉ đúng dưới điều kiện `k`, với `k` là **đại diện
  của mệnh đề điều kiện**.

Điều kiện sinh ra khi triệt một nhân tử có chứa biến. `x/x` thành `1` chỉ khi `x ≠ 0`;
`(x^2 - 1)/(x - 1)` thành `1 + x` chỉ khi `x - 1 ≠ 0` (một biến thì chạy thuật toán Euclid
trên đa thức hữu tỉ; nhiều biến thì mới triệt ước đơn thức chung). Mỗi nhân tử bị triệt thành
một mệnh đề `(f, 0) ∉ Eq`, và các mệnh đề đó ghép bằng `and` thành `k`. Triệt **hệ số** thì
không cần điều kiện, vì một số hữu tỉ khác 0 thì khác 0 ở mọi chỗ.

Cơ chế không được phép giấu một điều kiện vào trong một đẳng thức, nên điều kiện đi ra ngoài
và luật dùng `SimplifiedIf` phải đòi nó đã được xác lập:

```
Rule (eq simp if): for every u, v, k,
    if (u, v, k) ∈ SimplifiedIf and k ∈ Holds then (u, v) ∈ Eq.
```

`Simplify e as (nz).` đặt nhãn cho điều kiện, để `Assume (nz).` đặt ra đúng mệnh đề đó và
`[(nz)]` trỏ tới đại diện của nó, khỏi phải chép lại ở dạng chuẩn của tầng đại số.

### Máy luôn trả về kèm sai số

Cửa 3 không bao giờ chỉ trả về một con số. Nó trả về một số hữu tỉ **và** một chặn sai số:

```
1/3 + 1/6          →  1/2,        sai số 0
100 + 101          →  201,        sai số 0
sqrt(2), 10 chữ số →  p/q,        sai số 10^-10
pi, 10 chữ số      →  p/q,        sai số 10^-10
2/3, 2 chữ số      →  67/100,     sai số 1/100
```

Nên chỉ có một quan hệ cho mọi kết quả máy sinh ra, và nó mang ba thành phần:

```
(biểu thức, kết quả, sai số) ∈ Computed
```

Chính xác không phải một loại riêng — nó là trường hợp sai số bằng `0`. Và đẳng thức rơi ra
đúng từ chỗ đó, bằng một luật thư viện người dùng nạp:

```
Rule (Eq from exact computation): for every a, b, if (a, b, 0) ∈ Computed then (a, b) ∈ Eq.
```

Nạp dòng đó là tuyên bố tin số học của máy. Không nạp thì `Computed` nằm trơ và mọi thứ
phải đi cửa 1. Kết quả có sai số khác `0` thì không bao giờ thành đẳng thức, và cũng không
cần: nó cần các luật về chặn.

Nghĩa vụ đặt lên cơ chế gói gọn trong một câu: **mọi lần trả kết quả đều phải khai sai số
trung thực.** Đó là toàn bộ diện tích tin cậy của cửa số học.

### Không có dấu phẩy động

Ở đâu cũng không. `0.1` là `1/10`. Biểu diễn thập phân là một số hữu tỉ có mẫu là lũy thừa
của 10, nên hàm đổi sang thập phân chỉ là một phép tính nữa đi qua đúng cửa 3, với đúng hợp
đồng đó: `1/2` ra `1/2` sai số `0`, còn `1/3` tới bốn chữ số ra `3333/10000` sai số `10^-4`.

Hệ quả: cơ chế không bao giờ viết ra `2/3 = 0.67`. Câu đó sai. Cái nó viết ra là
`(2/3, 67/100, 1/100) ∈ Computed` — máy trả về `67/100` và khai sai số `1/100`. Sai số là
một thành phần của chính fact, nên nó không rụng mất trên đường đi, khác hẳn dấu phẩy động
nơi sai số biến mất ngay khi được cất vào chỗ dành cho đẳng thức.

### Xấp xỉ là lý thuyết, không phải cơ chế

Cơ chế chỉ khai "tôi trả về gì, sai số tôi khai là bao nhiêu". Chữ *xấp xỉ* không xuất hiện
ở đó. Việc một con số có đáng gọi là xấp xỉ của con số khác hay không là chuyện của lý
thuyết, và lý thuyết là của người dùng:

```
Rule (approx def): for every a, b, e, (a, b, e) ∈ Approx iff |a - b| <= e.
Rule (approx sum): for every a, b, c, d, e, f,
    if (a, b, e) ∈ Approx and (c, d, f) ∈ Approx then (a + c, b + d, e + f) ∈ Approx.
```

Sai số cộng dồn là một luật, không phải một cơ chế. Viết định nghĩa khác — sai số tương
đối, khoảng, theo bậc độ lớn — thì ra nghĩa khác. Viết một định nghĩa khiến `1` xấp xỉ `100`
cũng được, và đó là một lý thuyết hợp lệ. Cơ chế không có ý kiến.

### So sánh

`a <= b` là cách viết của `(a, b) ∈ LessEq`; `a < b` của `(a, b) ∈ Less`; `a > b` và
`a >= b` viết ngược lại thành `Less`/`LessEq`. So sánh không phải một loại mệnh đề riêng, nó
vẫn là một ô. `Compute a <= b.` cho máy quyết định và ghi **cả hai chiều**: đúng thì bật cờ
∈, sai thì bật cờ ∉, lý do là *máy tính ra*. Ô máy chưa được hỏi thì vẫn là chưa ai nói gì.

### Các tầng số

**ℚ** có sẵn ở dạng giá trị: numeral của cơ chế chính là hữu tỉ.

**ℤ, ℕ** là tập con do lý thuyết định nghĩa.

**ℝ** không có sẵn và là ký hiệu. `√2` là một đối tượng với tiên đề `x · x = 2` và `x > 0`,
không phải một dãy chữ số. Lý do không phải kỹ thuật: số thực không có biểu diễn hữu hạn, và
đẳng thức trên nó không quyết định được. Lean cũng đúng như vậy — ℝ dựng từ dãy Cauchy hữu
tỉ, `noncomputable`, và dùng thuần qua giao diện trường sắp thứ tự đầy đủ.

**Chia cho 0** không lỗi và không quy ước giá trị. `Compute 1 / 0.` ghi `(Div, 1, 0) ∈ NoValue`,
và lý thuyết tự quyết xử lý thế nào.

## Expressibility

Sức diễn đạt của hệ thống nằm ở kho luật: kho chứa được những hình dạng mệnh đề nào thì
hệ thống nói được đến đó. Vì lý thuyết dựng ra từ core, mọi nội dung — Eq, số học, logic
cổ điển, quy nạp — đều là luật người dùng viết, nên trần của toàn bộ nội dung nằm đúng ở
chỗ đó. Hình dạng term dựng được cũng tính vào đây, vì term nằm bên trong mệnh đề: không
viết được term thì không luật nào nói tới thứ đó được.

Bộ thao tác không đặt thêm trần nào lên trên. Chứng minh một mệnh đề ghép là mở một scope
và thoát ra: chứng minh `for every x, A` là tạo một đối tượng mới, suy ra `A` về nó, rồi
thoát; chứng minh `if P then Q` là giả định `P`, suy ra `Q`, rồi thoát. Cả hai chỉ dùng
những thao tác runtime vốn đã có — tạo đối tượng, ghi cờ, cất mệnh đề, đóng mốc, quay về
mốc.

### So với Lean

Mốc là **tiềm lực ngang**, không phải viết giống. Lean mạnh ở chỗ lượng từ đi trên mọi thứ,
kể cả kiểu và mệnh đề. V đi hai nước khác, và hai nước đó phủ đúng chỗ ấy.

**Set là đối tượng.** Cả hai vị trí của `∈` nhận term bất kỳ, nên `for every S, …` là lượng
từ bậc nhất mà vẫn với tới mọi tập. Hệ quả: axiom schema không cần. Separation và quy nạp của
ZFC là một họ vô hạn tiên đề, một cái cho mỗi công thức, vì ở đó lượng từ không đi trên tính
chất; ở đây tính chất là set, nên mỗi cái là **một** luật, và phủ rộng hơn cả họ:

```
Rule (sep): for every A, S,
    there exists B such that (for every x, x ∈ B iff x ∈ A and x ∈ S).

Rule (induction): for every P,
    if 0 ∈ P and (for every n, if n ∈ N and n ∈ P then n + 1 ∈ P)
    then (for every n, if n ∈ N then n ∈ P).
```

Và vì V không phải một danh sách tiên đề cố định, cần một tập con cụ thể thì khai nó ra rồi
nói điều kiện — comprehension viết thẳng, kể cả tập Russell.

**Mệnh đề là đối tượng.** Lượng từ trên mệnh đề đi qua tầng phản chiếu của cơ chế (xem mục
*Mệnh đề là đối tượng*): đại diện là đối tượng, nên lượng từ trên đại diện là bậc nhất. Một
lối lập luận chứng minh một lần rồi áp một dòng mãi mãi — `(chain)`, `(cases)`, `(dne)`, và
cả ∀-elim — đều là luật thường. Cái Lean đặt ở siêu ngôn ngữ, thao tác trên `Expr` và không
phải định lý của logic, ở đây nằm trong cùng thế giới, có lý do, đọc ngược được.

**`Prop`/`Sat` viết tay còn đúng một chỗ dùng:** nhúng **một logic khác** — trực giác, modal,
lambda calculus — khi muốn một bản sao do mình kiểm soát chứ không mượn mệnh đề của V. Bản
v0.3 đã chạy thật một lý thuyết natural deduction nhúng như vậy. Mã hóa tay thì trung thực
đúng bằng các luật cầu người viết đặt ra, và dòng chịu lực là luật nối công thức mã hóa với
membership thật, kiểu `(x, (mem, S)) ∈ Sat iff x ∈ S`. Hình dạng tuple đặt vào `Sat` là do
luật quyết, không có arity nào cố định, nên lượng từ trong mã hóa cũng chỉ là luật thêm.

**Chỗ chênh thật nằm ở giá, không ở cái nói được.** V không có tính toán theo định nghĩa:
Lean coi `2 + 2` và `4` là một, còn ở đây mỗi lần nhận ra hai tên chỉ một thứ là một bước
phải viết. Không phải thiếu sót — định nghĩa việc nghĩ thì không được giấu bước nghĩ — nhưng
đó là khoảng cách lớn nhất về độ dài chứng minh.

**Chỗ V rộng hơn Lean:** không có phân tầng universe. `SET ∈ SET`, `Domain(Domain) = MAP`,
mệnh đề nói về chính nó, luật kiểu Russell — viết được hết. Lean chặn bằng kiểu. Ở đây, định
nghĩa chân lý cho toàn bộ ngôn ngữ ngay bên trong nó là chỗ câu nói dối sống, và nó viết
được; đi vào thì một ô có cả hai cờ, mỗi cờ mang lý do riêng — không nổ, không cách ly, và
đọc lại được từng cờ đến từ chuỗi nào. Cái mất là không phát biểu được "mã hóa trung thực với
toàn bộ ngôn ngữ" thành định lý của chính hệ, chỉ phát biểu được cho từng mảnh; mô tả một hệ
**khác** thì không có giới hạn đó.

## Thư viện đi kèm

Cơ chế xuất xưởng với con số không về logic: không đẳng thức, không luật nổ, không bài
trung, không phủ định kép, không tính mở rộng, không quy nạp. Nhưng có một bộ thư viện đi
kèm, viết bằng chính V, để người dùng có cái mà làm việc.

Bộ đó là `lib/prelude.v`, nạp trước mọi file bằng đúng cái máy chạy file người dùng, và
nhúng vào nhị phân qua `tools/embed_prelude.py`. Không có dòng nào của nó nằm trong C++.

**Prelude — luôn nạp.**

- `SET`, `RELATION`, `MAP` là đối tượng thường: `SET ∈ SET`, `RELATION ∈ SET`,
  `MAP ∈ SET`, và hai luật `(relation is set)`, `(map is relation)`.
- `Domain` là hàm, và chuỗi của nó dừng ở `Apply Domain to Domain as MAP.` — dạng `as` là
  dạng đặt ra, cùng loại với `SET ∈ SET`.
- `Eq`: `(eq refl)`, `(eq symm)`, `(eq trans)`, `(eq subst)`, và `(eq from definition)` nối
  `Defined` sang `Eq`.
- `Function`, `TotalOn`, với `(function is relation)`, `(function unique)`, `(total on)`.
- `Approx`, với `(approx def)` và `(approx sum)` — một định nghĩa xấp xỉ, không phải định
  nghĩa duy nhất.

**Cố tình không có trong prelude**, vì nạp chúng là tuyên bố tin một thứ gì đó, nên phải do
người viết tự đặt vào file của mình:

- luật nối `Computed`, `Simplified`, `SimplifiedIf`, `Expanded` sang `Eq` — tin máy, tin tầng
  đại số;
- logic cổ điển — nổ, bài trung, phủ định kép;
- `(column is set)` — "đã làm cột thì phải thuộc SET". Nạp nó vào prelude thì `alice ∈ 5`
  sẽ lặng lẽ kéo theo `5 ∈ SET` cho mọi người; để ngoài thì ai cần phán xét mới tự nạp.

**Viết được, chưa đóng gói sẵn.** `Naturals`, `Index(N)`, `Tuple(N)`, quy nạp, lý thuyết tập
hợp (`Subset`, `Union`, `Inter`, `PowerOf`) — có ví dụ trong `examples/`, chưa vào prelude.

Không nạp logic cổ điển thì hành vi là paraconsistent, và đó là mặc định chứ không phải một
chế độ đặc biệt.

## Đóng gói lý thuyết

```
Theory Preorder {
    Let Carrier be a set.
    Let Below be a relation.
    Rule (refl):  for every x, if x in Carrier then (x, x) in Below.
    Rule (trans): for every x, y, z,
        if (x, y) in Below and (y, z) in Below then (x, z) in Below.
}

Import Preorder as Age with (Carrier := People, Below := Older).
```

Thân của `Theory` giữ nguyên ở dạng token, không parse, vì tên bên trong chưa có nghĩa cho
tới khi `Import` gán chúng vào đâu đó. `Import` chạy lại thân dưới một phép đổi tên:

- tên được gán trỏ thẳng vào đối tượng đã có;
- tên khai báo bên trong mà không được gán thành tên riêng của thể hiện — `First_basepoint`,
  `Second_basepoint` — nên hai lần import không giẫm lên nhau;
- nhãn mang tên thể hiện: `(trans)` thành `(Age trans)`;
- lý thuyết lồng nhau được: `Import` viết bên trong một `Theory` thì tên thể hiện con mang
  tên thể hiện ngoài, `(One Sub refl)`.

Danh tính một thể hiện là cặp (tên lý thuyết, ánh xạ); import lại đúng cặp đó là không làm
gì. Runtime không biết có lý thuyết nào tồn tại: import chỉ là chạy lại câu lệnh, nên luật
`Age trans` là một luật bình thường trong kho.

## Notation

```
Notation: "A manages B" means (A, B) in Boss.
```

Notation là cách viết, không phải cơ chế: dùng ở chỗ nào một mệnh đề đứng được, kể cả bên
trong luật với biến của luật ngồi trong lỗ, và nó mở ra đúng một mệnh đề. Vế phải không chứa
lượng từ.

Chữ nào trong mẫu là lỗ thì phải đọc vế phải mới biết, nên parser đọc hai lượt: lượt một coi
mọi chữ là lỗ và xem vế phải dùng chữ nào; lượt hai đọc lại với đúng những chữ đó. Chữ
không được dùng là chữ cố định.

**Hygiene.** Vế phải quy về đối tượng ngay lúc khai báo: `Boss` thành một `ObjectId`, không
còn là tên. Chỗ dùng chỉ điền term vào lỗ, không tra tên nào, nên biến của một luật trùng
tên với `Boss` không bắt được nó. Đây là đúng chỗ bản v0.3 hỏng. Lỗ đánh số ở một dải riêng,
tách khỏi chỉ số de Bruijn.

**Quá tải.** Hai mẫu cùng hình dạng phân biệt bằng `where`:

```
Notation: "A plus B is C" means (A, B, C) in VecSum where A in Vectors, B in Vectors.
Notation: "A plus B is C" means (A, B, C) in NumSum where A in Scalars, B in Scalars.
```

Dài nhất thắng; hai mẫu cùng khớp cùng độ dài mà không phân biệt được thì báo lỗi, không
chọn bừa. Giới hạn: `where` chỉ kiểm được khi chỗ điền là đối tượng cụ thể, còn biến của
luật thì bỏ qua.

## Quan hệ và nhãn của cơ chế

Mọi thứ dưới đây là đối tượng thường, gọi được bằng tên trong nguồn. Cơ chế ghi vào chúng;
lý thuyết đọc chúng. Không có cái nào mang nghĩa logic cho tới khi có luật nói về nó.

| tên | ai ghi, khi nào |
|---|---|
| `Holds` | đồng bộ hai chiều với kho mệnh đề, cho mọi mệnh đề đã có đại diện |
| `Column` | mỗi lần một đối tượng được dùng làm cột |
| `Defined` | `Let t = (a, b).` ghi `(t, (a, b))` |
| `Computed` | `Compute e.` ghi `(e, kết quả, sai số)` |
| `Less`, `LessEq` | `Compute a < b.`, `Compute a <= b.` ghi cả hai chiều |
| `NoValue` | `Compute` gặp chia cho 0 |
| `Simplified` | `Simplify e.` khi không phải triệt gì có chứa biến |
| `SimplifiedIf` | `Simplify e.` khi có triệt, kèm đại diện của điều kiện |
| `Expanded` | `Expand (a, b, c) as t.` ghi `((a, b, c), t)` |
| `Instance` | `Instantiate (h) at t.` ghi `(đại diện h, t, đại diện thể hiện)` |

Nhãn dựng đại diện và biểu thức: `Mem`, `NotMem`, `And`, `Or`, `Implies`, `Iff`, `Not`,
`All`, `Ex`, `Var`, và `Plus`, `Minus`, `Times`, `Div`, `Pow`.

## Vòng đời một chương trình

Chạy xong thì thế giới biến mất — đối tượng, ô, kho mệnh đề, nhật ký, tất cả. Không có gì
sống qua lần chạy.

Sửa một chứng minh nghĩa là sửa file `.v` rồi chạy lại, không phải sửa thế giới đang chạy.
Mệnh đề không động: không có thao tác nào sửa một mệnh đề đã cất. Mọi thứ trông như sửa đều
là chuyện khác — áp luật sinh ra mệnh đề mới đã thế biến xong và luật gốc nằm nguyên; đóng
scope sinh ra mệnh đề mới; suy ra lại theo đường khác thì thêm một lý do chứ không đổi mệnh
đề; thay tên hàng loạt là đọc rồi sinh bản mới; rút lại là quay về mốc, gỡ đi chứ không đổi.

Nói rồi thì không sửa lời đã nói. Nói thêm lời khác, hoặc rút lại cả đoạn bằng scope.

## Để sau

**Chia trường hợp kiểu xây dựng.** Hiện `or`-dùng chỉ có đường qua phản chứng, nên chứng
minh xây dựng không có lối. Vá bằng thư viện không được, vì chia nhánh cần mở hai scope rồi
hợp lại, mà luật thì không mở được scope. Nếu muốn phục vụ thì phải thêm một thao tác chia
nhánh vào cơ chế. Tạm đóng lại.

**API cho runtime.** Runtime là một thư viện và V là một front-end trên nó; mở một API để
người dùng gọi thẳng, dựng và biến đổi mệnh đề từ C hoặc C++, viết front-end khác. Điều này
ép ranh giới ngôn ngữ/runtime phải sạch — lá của cây mệnh đề là handle, không phải tên bề
mặt, đúng chỗ bản v0.3 hỏng. Ghi lại để làm sau; trước mắt làm V như một ngôn ngữ đã.

