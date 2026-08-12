#!/usr/bin/env python3
"""Render the full character inventory to a PNG for visual inspection.

Usage: python3 scripts/gen-char-table.py [out.png]
"""
import re, sys, os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), "..")

def load_table(fn):
    data = open(os.path.join(ROOT, "tools", "vterm", fn)).read()
    out = []
    for m in re.finditer(r'\{ 0x([0-9A-Fa-f]{4,8})(?:, (\d))?, \{ ([^}]+) \}', data):
        cp = int(m.group(1), 16)
        w = int(m.group(2)) if m.group(2) else 1
        g = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', m.group(3))]
        out.append((cp, w, g))
    return sorted(out)

symbols = load_table("symbol_glyphs.h")
emojis = load_table("emoji_glyphs.h")
half = [(cp, w, g[:16]) for cp, w, g in symbols if w == 1]

def rendered_glyph(cp, g, w):
    return (w, g[:32])  # w=1: 8x16, w=2: 16x16 (unifont source as-is)
half = [(cp, w, g[:16]) for cp, w, g in symbols if w == 1]
full = [(cp, w, g[:32]) for cp, w, g in symbols if w == 2]
full_emoji = [(cp, 2, g[:32]) for cp, w, g in emojis]

cj = open(os.path.join(ROOT, "tools", "vterm", "unicode_glyph.h")).read()
cjk_sample = []
for ch in "你好世界终端模拟器中文测试渲染字体":
    m = re.search(r'\{ 0x%04X, \{ ([^}]+) \}' % ord(ch), cj)
    if m:
        cjk_sample.append((ord(ch), 2, [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', m.group(1))]))

SCALE = 2
def paint8(g, img, ox, oy):
    px = img.load()
    for y in range(16):
        for x in range(8):
            if (g[y] >> (7 - x)) & 1:
                for sy in range(SCALE):
                    for sx in range(SCALE):
                        px[ox + x * SCALE + sx, oy + y * SCALE + sy] = 255

def paint(g, img, ox, oy):
    px = img.load()
    if len(g) == 16:
        for y in range(16):
            for x in range(8):
                if (g[y] >> (7 - x)) & 1:
                    for sy in range(SCALE):
                        for sx in range(SCALE):
                            px[ox + x * SCALE + sx, oy + y * SCALE + sy] = 255
    else:
        for y in range(16):
            for x in range(16):
                byte = g[y * 2] if x < 8 else g[y * 2 + 1]
                if (byte >> (7 - (x & 7))) & 1:
                    for sy in range(SCALE):
                        for sx in range(SCALE):
                            px[ox + x * SCALE + sx, oy + y * SCALE + sy] = 255

# source glyphs: parse unifont.hex + NotoEmoji render (same as the generator)
UNIFONT = os.path.join(ROOT, "tools", "vterm", "fonts", "unifont.hex")
EMOJI_FONT = os.path.join(ROOT, "tools", "vterm", "fonts", "noto-emoji-mono.ttf")

def parse_unifont():
    g = {}
    for line in open(UNIFONT):
        line = line.strip()
        if not line or ":" not in line:
            continue
        cp_s, _, data = line.partition(":")
        g[int(cp_s, 16)] = bytes.fromhex(data.strip())
    return g

def shrink(g):
    if len(g) == 32:
        out = bytearray()
        for y in range(16):
            lo, hi = g[y*2], g[y*2+1]
            b = 0
            for x in range(8):
                c1 = (lo if 2*x < 8 else hi) >> (7 - (2*x % 8))
                c2 = (lo if 2*x+1 < 8 else hi) >> (7 - ((2*x+1) % 8))
                if (c1 & 1) or (c2 & 1):
                    b |= 1 << (7 - x)
            out.append(b)
        return bytes(out)
    return g

UNIFONT_GLYPHS = parse_unifont()

def symbol_source_glyph(cp, w):
    raw = UNIFONT_GLYPHS.get(cp)
    if raw is None:
        return None
    if len(raw) == 32:
        # unifont stores a 16x16 glyph: keep it full-width
        return (2, list(raw) + [0] * 0)
    g = shrink(raw)
    return (1, list(g) + [0] * 16)

def emoji_source_glyph(cp):
    return (2, _emoji_source_raw(cp))

def _emoji_source_raw(cp):
    # same rendering parameters as gen_emoji.py so SOURCE matches RENDERED
    font = ImageFont.truetype(EMOJI_FONT, 16)
    img = Image.new("L", (24, 20), 0)
    ImageDraw.Draw(img).text((0, 0), chr(cp), font=font, fill=255)
    bbox = img.getbbox()
    if not bbox:
        return None
    crop = img.crop(bbox)
    w, h = crop.size
    scale = min(15/w, 16/h, 1.0)
    if scale < 1.0:
        crop = crop.resize((max(1, int(w*scale)), max(1, int(h*scale))), Image.LANCZOS)
    canvas = Image.new("L", (16, 16), 0)
    canvas.paste(crop, ((16-crop.size[0])//2, (16-crop.size[1])//2))
    px = canvas.load()
    g = bytearray()
    for y in range(16):
        lo = hi = 0
        for x in range(16):
            if px[x, y] > 100:
                if x < 8: lo |= 1 << (7-x)
                else: hi |= 1 << (15-x)
        g.append(lo); g.append(hi)
    return list(g)

COLS8, COLS16 = 32, 16
W = max(COLS8 * 8 * SCALE, COLS16 * 16 * SCALE) + 300
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14)
y = 16
img = Image.new("L", (W, 20000), 0)
draw = ImageDraw.Draw(img)

def section(name, items, cols, cw, ch, source_fn=None):
    """Each glyph drawn twice: SOURCE above, RENDERED below."""
    global y
    y += 10
    draw.line((0, y, W, y), fill=100)
    y += 6
    draw.text((10, y), name + "   [upper=SOURCE  lower=RENDERED]", font=font, fill=255)
    y += 22
    for idx, (cp, w, g) in enumerate(items):
        r, c = divmod(idx, cols)
        src_g = source_fn(cp) if source_fn else None
        if src_g:
            sw, sglyph = src_g
            if sw >= 2:
                paint(sglyph, img, 10 + c * cw, y + r * ch * 2)
            else:
                # 8x16 source: draw 16px, left-aligned in the 32px column
                paint8(sglyph, img, 10 + c * cw, y + r * ch * 2)
        rw, rg = rendered_glyph(cp, g, w)
        if rw >= 2:
            paint(rg, img, 10 + c * cw, y + r * ch * 2 + ch)
        else:
            paint8(rg, img, 10 + c * cw, y + r * ch * 2 + ch)
    y += (len(items) + cols - 1) // cols * ch * 2

section(f"SYMBOLS 8x16 (natural width)  ({len(half)})", half, COLS8, 8 * SCALE, 16 * SCALE,
          lambda cp: symbol_source_glyph(cp, 1))
section(f"SYMBOLS 16x16 (natural width)  ({len(full)})", full, COLS16, 16 * SCALE, 16 * SCALE,
          lambda cp: symbol_source_glyph(cp, 2))
section(f"EMOJI 16x16  ({len(full_emoji)})", full_emoji, COLS16, 16 * SCALE, 16 * SCALE,
          emoji_source_glyph)
section(f"CJK sample 16x16  ({len(cjk_sample)})", cjk_sample, COLS16, 16 * SCALE, 16 * SCALE)

img = img.crop((0, 0, W, y + 16))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "docs", "char-table.png")
os.makedirs(os.path.dirname(out), exist_ok=True)
img.save(out)
print(f"saved {out} {img.size}")
