#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "v/interp.hpp"

using namespace v;

static int failures = 0;

static std::string slurp(const std::string& path) {
  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

static void runFile(const std::string& path, int expectedChecks) {
  Interp interp;
  try {
    auto rep = interp.run(slurp(path));
    bool ok = rep.checksFailed == 0 && rep.checksRun == expectedChecks;
    std::cout << (ok ? "ok    " : "FAIL  ") << path << " — " << rep.checksRun
              << " kiểm tra, " << rep.checksFailed << " sai\n";
    if (!ok) ++failures;
  } catch (const std::exception& e) {
    std::cout << "FAIL  " << path << " — " << e.what() << "\n";
    ++failures;
  }
}

static void expectError(const std::string& name, const std::string& source,
                        const std::string& fragment) {
  Interp interp;
  try {
    interp.run(source);
    std::cout << "FAIL  " << name << " — lẽ ra phải hỏng\n";
    ++failures;
  } catch (const std::exception& e) {
    std::string msg = e.what();
    bool ok = msg.find(fragment) != std::string::npos;
    std::cout << (ok ? "ok    " : "FAIL  ") << name << " — " << msg << "\n";
    if (!ok) ++failures;
  }
}

// Every ```v block in the language reference is a complete program. Run them all so the
// reference cannot drift from the code.
static void runDocBlocks(const std::string& path) {
  std::string text = slurp(path);
  if (text.empty()) {
    std::cout << "FAIL  " << path << " — không đọc được\n";
    ++failures;
    return;
  }
  size_t pos = 0;
  int n = 0;
  while ((pos = text.find("```v\n", pos)) != std::string::npos) {
    size_t start = pos + 5;
    size_t end = text.find("```", start);
    if (end == std::string::npos) break;
    std::string src = text.substr(start, end - start);
    pos = end + 3;
    ++n;

    Interp interp;
    try {
      auto rep = interp.run(src);
      bool ok = rep.checksFailed == 0;
      std::cout << (ok ? "ok    " : "FAIL  ") << path << " khối " << n << " — " << rep.checksRun
                << " kiểm tra, " << rep.checksFailed << " sai\n";
      if (!ok) ++failures;
    } catch (const std::exception& e) {
      std::cout << "FAIL  " << path << " khối " << n << " — " << e.what() << "\n";
      ++failures;
    }
  }
  if (n == 0) {
    std::cout << "FAIL  " << path << " — không có khối nào\n";
    ++failures;
  }
}

int main(int argc, char** argv) {
  std::string dir = argc > 1 ? argv[1] : "examples";
  std::string docs = argc > 2 ? argv[2] : "docs";
  runDocBlocks(docs + "/language.md");
  runDocBlocks(docs + "/language.en.md");
  runFile(dir + "/boss.v", 1);
  runFile(dir + "/cases.v", 1);
  runFile(dir + "/forall.v", 2);
  runFile(dir + "/exists.v", 2);
  runFile(dir + "/classical.v", 2);
  runFile(dir + "/arith.v", 3);
  runFile(dir + "/kinds.v", 4);
  runFile(dir + "/eq.v", 3);
  runFile(dir + "/fn.v", 2);
  runFile(dir + "/index.v", 2);
  runFile(dir + "/function.v", 3);
  runFile(dir + "/theory.v", 6);
  runFile(dir + "/symbolic.v", 4);
  runFile(dir + "/sequence.v", 6);
  runFile(dir + "/reflect.v", 4);
  runFile(dir + "/notation.v", 5);
  runFile(dir + "/column.v", 4);
  runFile(dir + "/induction.v", 2);
  runFile(dir + "/reals.v", 2);

  // A cell nobody has spoken about checks as neither polarity — which is not the same as
  // "false".
  {
    Interp interp;
    auto rep = interp.run(
        "Let alice be an entity.\n"
        "Therefore alice in SET.\n"
        "Therefore alice notin SET.\n");
    bool ok = rep.checksRun == 2 && rep.checksFailed == 2;
    std::cout << (ok ? "ok    " : "FAIL  ") << "ô vắng: cả hai cực đều không kiểm được\n";
    if (!ok) ++failures;
  }

  expectError("thiếu tiền đề",
              "Let a be an entity.\n"
              "Let A, B be sets.\n"
              "Rule (r): for every x, if x in A then x in B.\n"
              "By rule (r) applied to (a), it follows that a in B.\n",
              "chưa có: a ∈ A");

  expectError("kết luận nhắc tới nhân chứng",
              "Let a be an entity.\n"
              "Let P be a set.\n"
              "Assume (h): a in P.\n"
              "From (h), it follows that there exists z such that z in P as (ex).\n"
              "Take w from (ex) {\n"
              "}\n"
              "Hence (r): w in P.\n",
              "nhắc tới nhân chứng");

  expectError("kết luận viết ra không khớp",
              "Let a be an entity.\n"
              "Let A, B, C be sets.\n"
              "Rule (r): for every x, if x in A then x in B.\n"
              "Assume (h): a in A.\n"
              "By rule (r) applied to (a), it follows that a in C.\n",
              "không khớp");

  expectError("nhãn trong scope không rò ra ngoài",
              "Let a be an entity.\n"
              "Let A, B be sets.\n"
              "Assume (h): a in A.\n"
              "Suppose (s): a in B {\n"
              "    From (h), (s), it follows that a in A and a in B as (both).\n"
              "}\n"
              "Hence (r): if a in B then a in A and a in B.\n"
              "From (both), it follows that a in A.\n",
              "chưa có nhãn (both)");

  expectError("tên đối tượng tạm không rò ra ngoài",
              "Let A, B be sets.\n"
              "Rule (ab): for every x, if x in A then x in B.\n"
              "Take u with u in A {\n"
              "    By rule (ab) applied to (u), it follows that u in B as (t1).\n"
              "}\n"
              "Hence (ac): for every x, if x in A then x in B.\n"
              "Assume (bad): u in A.\n",
              "chưa khai báo: u");

  expectError("F(a) trước khi có bước áp",
              "Let F be a map.\n"
              "Let a be an entity.\n"
              "Let P be a set.\n"
              "Apply Domain to F.\n"
              "Assume (h): a in Domain(F).\n"
              "Assume (p): F(a) in P.\n",
              "chưa có — cần `Apply F to");

  expectError("áp hàm khi chưa xác lập đối số thuộc miền",
              "Let F be a map.\n"
              "Let a be an entity.\n"
              "Apply Domain to F.\n"
              "Apply F to a.\n",
              "chưa xác lập `a in Domain(F)`");

  expectError("ChildOf không thuộc MAP nên ChildOf(lan) không hình thành",
              "Let ChildOf be a relation.\n"
              "Let lan be an entity.\n"
              "Apply Domain to ChildOf.\n",
              "chưa xác lập `ChildOf in MAP`");

  // Without the condition established, the conditional rule does not go through.
  expectError("triệt ước mà chưa xác lập điều kiện",
              "Let x be an entity.\n"
              "Rule (r): for every u, v, k,\n"
              "    if (u, v, k) in SimplifiedIf and k in Holds then (u, v) in Eq.\n"
              "Simplify x / x.\n"
              "By rule (r) applied to (x/x, 1, [(x, 0) notin Eq]),\n"
              "    it follows that (x/x, 1) in Eq.\n",
              "chưa có");

  // The hygiene bug of v0.3: a rule variable with the same name as a name on the notation's
  // right-hand side captured it. Here the right-hand side is resolved to objects at
  // declaration, so the use site looks up no names at all.
  expectError("hai mẫu cùng khớp mà không phân biệt được",
              "Let R, S be relations.\n"
              "Let a, b be entities.\n"
              "Notation: \"A near B\" means (A, B) in R.\n"
              "Notation: \"A near B\" means (A, B) in S.\n"
              "Assume (h): a near b.\n",
              "nhiều mẫu cùng khớp");

  expectError("notation không bị bắt biến",
              "Let Boss, Mentor be relations.\n"
              "Let alice, bob be entities.\n"
              "Notation: \"A manages B\" means (A, B) in Boss.\n"
              "Rule (cap): for every Boss, for every x, y,\n"
              "    if (x, y) in Boss then x manages y.\n"
              "Assume (h): (alice, bob) in Mentor.\n"
              "By rule (cap) applied to (Mentor, alice, bob),\n"
              "    it follows that (alice, bob) in Mentor.\n",
              "không khớp");

  expectError("lỗi trong thân lý thuyết báo đúng dòng gốc",
              "Theory Broken {\n"
              "    Let Carrier be a set.\n"
              "    Assume (bad): zzz in Carrier.\n"
              "}\n"
              "Let A be a set.\n"
              "Import Broken as B with (Carrier := A).\n",
              "dòng 3");

  expectError("chia cho 0 ở tầng đại số",
              "Let x be an entity.\n"
              "Simplify x / 0.\n",
              "không rút gọn được");

  expectError("tên chưa khai báo",
              "Let A be a set.\n"
              "Assume (h): zz in A.\n",
              "chưa khai báo");

  std::cout << (failures ? "\nFAILURES: " : "\nall passed, failures: ") << failures << "\n";
  return failures ? 1 : 0;
}
