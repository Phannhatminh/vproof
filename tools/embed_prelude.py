#!/usr/bin/env python3
"""Embed lib/prelude.v into include/v/prelude.hpp."""
import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
src = (root / "lib" / "prelude.v").read_text()
(root / "include" / "v" / "prelude.hpp").write_text(
    '#pragma once\n\nnamespace v {\n\n'
    '// Source of `lib/prelude.v`, embedded so the binary stands on its own.\n'
    '// Edit that file, then run `tools/embed_prelude.py` to refresh this one.\n'
    'inline const char* kPrelude = R"VPRELUDE(\n' + src + ')VPRELUDE";\n\n'
    '}  // namespace v\n')
print("embedded", len(src), "characters")
