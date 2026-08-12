#!/usr/bin/env python3
"""Regenerate tools/xterm_seqs.h from an xterm.js checkout.

Usage: python3 scripts/gen-xterm-seqs.py [path-to-xterm.js-src]
"""
import re, glob, sys, os

root = sys.argv[1] if len(sys.argv) > 1 else "/tmp/xterm.js/src"
seqs = set()
for f in glob.glob(os.path.join(root, "**", "*.test.ts"), recursive=True):
    txt = open(f, encoding="utf-8").read()
    for m in re.finditer(r"'((?:\\x1b|\\u001b)[^']*?)'", txt):
        s = m.group(1).replace("\\x1b", "\x1b").replace("\\u001b", "\x1b")
        seqs.add(s)
seqs = sorted(seqs)
out = os.path.join(os.path.dirname(__file__), "..", "tools", "vterm", "xterm_seqs.h")
with open(out, "w") as f:
    f.write("/* Auto-generated from xterm.js test suite (MIT), src tests.\n")
    f.write(" * Escape sequences exercised by xterm.js tests, used as a\n")
    f.write(" * smoke-test corpus for the libvterm pipeline. Regenerate with\n")
    f.write(" * the script in scripts/gen-xterm-seqs.py.\n */\n")
    f.write("#ifndef XTERM_SEQS_H\n#define XTERM_SEQS_H\n\n")
    f.write("static const char *const k_xterm_seqs[] = {\n")
    for s in seqs:
        esc = "".join(f"\\x{b:02x}" for b in s.encode("latin-1", "ignore"))
        f.write(f'    "{esc}",\n')
    f.write("};\n")
    f.write(f"#define XTERM_SEQS_COUNT {len(seqs)}\n\n#endif\n")
print(f"wrote {out} ({len(seqs)} sequences)")
