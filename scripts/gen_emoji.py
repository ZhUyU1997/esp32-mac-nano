#!/usr/bin/env python3
"""Extract common TUI emoji from Noto Emoji monochrome (OFL) as 16x16 glyphs."""
import os
ROOT = os.path.join(os.path.dirname(__file__), "..")
from PIL import Image, ImageDraw, ImageFont

FONT = os.path.join(ROOT, 'tools', 'vterm', 'fonts', 'noto-emoji-mono.ttf')  # Noto Emoji monochrome (OFL)

# common emoji in TUI/CLI output
emoji = (
    # UI / status
    "🗑📁📂📄📝📌📍📎🔗🔒🔓🔍🔎✅❌⚠ℹ❗❓❕❔"
    "💾💿📀📥📤🔽🔼⏬⏫📢🔔♻🔄🔃🔌"
    # actions / people
    "👍👎👌✋👏👀💡🎯🚀🔥💯⭐🌟✨❤💔💚💙🎉🎊"
    # time / misc
    "⏰⏳📅📆🌍🌐🌙☀🌈⚡💧🍀🎵🎶📦🔧🔨🔩💻🖥⌨🖱📱"
)
# dedupe keep order
seen = []
for ch in emoji:
    if ch not in seen:
        seen.append(ch)
print(f"{len(seen)} emoji")

if not os.path.exists(FONT):
    sys.exit(f"ERROR: noto-emoji-mono.ttf not found at {FONT}\n"
             "Place it in tools/vterm/fonts/ (see tools/vterm/README.md)")
font = ImageFont.truetype(FONT, 16)  # render 8x16 directly
out = {}
for ch in seen:
    img = Image.new('L', (24, 20), 0)
    d = ImageDraw.Draw(img)
    d.text((0, 0), ch, font=font, fill=255)
    bbox = img.getbbox()
    if not bbox:
        print(f"  {ch} U+{ord(ch):04X}: no glyph")
        continue
    # crop to content, scale to fit 16x16 (full-width emoji, 2 cells)
    crop = img.crop(bbox)
    w, h = crop.size
    scale = min(15 / w, 16 / h, 1.0)  # 15px: tiny side margin
    if scale < 1.0:
        nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
        crop = crop.resize((nw, nh), Image.LANCZOS)
    # center in 16x16
    canvas = Image.new('L', (16, 16), 0)
    x0 = (16 - crop.size[0]) // 2
    y0 = (16 - crop.size[1]) // 2
    canvas.paste(crop, (x0, y0))
    px = canvas.load()
    glyph = bytearray()
    for y in range(16):
        lo = hi = 0
        for x in range(16):
            if px[x, y] > 100:
                if x < 8: lo |= 1 << (7 - x)
                else: hi |= 1 << (15 - x)
        glyph.append(lo); glyph.append(hi)
    out[ord(ch)] = bytes(glyph)

print(f"extracted {len(out)} ({len(out)*32//1024} KB)")
with open('/home/yzhu/esp32-mini-mac/tools/vterm/emoji_glyphs.h', 'w') as f:
    f.write("/* Auto-generated from Noto Emoji monochrome (OFL).\n")
    f.write(" * Common TUI emoji as 16x16 glyphs (32 B each), MSB = left.\n")
    f.write(" * Layout width comes from libvterm (xterm standard); renders 16px. */\n")
    f.write("#ifndef EMOJI_GLYPHS_H\n#define EMOJI_GLYPHS_H\n\n")
    f.write("typedef struct { uint32_t uni; uint8_t glyph[32]; } emoji_glyph_t;\n\n")
    f.write("static const emoji_glyph_t k_emoji_glyphs[] = {\n")
    for cp, g in sorted(out.items()):
        row = ", ".join(f"0x{b:02X}" for b in g)
        f.write(f"    {{ 0x{cp:04X}, {{ {row} }} }},\n")
    f.write("};\n")
    f.write(f"#define EMOJI_GLYPH_COUNT {len(out)}\n\n#endif\n")
print("wrote emoji_glyphs.h")
