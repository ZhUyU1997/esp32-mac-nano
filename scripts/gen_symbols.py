#!/usr/bin/env python3
"""Extract a TUI symbol set from GNU Unifont into k_symbol_glyphs.h."""
import sys
import os

ROOT = os.path.join(os.path.dirname(__file__), '..')

HEX = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'tools', 'vterm', 'fonts', 'unifont.hex')

# full ranges (every defined glyph)
full_ranges = [
    (0x2190, 0x21FF),  # arrows
    (0x2500, 0x257F),  # box drawing (incl. rounded corners)
    (0x2580, 0x259F),  # block elements
    (0x25A0, 0x25FF),  # geometric shapes
]
# curated whitelists (common in TUI/CLI output)
misc = "☀☁☂☃★☆☎☐☑☒☓☕☝☠☢☣☮☯☸☹☺☻☼☽☾♀♂♠♡♢♣♤♥♦♧♨♪♫♬♭♮♯☿♃♄♅♆♇♈♉♊♋♌♍♎♏♐♑♒♓♔♕♖♗♘♙♚♛♜♝♞♟♰♱♲♳♴♵♶♷♸♹♺♻♼♽♾♿⚀⚁⚂⚃⚄⚅⚐⚑⚒⚓⚔⚕⚖⚗⚘⚙⚚⚛⚜⚠⚡⚢⚣⚤⚥⚦⚧⚨⚩⚪⚫⚬⚭⚮⚯⚰⚱⚲⚳⚴⚵⚶⚷⚸⚹⚺⚻⚼⚽⚾⚿"
dingbats = "✀✁✂✃✄✅✆✇✈✉✊✋✌✍✎✏✐✑✒✓✔✕✖✗✘✙✚✛✜✝✞✟✠✡✢✣✤✥✦✧✨✩✪✫✬✭✮✯✰✱✲✳✴✵✶✷✸✹✺✻✼✽✾✿❀❁❂❃❄❅❆❇❈❉❊❋❌❍❎❏❐❑❒❓❔❕❖❗❘❙❚❛❜❝❞❟❠❡❢❣❤❥❦❧❨❩❪❫❬❭❮❯❰❱❲❳❴❵❶❷❸❹❺❻❼❽❾❿➀➁➂➃➄➅➆➇➈➉➊➋➌➍➎➏➐➑➒➓➔➕➖➗➘➙➚➛➜➝➞➟➠➡➢➣➤➥➦➧➨➩➪➫➬➭➮➯➰➱➲➳➴➵➶➷➸➹➺➻➼➽➾➿"
math = "∀∁∂∃∄∅∆∇∈∉∊∋∌∍∎∏∐∑−∓∔∕∖∗∘∙√∛∜∝∞∟∠∡∢∣∤∥∦∧∨∩∪∫∬∭∮∴∵∶∷∼∽∾∿≀≁≂≃≄≅≆≇≈≉≊≋≌≍≎≏≐≑≒≓≔≕≖≗≘≙≚≛≜≝≞≟≠≡≢≣≤≥≦≧≨≩≪≫≬≭≮≯≰≱≲≳≴≵≶≷≸≹≺≻≼≽≾≿"
tech = "⌂⌃⌄⌅⌆⌇⌈⌉⌊⌋⌌⌍⌎⌏⌐⌑⌒⌓⌔⌕⌖⌗⌘⌙⌚⌛⌜⌝⌞⌟⌠⌡⌢⌣⌤⌥⌦⌧⌨〈〉⌫⌬⌭⌮⌯⌰⌱⌲⌳⌴⌵⌶⌷⌸⌹⌺⌻⌼⌽⌾⌿⍀⍁⍂⍃⍄⍅⍆⍇⍈⍉⍊⍋⍌⍍⍎⍏⍐⍑⍒⍓⍔⍕⍖⍗⍘⍙⍚⍛⍜⍝⍞⍟⍠⍡⍢⍣⍤⍥⍦⍧⍨⍩⍪⍫⍬⍭⍮⍯⍰⍱⍲⍳⍴⍵⍶⍷⍸⍹⍺⍻⍼⍽⍾⍿⎀⎁⎂⎃⎄⎅⎆⎇⎈⎉⎊⎋⎌⎍⎎⎏⎐⎑⎒⎓⎔⎕⎖⎗⎘⎙⎚⎛⎜⎝⎞⎟⎠⎡⎢⎣⎤⎥⎦⎧⎨⎩⎪⎫⎬⎭⎮⎯⎰⎱⎲⎳⎴⎵⎶⎷⎸⎹⎺⎻⎼⎽⎾⎿⏀⏁⏂⏃⏄⏅⏆⏇⏈⏉⏊⏋⏌⏍⏎⏏⏐⏑⏒⏓⏔⏕⏖⏗⏘⏙⏚⏛⏜⏝⏞⏟⏠⏡⏢⏣⏤⏥⏦⏧⏨⏩⏪⏫⏬⏭⏮⏯⏰⏱⏲⏳⏴⏵⏶⏷⏸⏹⏺⏻⏼⏽⏾⏿"
punct = "–—―‘’‚“”„†‡•…‰′″‹›‼‽⁇⁈⁉⁺⁻⁼⁽⁾ⁿ"
# Latin-1 symbols not present in CP437 (©®™€¤¦ªº etc.)
latin1 = "©®™€£¥¢¤¦§ªº¬¯´¸¶¨¹³µ¿×÷"

want = set()
for lo, hi in full_ranges:
    want.update(range(lo, hi + 1))
for s in (misc, dingbats, math, tech, punct, latin1):
    want.update(ord(c) for c in s)

# parse unifont.hex once
if not os.path.exists(HEX):
    sys.exit(f"ERROR: unifont.hex not found at {HEX}\n"
             "Place it in tools/vterm/fonts/ (see tools/vterm/README.md)")
glyphs = {}
for line in open(HEX):
    line = line.strip()
    if not line or ':' not in line:
        continue
    cp_s, _, data = line.partition(':')
    cp = int(cp_s, 16)
    if cp in want:
        glyphs[cp] = bytes.fromhex(data.strip())

# keep only entries that have a real glyph
def prep(cp, g):
    # glyphs as-is from the source: fixed 16px height, natural width
    # (8x16 sources stay 8px wide, 16x16 stay 16px) — no scaling, the
    # aspect ratio must not change
    return g
items = sorted((cp, prep(cp, g)) for cp, g in glyphs.items()
               if len(g) in (16, 32) and any(g))
print(f"extracted {len(items)} symbols ({len(items)*16//1024} KB)")

with open('/home/yzhu/esp32-mini-mac/tools/vterm/symbol_glyphs.h', 'w') as f:
    f.write("/* Auto-generated from GNU Unifont (GPL-2.0 WITH FONT EXCEPTION).\n")
    f.write(" * TUI symbols: box/block/arrow/geometric/dingbats/math/technical.\n")
    f.write(" * 8x16 and 16x16 glyphs as-is from the source, MSB = leftmost.\n")
    f.write(" * Sorted by code point. */\n")
    f.write("#ifndef SYMBOL_GLYPHS_H\n#define SYMBOL_GLYPHS_H\n\n")
    f.write("typedef struct { uint16_t uni; uint8_t w; uint8_t glyph[32]; } symbol_glyph_t;\n")
    f.write("static const symbol_glyph_t k_symbol_glyphs[] = {\n")
    for cp, g in items:
        row = ", ".join(f"0x{b:02X}" for b in g)
        w = 2 if len(g) == 32 else 1  # 16x16 source -> 2 cells, 8x16 -> 1
        glyph = g if len(g) == 32 else g + bytes(16)
        row = ", ".join(f"0x{b:02X}" for b in glyph)
        f.write(f"    {{ 0x{cp:04X}, {w}, {{ {row} }} }},\n")
    f.write("};\n")
    f.write(f"#define SYMBOL_GLYPH_COUNT {len(items)}\n\n#endif\n")
print("wrote symbol_glyphs.h")
