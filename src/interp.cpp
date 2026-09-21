#include "v/interp.hpp"

#include "v/prelude.hpp"
#include "v/symbolic.hpp"

#include <algorithm>

namespace v {

PropId Interp::ref(const std::string& label, int line) const {
  auto it = labels_.find(label);
  if (it == labels_.end()) throw ParseError(line, "chưa có nhãn (" + label + ")");
  return it->second;
}

void Interp::bind(const std::string& label, PropId p) {
  if (label.empty()) return;
  labels_[label] = p;
  if (!frames_.empty()) frames_.back().labels.push_back(label);
}

void Interp::whyChain(PropId p, int depth, Report& rep) const {
  std::string pad(depth * 2 + 2, ' ');
  const auto& rs = w_.propReasons(p);
  if (rs.empty()) {
    rep.lines.push_back(pad + "(chưa có lý do)");
    return;
  }
  for (const Reason& r : rs) {
    if (r.stipulated) {
      rep.lines.push_back(pad + "đặt ra ở dòng " + std::to_string(r.line));
      continue;
    }
    std::string head = pad + "suy ra bằng " + r.rule + ", dòng " + std::to_string(r.line);
    if (!r.binding.empty()) {
      head += " [";
      for (size_t i = 0; i < r.binding.size(); ++i)
        head += (i ? ", " : "") + r.binding[i].first + " := " + w_.show(r.binding[i].second);
      head += "]";
    }
    rep.lines.push_back(head);
    if (depth < 6)
      for (const Antecedent& a : r.antecedents) {
        rep.lines.push_back(pad + "  vì " + w_.showProp(a.prop));
        whyChain(a.prop, depth + 2, rep);
      }
  }
}

// Khớp một term cụ thể với một term có biến buộc ở chỉ số `depth`.
bool Interp::matchTerm(const Term& concrete, const Term& pattern, VarId depth,
                       ObjectId& witness) const {
  if (pattern.kind == TermKind::Var) {
    if (pattern.var != depth) return concrete.kind == TermKind::Var && concrete.var == pattern.var;
    if (concrete.kind != TermKind::Obj) return false;
    if (witness != kNoObject && witness != concrete.obj) return false;  // phải cùng một nhân chứng
    witness = concrete.obj;
    return true;
  }
  if (concrete.kind != pattern.kind) return false;
  if (pattern.kind == TermKind::Obj) return concrete.obj == pattern.obj;
  if (concrete.elems.size() != pattern.elems.size()) return false;
  for (size_t i = 0; i < pattern.elems.size(); ++i)
    if (!matchTerm(concrete.elems[i], pattern.elems[i], depth, witness)) return false;
  return true;
}

bool Interp::matchWitness(PropId concrete, PropId pattern, VarId depth,
                          ObjectId& witness) const {
  const Prop& a = w_.prop(concrete);
  const Prop& b = w_.prop(pattern);
  if (a.kind != b.kind) return false;
  if (b.kind == PropKind::Atom)
    return a.positive == b.positive && matchTerm(a.subject, b.subject, depth, witness) &&
           matchTerm(a.column, b.column, depth, witness);
  VarId inner = (b.kind == PropKind::ForAll || b.kind == PropKind::Exists) ? depth + 1 : depth;
  if (b.left != kNoProp && !matchWitness(a.left, b.left, inner, witness)) return false;
  if (b.right != kNoProp && !matchWitness(a.right, b.right, inner, witness)) return false;
  return true;
}

Rational Interp::evalExpr(const Term& t, int line) {
  if (t.kind == TermKind::Obj) {
    const Object& o = w_.obj(t.obj);
    if (o.kind != Kind::Numeral)
      throw ParseError(line, "không phải số: " + o.name);
    return o.value;
  }
  if (t.kind != TermKind::Tuple || t.elems.size() != 3 || t.elems[0].kind != TermKind::Obj)
    throw ParseError(line, "không phải biểu thức số");

  const std::string& op = w_.obj(t.elems[0].obj).name;
  Rational a = evalExpr(t.elems[1], line), b = evalExpr(t.elems[2], line);
  if (op == "Plus") return a + b;
  if (op == "Minus") return a - b;
  if (op == "Times") return a * b;
  if (op == "Div") return a / b;  // ném RationalDivByZero, bắt ở chỗ gọi
  throw ParseError(line, "phép toán lạ: " + op);
}

void Interp::exec(const Stmt& s, Report& rep) {
  switch (s.kind) {
    case StmtKind::Let: {
      if (s.isTupleLet) {
        std::vector<Term> elems;
        for (const std::string& e : s.tupleElems) {
          auto it = names_.find(e);
          if (it == names_.end()) throw ParseError(s.line, "chưa khai báo: " + e);
          elems.push_back(Term::of(it->second));
        }
        ObjectId t = w_.declare(s.names[0]);
        names_[s.names[0]] = t;
        ObjectId built = w_.resolve(Term::ofTuple(elems));
        // `Let t = (a, b)` ghi vào quan hệ của cơ chế, không ghi vào Eq.
        w_.tellIn(w_.tuple({t, built}), w_.tag("Defined"), Reason::stipulate(s.line));
      } else {
        // `be a set` / `be a relation` là nội dung, không phải loại của cơ chế:
        // nó ghi một fact vào SET hoặc RELATION, đúng như mọi fact khác.
        ObjectId column = kNoObject;
        if (s.letKind == "set") column = names_["SET"];
        else if (s.letKind == "relation") column = names_["RELATION"];
        else if (s.letKind == "map") column = names_["MAP"];
        for (const std::string& n : s.names) {
          ObjectId o = w_.declare(n);
          names_[n] = o;
          if (column != kNoObject)
            w_.tellIn(o, column, Reason::stipulate(s.line));
        }
      }
      return;
    }

    case StmtKind::Assume:
    case StmtKind::Rule: {
      pv_.assume(s.prop, s.line);
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::ByRule: {
      PropId rule = ref(s.refs[0], s.line);
      std::vector<ObjectId> args;
      for (const Term& a : s.args) args.push_back(w_.resolve(a));
      auto r = w_.applyRule(rule, args, s.refs[0], s.line);
      if (!r.ok)
        throw ParseError(s.line, "bước không đi được, chưa có: " + w_.showProp(r.missing));
      if (r.conclusion != s.prop)
        throw ParseError(s.line, "kết luận viết ra không khớp: luật cho `" +
                                     w_.showProp(r.conclusion) + "`");
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::From: {
      std::vector<PropId> ps;
      for (const std::string& r : s.refs) ps.push_back(ref(r, s.line));
      const Prop& out = w_.prop(s.prop);
      Prover::Result res;

      if (ps.size() == 1 && w_.prop(ps[0]).kind == PropKind::And &&
          (w_.prop(ps[0]).left == s.prop || w_.prop(ps[0]).right == s.prop)) {
        res = pv_.elimAnd(ps[0], w_.prop(ps[0]).left == s.prop, s.line);
      } else if (ps.size() == 1 && w_.prop(ps[0]).kind == PropKind::Iff &&
                 out.kind == PropKind::Implies) {
        res = pv_.elimIff(ps[0], w_.prop(ps[0]).left == out.left, s.line);
      } else if (out.kind == PropKind::And && ps.size() == 2) {
        res = pv_.introAnd(ps[0], ps[1], s.line);
      } else if (out.kind == PropKind::Or && ps.size() == 1) {
        bool left = out.left == ps[0];
        res = pv_.introOr(ps[0], left ? out.right : out.left, left, s.line);
      } else if (out.kind == PropKind::Iff && ps.size() == 2) {
        res = pv_.introIff(ps[0], ps[1], s.line);
      } else if (out.kind == PropKind::Exists && ps.size() == 1) {
        // Nhân chứng đọc ra bằng một lượt khớp mẫu giữa tiền đề và thân kết
        // luận: chỗ nào thân có biến buộc thì tiền đề có đối tượng, và mọi
        // chỗ như vậy phải là cùng một đối tượng. Tra chứ không thử.
        ObjectId witness = kNoObject;
        if (!matchWitness(ps[0], out.left, 0, witness) || witness == kNoObject)
          throw ParseError(s.line, "không đọc được nhân chứng từ tiền đề");
        res = pv_.introExists(ps[0], witness, out.binderName, s.line);
      } else {
        throw ParseError(s.line, "không nhận ra bước này");
      }

      if (!res.ok)
        throw ParseError(s.line, res.missing != kNoProp
                                     ? "chưa có: " + w_.showProp(res.missing)
                                     : res.error);
      if (res.prop != s.prop)
        throw ParseError(s.line, "kết luận không khớp: bước cho `" + w_.showProp(res.prop) + "`");
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::Suppose: {
      pv_.suppose(s.prop, s.label, s.line);
      frames_.push_back({});
      bind(s.label, s.prop);
      return;
    }

    case StmtKind::Take: {
      pv_.take(s.names[0], s.line);
      frames_.push_back({});
      names_[s.names[0]] = pv_.freshOf();
      frames_.back().names.push_back(s.names[0]);
      return;
    }

    case StmtKind::TakeIn: {
      auto it = names_.find(s.setName);
      if (it == names_.end()) throw ParseError(s.line, "chưa khai báo: " + s.setName);
      pv_.takeIn(s.names[0], it->second, s.line);
      frames_.push_back({});
      names_[s.names[0]] = pv_.freshOf();
      frames_.back().names.push_back(s.names[0]);
      return;
    }

    case StmtKind::TakeFrom: {
      auto r = pv_.takeFrom(s.names[0], ref(s.refs[0], s.line), s.line);
      if (!r.ok) throw ParseError(s.line, r.error);
      frames_.push_back({});
      names_[s.names[0]] = pv_.freshOf();
      frames_.back().names.push_back(s.names[0]);
      return;
    }

    case StmtKind::Close:
      return;  // `}` chỉ đóng thân; `Hence` mới làm việc

    case StmtKind::Hence: {
      auto r = pv_.hence(s.prop, s.label, s.line);
      if (!r.ok)
        throw ParseError(s.line, r.missing != kNoProp
                                     ? "chưa có: " + w_.showProp(r.missing)
                                     : r.error);
      if (!frames_.empty()) {
        for (const std::string& l : frames_.back().labels) labels_.erase(l);
        for (const std::string& n : frames_.back().names) names_.erase(n);
        frames_.pop_back();
      }
      bind(s.label, r.prop);  // nhãn của Hence thuộc scope bên ngoài
      return;
    }

    case StmtKind::Absurd: {
      std::vector<PropId> ps;
      for (const std::string& r : s.refs) ps.push_back(ref(r, s.line));
      Prover::Result r;
      if (ps.size() == 2) r = pv_.absurd(ps[0], ps[1], s.line);
      else if (ps.size() == 3) r = pv_.absurdFromOr(ps[0], ps[1], ps[2], s.line);
      else throw ParseError(s.line, "Absurd cần hai hoặc ba mệnh đề");
      if (!r.ok) throw ParseError(s.line, r.error);
      return;
    }

    case StmtKind::Apply: {
      ObjectId F = w_.resolve(s.fn), a = w_.resolve(s.arg);

      if (!s.names.empty()) {
        // Dạng `as`: đặt ra giá trị canonical, không tra miền. Đây là chỗ cắt
        // vòng — `Apply Domain to Domain as MAP.` cùng loại với `SET ∈ SET`.
        auto it = names_.find(s.names[0]);
        if (it == names_.end()) throw ParseError(s.line, "chưa khai báo: " + s.names[0]);
        applied_[{F, a}] = it->second;
        w_.tellIn(w_.tuple({a, it->second}), F, Reason::stipulate(s.line));
        return;
      }

      // Áp hàm không phải thao tác của cơ chế: nó dịch xuống việc tra miền rồi
      // lấy một nhân chứng. Miền là kết quả của một bước áp hàm khác.
      // `Domain` là tên do prelude khai báo, không phải nhãn của cơ chế.
      auto domIt = names_.find("Domain");
      if (domIt == names_.end()) throw ParseError(s.line, "chưa có `Domain`");
      auto d = applied_.find({domIt->second, F});
      if (d == applied_.end())
        throw ParseError(s.line, "chưa có Domain(" + w_.show(F) + ") — cần `Apply Domain to " +
                                     w_.show(F) + ".` trước");
      if (!w_.toldIn(a, d->second))
        throw ParseError(s.line, "chưa xác lập `" + w_.show(a) + " in " + w_.show(d->second) + "`");

      auto have = applied_.find({F, a});
      if (have != applied_.end()) return;  // đã có giá trị canonical

      ObjectId value = w_.declare(w_.show(F) + "(" + w_.show(a) + ")");
      applied_[{F, a}] = value;
      Reason why = Reason::derive("áp hàm", s.line);
      w_.tellIn(w_.tuple({a, value}), F, std::move(why));
      rep.lines.push_back("  áp    dòng " + std::to_string(s.line) + ": " + w_.show(value));
      return;
    }

    case StmtKind::Theory: {
      theories_[s.names[0]] = s;  // ghi lại, không chạy
      return;
    }

    case StmtKind::Import: {
      auto it = theories_.find(s.names[0]);
      if (it == theories_.end()) throw ParseError(s.line, "chưa có lý thuyết " + s.names[0]);
      const Stmt& th = it->second;

      // Danh tính thể hiện: tên lý thuyết cộng ánh xạ. Import lại là no-op.
      std::string key = s.names[0];
      for (const auto& m : th.mapping) (void)m;
      for (const auto& m : s.mapping) key += "|" + m.first + "=" + m.second;
      if (!imported_.insert(key).second) return;

      std::map<std::string, std::string> rename;
      for (const auto& m : s.mapping) rename[m.first] = m.second;

      // Tên khai báo bên trong mà không được gán thì đổi thành tên riêng của
      // thể hiện, để hai lần import không giẫm lên nhau.
      for (size_t i = 0; i + 2 < th.body.size(); ++i) {
        if (th.body[i].first != "Let") continue;
        size_t j = i + 1;
        while (j < th.body.size() && th.body[j].first != "be" && th.body[j].first != "=") {
          if (th.bodyWord[j] && !rename.count(th.body[j].first))
            rename[th.body[j].first] = s.alias + "_" + th.body[j].first;
          ++j;
        }
      }

      // Nhãn đứng ngay sau các từ mở nhãn; chúng cũng mang tên thể hiện.
      auto opensLabel = [](const std::string& w) {
        return w == "Rule" || w == "Assume" || w == "Suppose" || w == "Hence" ||
               w == "rule" || w == "from" || w == "From";
      };

      // Lý thuyết lồng nhau: tên của thể hiện con và tên lý thuyết khai bên
      // trong cũng phải mang tên thể hiện ngoài, nếu không thì hai lần import
      // cùng một lý thuyết sẽ giẫm lên nhau.
      auto nestedName = [&](size_t i) {
        if (i >= 1 && th.body[i - 1].first == "Theory") return true;
        if (i >= 3 && th.body[i - 1].first == "as" && th.body[i - 3].first == "Import")
          return true;
        return false;
      };

      std::vector<Parser::Token> out;
      for (size_t i = 0; i < th.body.size(); ++i) {
        // Giữ số dòng gốc trong thân lý thuyết, để lỗi bên trong báo đúng chỗ.
        Parser::Token t{th.body[i].first, th.body[i].second, th.bodyWord[i], th.bodyNumber[i]};
        if (t.word) {
          auto r = rename.find(t.text);
          if (r != rename.end()) t.text = r->second;
          else if (nestedName(i)) t.text = s.alias + " " + t.text;
          else if (i >= 2 && th.body[i - 1].first == "(" && opensLabel(th.body[i - 2].first))
            t.text = s.alias + " " + t.text;
        }
        out.push_back(std::move(t));
      }

      Parser replay(w_, names_, applied_, labels_);
      replay.beginTokens(std::move(out));
      while (replay.more()) exec(replay.next(), rep);
      return;
    }

    case StmtKind::Instantiate: {
      PropId all = ref(s.refs[0], s.line);
      const Prop& p = w_.prop(all);
      if (p.kind != PropKind::ForAll && p.kind != PropKind::Exists)
        throw ParseError(s.line, "cần một mệnh đề có lượng từ");
      ObjectId t = w_.resolve(s.expr);

      // Phép thế trên đại diện không viết lại: nó là giải mã, dùng đúng
      // `instantiate` mà áp luật đang dùng, rồi mã hoá lại. Hai phép thế do
      // đó không thể lệch nhau.
      PropId inst = w_.instantiate(all, t);
      w_.tellIn(w_.tuple({w_.represent(all), t, w_.represent(inst)}), w_.tag("Instance"),
                Reason::derive("thể hiện", s.line));
      bind(s.label, inst);
      rep.lines.push_back("  thể   dòng " + std::to_string(s.line) + ": " + w_.showProp(all) +
                          "  tại " + w_.show(t) + "  ->  " + w_.showProp(inst));
      return;
    }

    case StmtKind::Expand: {
      if (s.expr.kind != TermKind::Tuple)
        throw ParseError(s.line, "Expand cần một dãy liệt kê");
      ObjectId listed = w_.resolve(s.expr);

      // Dạng chuẩn của một dãy là dạng hàm: một set các cặp (chỉ số, phần tử).
      // Dạng liệt kê không bị bỏ — hai đối tượng cùng tồn tại, và bước này là
      // chỗ nối chúng, tường minh.
      std::string name = s.names.empty() ? "fn" + w_.show(listed) : s.names[0];
      ObjectId fn = w_.declare(name);
      if (!s.names.empty()) names_[s.names[0]] = fn;
      for (size_t i = 0; i < s.expr.elems.size(); ++i) {
        ObjectId idx = w_.numeral(Rational(static_cast<long long>(i + 1)));
        ObjectId elem = w_.resolve(s.expr.elems[i]);
        w_.tellIn(w_.tuple({idx, elem}), fn, Reason::derive("khai triển", s.line));
      }
      w_.tellIn(w_.tuple({listed, fn}), w_.tag("Expanded"), Reason::derive("khai triển", s.line));
      rep.lines.push_back("  khai  dòng " + std::to_string(s.line) + ": " + w_.show(listed) +
                          "  ->  " + name);
      return;
    }

    case StmtKind::Simplify: {
      ObjectId expr = w_.resolve(s.expr);
      std::vector<Term> nonzero;
      ObjectId result;
      try {
        result = w_.resolve(simplify(w_, s.expr, &nonzero));
      } catch (const std::exception& e) {
        throw ParseError(s.line, std::string("không rút gọn được: ") + e.what());
      }

      std::string shown = "  gọn  dòng " + std::to_string(s.line) + ": " + w_.show(expr) +
                          "  ->  " + w_.show(result);
      if (nonzero.empty()) {
        // Không triệt gì có chứa biến, nên đẳng thức không kèm điều kiện.
        w_.tellIn(w_.tuple({expr, result}), w_.tag("Simplified"),
                  Reason::derive("đại số", s.line));
      } else {
        // Có triệt: điều kiện đi ra ngoài thành một mệnh đề phải xác lập, chứ
        // không nằm ngầm trong đẳng thức.
        ObjectId Eq = names_["Eq"], zero = w_.numeral(Rational(0));
        PropId cond = kNoProp;
        for (const Term& t : nonzero) {
          PropId one = w_.atom(Term::ofTuple({t, Term::of(zero)}), Term::of(Eq), false);
          cond = cond == kNoProp ? one : w_.conj(cond, one);
        }
        w_.tellIn(w_.tuple({expr, result, w_.represent(cond)}), w_.tag("SimplifiedIf"),
                  Reason::derive("đại số", s.line));
        shown += "   với điều kiện  " + w_.showProp(cond);
        bind(s.label, cond);  // trích lại điều kiện bằng nhãn, khỏi chép tay
      }
      rep.lines.push_back(shown);
      return;
    }

    case StmtKind::Compute: {
      if (!s.cmpOp.empty()) {
        // Máy quyết định so sánh và ghi lại cả hai chiều: đúng thì bật cờ ∈,
        // sai thì bật cờ ∉. Lý do ghi rõ đây là máy tính ra.
        Rational a = evalExpr(s.expr, s.line), b = evalExpr(s.rhs, s.line);
        bool flip = s.cmpOp[0] == '>';
        bool strict = s.cmpOp.size() == 1;
        const Rational& lo = flip ? b : a;
        const Rational& hi = flip ? a : b;
        bool truth = strict ? lo < hi : lo <= hi;
        ObjectId pair = w_.tuple({w_.numeral(lo), w_.numeral(hi)});
        ObjectId rel = w_.tag(strict ? "Less" : "LessEq");
        Reason why = Reason::derive("máy tính ra", s.line);
        if (truth) w_.tellIn(pair, rel, std::move(why));
        else w_.tellOut(pair, rel, std::move(why));
        rep.lines.push_back("  tính  dòng " + std::to_string(s.line) + ": " + w_.show(pair) +
                            (truth ? " ∈ " : " ∉ ") + w_.show(rel));
        return;
      }
      ObjectId expr = w_.resolve(s.expr);
      try {
        Rational value = evalExpr(s.expr, s.line);
        // Máy luôn trả kèm sai số. Bốn phép trên hữu tỉ thì sai số bằng 0.
        ObjectId result = w_.numeral(value);
        ObjectId error = w_.numeral(Rational(0));
        w_.tellIn(w_.tuple({expr, result, error}), w_.tag("Computed"),
                  Reason::derive("máy tính ra", s.line));
        rep.lines.push_back("  tính  dòng " + std::to_string(s.line) + ": " + w_.show(expr) +
                            " = " + w_.show(result) + " (sai số " + w_.show(error) + ")");
      } catch (const RationalDivByZero&) {
        // Không lỗi, không quy ước giá trị: ghi một fact vào quan hệ của cơ chế.
        w_.tellIn(expr, w_.tag("NoValue"), Reason::derive("chia cho 0", s.line));
        rep.lines.push_back("  tính  dòng " + std::to_string(s.line) + ": " + w_.show(expr) +
                            " không có giá trị");
      }
      return;
    }

    case StmtKind::Therefore: {
      ++rep.checksRun;
      bool ok = w_.holds(s.prop);
      if (!ok) ++rep.checksFailed;
      rep.lines.push_back(std::string(ok ? "  ok   " : "  SAI  ") + "dòng " +
                          std::to_string(s.line) + ": " + w_.showProp(s.prop));
      return;
    }

    case StmtKind::Why: {
      rep.lines.push_back("  vì sao " + w_.showProp(s.prop) + ":");
      whyChain(s.prop, 0, rep);
      return;
    }
  }
}

Interp::Report Interp::run(const std::string& source) {
  Report rep;
  // Nhãn của cơ chế dùng được như tên thường, để viết luật trên đại diện.
  for (const char* t : {"Mem", "NotMem", "And", "Or", "Implies", "Iff", "Not", "All", "Ex",
                        "Defined", "Computed", "NoValue",
                        "Plus", "Minus", "Times", "Div", "Less", "LessEq", "Pow", "Simplified", "SimplifiedIf", "Expanded", "Var", "Instance"})
    names_[t] = w_.tag(t);
  names_["Holds"] = w_.holdsColumn();
  names_["Column"] = w_.columnTag();

  // Thư viện tối thiểu là nội dung viết bằng V, không phải mã C++. Nạp nó
  // bằng đúng cái máy chạy file người dùng.
  if (!preludeLoaded_) {
    preludeLoaded_ = true;
    Report ignored;
    Parser pre(w_, names_, applied_, labels_);
    pre.begin(kPrelude);
    while (pre.more()) exec(pre.next(), ignored);
  }
  parser_.begin(source);
  while (parser_.more()) exec(parser_.next(), rep);
  return rep;
}

}  // namespace v
