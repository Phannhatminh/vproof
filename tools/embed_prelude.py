#!/usr/bin/env python3
"""Nhúng lib/prelude.v vào include/v/prelude.hpp."""
import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
src = (root / "lib" / "prelude.v").read_text()
(root / "include" / "v" / "prelude.hpp").write_text(
    '#pragma once\n\nnamespace v {\n\n'
    '// Nguồn của `lib/prelude.v`, nhúng vào để nhị phân tự đứng được.\n'
    '// Sửa file kia rồi chạy `tools/embed_prelude.py` để cập nhật chỗ này.\n'
    'inline const char* kPrelude = R"VPRELUDE(\n' + src + ')VPRELUDE";\n\n'
    '}  // namespace v\n')
print("đã nhúng", len(src), "ký tự")
