#!/usr/bin/env python3
"""Generate tools/hzk16_data.h (HZK16 glyph array) and tools/gb2312_map.h
(Unicode -> GB2312 offset map) from tools/HZK16 (267616 bytes, GB2312
16x16 dot-matrix, 32 bytes per glyph, offset = (qu-1)*94+(wei-1))."""
import os, sys

root = os.path.join(os.path.dirname(__file__), "..", "tools", "vterm")
hzk = open(os.path.join(root, "HZK16"), "rb").read()
assert len(hzk) == 267616, len(hzk)

# Unicode -> GB2312 offset, from python's gb2312 codec
entries = {}   # uni -> quwei index (0-based)
for u in list(range(0x4E00, 0xA000)) + list(range(0xFF01, 0xFF5F)) + \
         list(range(0x3000, 0x303F)) + [0xB7, 0xB0, 0xB1, 0xB2, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026, 0x00D7, 0x00F7, 0x00B1]:
    try:
        b = chr(u).encode("gb2312")
    except UnicodeEncodeError:
        continue
    if len(b) == 2:
        q = (b[0] - 0xA1) * 94 + (b[1] - 0xA1)
        entries[u] = q

items = sorted(entries.items())
# glyph = hzk[offset*32 : offset*32+32]
with open(os.path.join(root, "unicode_glyph.h"), "w") as f:
    f.write("/* Auto-generated from HZK16: Unicode code point -> 16x16 glyph (32 B).\n")
    f.write(" * Sorted by Unicode; binary search at render time. */\n")
    f.write("#ifndef UNICODE_GLYPH_H\n#define UNICODE_GLYPH_H\n\n")
    f.write("typedef struct { uint16_t uni; uint8_t glyph[32]; } unicode_glyph_t;\n\n")
    f.write("static const unicode_glyph_t k_unicode_glyphs[] = {\n")
    for u, q in items:
        g = ", ".join(f"0x{b:02X}" for b in hzk[q*32:q*32+32])
        f.write(f"    {{ 0x{u:04X}, {{ {g} }} }},\n")
    f.write("};\n")
    f.write(f"#define UNICODE_GLYPH_COUNT {len(items)}\n\n#endif\n")

print(f"glyph entries: {len(items)} ({len(items)*34//1024} KB)")
print("wrote unicode_glyph.h")
