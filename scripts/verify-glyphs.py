#!/usr/bin/env python3
"""Verify glyph tables against their sources (source consistency).

symbol_glyphs.h  vs GNU Unifont (unifont.hex)
emoji_glyphs.h   vs Noto Emoji monochrome (rendered at 32px, downscaled)

Usage: python3 scripts/verify-glyphs.py [unifont.hex] [noto-emoji.ttf]
"""
import re, sys, os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), "..")
UNIFONT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "tools", "vterm", "fonts", "unifont.hex")
EMOJI_FONT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "tools", "vterm", "fonts", "noto-emoji-mono.ttf")

def parse_unifont(path):
    g = {}
    for line in open(path):
        line = line.strip()
        if not line or ":" not in line:
            continue
        cp_s, _, data = line.partition(":")
        g[int(cp_s, 16)] = bytes.fromhex(data.strip())
    return g


def render_emoji(ch, path):
    font = ImageFont.truetype(path, 16)
    img = Image.new("L", (24, 20), 0)
    ImageDraw.Draw(img).text((0, 0), ch, font=font, fill=255)
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
    return bytes(g)

bad = total = 0

# --- symbols vs unifont ---
src = parse_unifont(UNIFONT)
data = open(os.path.join(ROOT, "tools", "vterm", "symbol_glyphs.h")).read()
for m in re.finditer(r"\{ 0x([0-9A-Fa-f]{4}), (\d), \{ ([^}]+) \}", data):
    cp, w = int(m.group(1), 16), int(m.group(2))
    g = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(3)))
    raw = src.get(cp)
    total += 1
    if raw is None:
        print(f"FAIL symbol U+{cp:04X}: not in unifont"); bad += 1; continue
    if len(raw) == 32:
        exp = raw + b"\0" * 16
        if g != exp[:32]:
            print(f"FAIL symbol U+{cp:04X}: 16x16 vs source"); bad += 1
    else:
        exp = raw + b"\0" * 16
        if g != exp:
            print(f"FAIL symbol U+{cp:04X}: 8x16 vs source"); bad += 1

# --- emoji vs Noto Emoji render ---
data = open(os.path.join(ROOT, "tools", "vterm", "emoji_glyphs.h")).read()
for m in re.finditer(r"\{ 0x([0-9A-Fa-f]{4,8}), \{ ([^}]+) \}", data):
    cp = int(m.group(1), 16)
    g = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(2)))
    total += 1
    try:
        exp = render_emoji(chr(cp), EMOJI_FONT)
    except Exception:
        exp = None
    if exp is None:
        print(f"FAIL emoji U+{cp:04X}: cannot render"); bad += 1
    elif g[:32] != exp:
        print(f"FAIL emoji U+{cp:04X}: vs font render"); bad += 1

print(f"{total} entries checked, {bad} mismatches")
sys.exit(1 if bad else 0)
