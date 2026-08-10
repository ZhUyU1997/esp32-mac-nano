#!/usr/bin/env node
/* Regenerate mono_opposans_<size>.c from OPPOSans-R.ttf with lv_font_conv.
 * Non-ASCII characters are auto-extracted from ui_strings.json (source of
 * truth), so any new UI text gets glyphs without manual work.
 * Usage: node scripts/gen-mono-opposans-font.js [size]   (default 18)
 * Requires: lv_font_conv in PATH (npm i -g lv_font_conv) */
'use strict';

const { spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const size = process.argv[2] || '18';
/* Small sizes look grainy at bpp=1 (isolated dots on OPPOSans Latin
 * glyphs, hinting has no effect); use bpp=2 AA for the 14px auxiliary font. */
const bpp = size === '14' ? '2' : '1';
const root = path.resolve(__dirname, '..');
const fontDir = path.join(root, 'main/arch/esp32/mach-s3/ui/font');
/* Source TTF stays INSIDE the repo at fonts/ but is git-ignored (it
 * contains personal metadata, never commit it). Override with OPPOSANS_TTF. */
const ttfPath = process.env.OPPOSANS_TTF || path.join(root, 'fonts/OPPOSans-R.ttf');
const jsonPath = path.join(root, 'main/arch/esp32/mach-s3/ui/ui_strings.json');

if (!fs.existsSync(ttfPath)) {
    console.error(`error: font source not found: ${ttfPath}`);
    console.error('copy OPPOSans-R.ttf to ~/fonts/ or set OPPOSANS_TTF');
    process.exit(1);
}

const data = JSON.parse(fs.readFileSync(jsonPath, 'utf8'));
const chars = new Set();
for (const value of Object.values(data)) {
    for (const ch of value) {
        if (ch.codePointAt(0) > 127) chars.add(ch);
    }
}
const symbols = [...chars].sort().join('');
if (!symbols) {
    console.error(`error: no non-ASCII characters found in ${jsonPath}`);
    process.exit(1);
}

const outFile = `mono_opposans_${size}.c`;
const outRel = `main/arch/esp32/mach-s3/ui/font/${outFile}`;
const r = spawnSync('lv_font_conv', [
    '--bpp', bpp,
    '--size', size,
    '--no-compress',
    '--font', 'fonts/OPPOSans-R.ttf',
    '-r', '32-127',
    '--symbols', symbols,
    '--format', 'lvgl',
    '-o', outRel,
], { cwd: root, stdio: 'inherit' });

if (r.error) {
    console.error('error: lv_font_conv not found in PATH; install with: npm i -g lv_font_conv');
    process.exit(1);
}
if (r.status !== 0) {
    process.exit(r.status);
}
console.log(`generated: ${path.join(fontDir, outFile)} (${symbols.length} unique non-ASCII chars)`);
