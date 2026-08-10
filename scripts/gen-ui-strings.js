#!/usr/bin/env node
/* Generate include/ui_strings.h from ui/ui_strings.json (source of truth).
 * Usage: node scripts/gen-ui-strings.js */
'use strict';

const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const jsonPath = path.join(root, 'main/arch/esp32/mach-s3/ui/ui_strings.json');
const outPath = path.join(root, 'main/arch/esp32/mach-s3/include/ui_strings.h');

const data = JSON.parse(fs.readFileSync(jsonPath, 'utf8'));
const lines = [
    '#ifndef MACH_S3_UI_STRINGS_H',
    '#define MACH_S3_UI_STRINGS_H',
    '',
    '/* GENERATED from ui_strings.json by scripts/gen-ui-strings.js - do not edit. */',
    '',
];
for (const [key, value] of Object.entries(data)) {
    lines.push(`#define ${key} ${JSON.stringify(value)}`);
}
lines.push('', '#endif /* MACH_S3_UI_STRINGS_H */', '');

fs.writeFileSync(outPath, lines.join('\n'), 'utf8');
console.log(`generated: ${outPath} (${Object.keys(data).length} strings)`);
