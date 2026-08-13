#!/usr/bin/env node
/*
 * count-combining.js — how many Unicode code points can stack in one cell
 * given OUR glyph support?
 *
 * A terminal cell holds 1 base char + N combining marks (zero-width).
 * libvterm reserves 6 slots (1 base + 5 combining). But we only render a
 * fixed glyph set (CP437 + symbol table + emoji + CJK), so if none of our
 * glyphs is a combining mark, every cell we can draw needs exactly 1 slot.
 *
 * Authority for "combining" = libvterm's own table (unicode.c `combining[]`,
 * the Kuhn Mk table) — the same table its parser uses to group marks.
 *
 * Usage: node scripts/count-combining.js [project-root]
 */
const fs = require('fs');
const path = require('path');

const root = process.argv[2] || '.';

/* ---- 1. extract code points from the glyph tables --------------------- */

// every glyph entry is "{ 0xNNNN, ..." — first field is the code point
function glyphsFromHeader(file) {
  const text = fs.readFileSync(file, 'utf8');
  const out = new Set();
  const re = /\{\s*(0x[0-9a-fA-F]+)/g;
  let m;
  while ((m = re.exec(text)) !== null) {
    out.add(parseInt(m[1], 16));
  }
  return out;
}

const cps = new Set();
const tables = [
  ['symbol_glyphs.h', 'main/arch/esp32/mach-s3/vterm/symbol_glyphs.h'],
  ['unicode_glyph.h', 'main/arch/esp32/mach-s3/vterm/unicode_glyph.h'],
  ['emoji_glyphs.h', 'main/arch/esp32/mach-s3/vterm/emoji_glyphs.h'],
];

for (const [name, f] of tables) {
  const p = path.join(root, f);
  if (!fs.existsSync(p)) {
    console.error('missing ' + f);
    process.exit(1);
  }
  const s = glyphsFromHeader(p);
  for (const cp of s) cps.add(cp);
  console.error(`${name}: ${s.size} code points`);
}
// CP437: vga8x16.h is 256 glyphs indexed 0..255, all base (no combining)
console.error('vga8x16.h (CP437): 256 code points (base only)');

/* ---- 2. extract libvterm's combining table ---------------------------- */

function combiningRanges() {
  const text = fs.readFileSync(path.join(root, 'libvterm/src/unicode.c'), 'utf8');
  const block = text.match(/static const struct interval combining\[\] = \{(.*?)\};/s);
  if (!block) throw new Error('combining[] not found in unicode.c');
  const re = /\{\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\}/g;
  const ranges = [];
  let m;
  while ((m = re.exec(block[1])) !== null)
    ranges.push([parseInt(m[1], 16), parseInt(m[2], 16)]);
  return ranges;
}

const ranges = combiningRanges();
const isCombining = (cp) => ranges.some(([a, b]) => cp >= a && cp <= b);

/* ---- 3. classify our glyphs ------------------------------------------- */

const combining = [];
for (const cp of cps) {
  if (isCombining(cp)) combining.push(cp);
}

console.error(`\ncombining ranges in libvterm: ${ranges.length}`);
console.error(`our total unique code points: ${cps.size}`);
console.error(`our combining code points: ${combining.length}`);
if (combining.length > 0)
  console.error('  ' + combining.map((cp) => 'U+' + cp.toString(16).toUpperCase().padStart(4, '0')).join(' '));

/* ---- 4. conclusion ----------------------------------------------------- */

console.log('\n=== result ===');
console.log('max code points per cell we can render: ' + (1 + (combining.length > 0 ? 1 : 0)));
if (combining.length === 0) {
  console.log('=> ScreenCell.chars[6] (24 B) could shrink to chars[1] (4 B):');
  console.log('   ScreenCell 36 B -> 16 B; screen buffer 84 KB -> 38 KB');
  console.log('   (needs a libvterm change: drop/store at most 1 code point per cell)');
}
