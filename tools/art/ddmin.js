#!/usr/bin/env node
/* ddmin.js — automatic delta-debugging minimizer for render-diff cases.
 * Given a pack entry, it renders vterm-ans vs ansilove, finds the diff
 * cells, maps them to source byte offsets (vterm-ans --trace-cells) and
 * iteratively drops bytes outside an ever-shrinking window until the
 * diff disappears, then backs off to the smallest window that keeps it.
 *
 * Usage:
 *   node tools/art/ddmin.js <pack.zip> <entry> [--window N] [--max-rounds N]
 *
 * Output: the minimal source slice (bytes + readable sequence) that still
 * reproduces the diff, saved to art-diff/ddmin-<entry>.ans and printed.
 */
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');
const { spawnSync } = require('child_process');
const artlib = require('./lib/art-lib');
const diff = require('./lib/diff-lib');
const { ROOT, PACKS, VTERM, ANSILOVE } = require('./lib/config');
const { renderBoth, traceCells } = require('./lib/render');

const argv = process.argv.slice(2);
if (argv.length < 2) {
	console.error('usage: ddmin.js <pack.zip> <entry> [--window N] [--max-rounds N]');
	process.exit(1);
}
const pack = argv[0];
const entry = argv[1];
let WINDOW = 120;
let MAX_ROUNDS = 40;
for (let i = 2; i < argv.length; i++) {
	if (argv[i] === '--window' && i + 1 < argv.length) WINDOW = parseInt(argv[++i], 10);
	if (argv[i] === '--max-rounds' && i + 1 < argv.length) MAX_ROUNDS = parseInt(argv[++i], 10);
}

const packBuf = fs.readFileSync(path.join(PACKS, pack));
const e = artlib.parseZip(packBuf).find(x => x.name === entry ||
                                             x.name.endsWith('/' + entry));
if (!e) { console.error('entry not found'); process.exit(1); }
const buf = e.method === 8 ? zlib.inflateRawSync(e.data) : e.data;

const work = fs.mkdtempSync(path.join(os.tmpdir(), 'ddmin-'));
function render(slice) {
	const aPath = work + '/a.ans', ap = work + '/a.png', vp = work + '/v.png';
	fs.writeFileSync(aPath, slice);
	spawnSync(ANSILOVE, [aPath, '-o', ap, '-q', '-c', '80']);
	spawnSync(VTERM, [aPath, '-o', vp, '--cols', '80']);
	const ai = diff.decodePngRgb(fs.readFileSync(ap));
	const vi = diff.decodePngRgb(fs.readFileSync(vp));
	return diff.cellDiff(vi, ai, true);
}
function diffCells(cmp) {
	const c = 80, out = [];
	for (let i = 0; i < cmp.map.length; i++)
		if (cmp.map[i]) out.push([Math.floor(i / c), i % c]);
	return out;
}

/* 1. full render: find the diff cells */
const full = render(buf);
if (full.diff === 0) { console.log('no diff in full file'); process.exit(0); }
console.log('full diff: ' + full.diff + '/' + full.total + ' (' +
            (full.rate * 100).toFixed(1) + '%)');
const cells = diffCells(full);
console.log('diff cells: ' + cells.map(([r, c]) => 'r' + r + 'c' + c).join(' '));

/* 2. map cells to byte offsets via --trace-cells */
const tPath = traceCells(work + '/f.ans', work + '/t.txt', 80).status !== 0 ? '' : work + '/t.txt';
fs.writeFileSync(work + '/f.ans', buf);
const map = new Map();
if (tPath) for (const line of fs.readFileSync(tPath, 'utf8').split('\n')) {
	const m = line.match(/^(\d+),(\d+),(\d+)$/);
	if (m) map.set(m[1] + ',' + m[2], parseInt(m[3], 10));
}
const offsets = cells.map(([r, c]) => map.get(r + ',' + c)).filter(x => x !== undefined);
console.log('byte offsets: ' + offsets.join(', '));
if (!offsets.length) { console.error('no cell->byte mapping'); process.exit(1); }

/* 3. split the file into units (complete ESC sequences / chars): the
 * window slicing of raw bytes can cut a sequence in half, which changes
 * the render; ddmin must only ever drop whole units. */
const units = []; /* [start,end) per unit */
for (let i = 0; i < buf.length; ) {
	if (buf[i] === 27) {
		let j = i + 1;
		if (buf[j] === 91) {
			j++;
			while (j < buf.length && !(buf[j] >= 0x40 && buf[j] < 0x60)) j++;
			j++;
		} else j++;
		units.push([i, j]);
		i = j;
	} else {
		units.push([i, i + 1]);
		i++;
	}
}

/* 4. ddmin over whole units (never cut a sequence): coarse passes drop
 * big groups first, then fine passes drop single units. Units are kept
 * whole so the result is always a valid sequence stream. */
let kept = units.map((u, i) => i);
let rounds = 0;
let changed = true;
let group = 256;
while (changed && rounds < MAX_ROUNDS) {
	changed = false;
	rounds++;
	/* coarse: try dropping groups of `group` units */
	for (let i = 0; i < kept.length && rounds < MAX_ROUNDS; i += group) {
		const n = Math.min(group, kept.length - i);
		const test = Buffer.concat(kept
			.filter((_, j) => j < i || j >= i + n)
			.map(k => buf.slice(units[k][0], units[k][1])));
		if (test.length === 0) continue;
		const r = render(test);
		if (r.diff > 0) {
			kept.splice(i, n);
			changed = true;
			console.log('round ' + rounds + ': dropped group ' + n +
			            ' (' + kept.length + ' units left), diff=' +
			            r.diff + '/' + r.total);
			i -= group;
		}
	}
	group = Math.max(1, group >> 1);
}

/* 5. output */
const final = Buffer.concat(kept.map(k => buf.slice(units[k][0], units[k][1])));
let readable = '';
for (const c of final) {
	readable += c === 27 ? '<ESC>' :
	           c >= 32 && c < 127 ? String.fromCharCode(c) :
	           '[' + c.toString(16).padStart(2, '0') + ']';
}
const outName = path.join(ROOT, 'art-diff', 'ddmin-' + entry.replace(/[^a-z0-9.]/gi, '_'));
fs.writeFileSync(outName, final);
console.log('\nminimal slice: ' + final.length + 'B -> ' + outName);
console.log('sequence: ' + readable);
fs.rmSync(work, { recursive: true, force: true });
