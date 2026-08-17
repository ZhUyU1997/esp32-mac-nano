/* render.js — vterm-ans / libansilove process wrappers shared by
 * compare.js, ddmin.js and cell2byte.js. */
'use strict';

const path = require('path');
const { spawnSync } = require('child_process');
const { ANSILOVE, VTERM } = require('./config');

/* SAUCE column width (40..200), else 80. vterm-ans honours SAUCE cols;
 * ansilove misses narrow widths, so callers force it to the same width. */
function sauceCols(buf) {
	const si = buf.indexOf('SAUCE00');
	if (si < 0) return 80;
	const c = buf[si + 7 + 89] | (buf[si + 7 + 90] << 8);
	return (c >= 40 && c <= 200) ? c : 80;
}

/* Render one ANSI file with both renderers into workDir, returning the
 * PNG paths plus the spawn results. cols overrides the grid width. */
function renderBoth(ansPath, workDir, cols = 80, bin = VTERM) {
	const ans = path.join(workDir, 'a.png');
	const vtm = path.join(workDir, 'v.png');
	const ansilove = spawnSync(ANSILOVE, [ansPath, '-o', ans, '-q', '-c', String(cols)], { encoding: 'utf8' });
	const vterm = spawnSync(bin, [ansPath, '-o', vtm, '--cols', String(cols)], { encoding: 'utf8' });
	return { ans, vtm, ansilove, vterm };
}

/* vterm-ans --trace-cells: row,col,byte_offset per visible char. */
function traceCells(ansPath, tracePath, cols = 80, bin = VTERM) {
	return spawnSync(bin, [ansPath, '--trace-cells', tracePath, '--cols', String(cols)], { encoding: 'utf8' });
}

module.exports = { sauceCols, renderBoth, traceCells };
