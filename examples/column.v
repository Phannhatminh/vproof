-- Cơ chế không tra gì trước khi ghi vào một ô: `alice in 5` viết được.
-- Nhưng nó ghi lại, để lý thuyết có đủ dữ kiện mà tự phán xét.

Let A be a set.
Let alice be an entity.

Assume (h1): alice in A.
Therefore A in Column.

-- Câu vô nghĩa vẫn viết được. Không ai chặn, và cũng không ai giấu.
Assume (h2): alice in 5.
Therefore 5 in Column.

-- Lý thuyết tự viết luật của nó. Đây là nội dung, không phải cơ chế.
Rule (column is set): for every c, if c in Column then c in SET.

-- Với A thì luật cho ra đúng thứ đã biết.
By rule (column is set) applied to (A), it follows that A in SET.
Therefore A in SET.

-- Với 5 thì nó cho ra `5 in SET`. Muốn đó là mâu thuẫn thì phải có người
-- nói `5 notin SET` rồi đi chuỗi tới đây — cơ chế không tự làm việc đó.
By rule (column is set) applied to (5), it follows that 5 in SET.
Therefore 5 in SET.
Why 5 in Column.
