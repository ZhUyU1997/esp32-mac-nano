#!/usr/bin/env node
/* cell2byte.js — map a rendered cell (row,col) back to its source byte
 * offset, using vterm-ans --trace-cells (the real libvterm state machine,
 * so the mapping is exactly what vterm-ans renders).
 *
 * Usage:
 *   node scripts/cell2byte.js <pack.zip> <entry> <row> <col> [row col ...]
 *   node scripts/cell2byte.js <file.ans> <row> <col>
 *
 * Prints: r<row>c<col> -> byte <off> + the source context around it.
 */
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');
const { spawnSync } = require('child_process');
const artlib = require('./art-lib');

const ROOT = path.join(__dirname, '..');
const VTERM = path.join(ROOT, 'build/linux/x86_64/release/vterm-ans');

const argv = process.argv.slice(2);
if (argv.length < 3) {
	console.error('usage: cell2byte.js <pack.zip|file.ans> <entry?> <row> <col> ...');
	process.exit(1);
}

let buf, entry;
let rest;
if (argv[0].endsWith('.zip')) {
	const pack = path.join(ROOT, 'scripts/art/packs', argv[0]);
	const entries = artlib.parseZip(fs.readFileSync(pack));
	const e = entries.find(x => x.name === argv[1] ||
	                           x.name.endsWith('/' + argv[1]));
	if (!e) { console.error('entry not found: ' + argv[1]); process.exit(1); }
	buf = e.method === 8 ? zlib.inflateRawSync(e.data) : e.data;
	entry = argv[0] + '/' + e.name;
	rest = argv.slice(2);
} else {
	buf = fs.readFileSync(argv[0]);
	entry = path.basename(argv[0]);
	rest = argv.slice(1);
}
const targets = [];
for (let i = 0; i + 1 < rest.length; i += 2)
	targets.push([parseInt(rest[i], 10), parseInt(rest[i + 1], 10)]);

const work = fs.mkdtempSync(path.join(os.tmpdir(), 'cell2byte-'));
const ansPath = path.join(work, 'a.ans');
const tracePath = path.join(work, 'trace.txt');
fs.writeFileSync(ansPath, buf);
const r = spawnSync(VTERM, [ansPath, '--trace-cells', tracePath],
                    { encoding: 'utf8' });
if (r.status !== 0) {
	console.error('vterm-ans: ' + (r.stderr || '').trim());
	process.exit(1);
}

/* trace lines: row,col,offset */
const map = new Map();
for (const line of fs.readFileSync(tracePath, 'utf8').split('\n')) {
	const m = line.match(/^(\d+),(\d+),(\d+)$/);
	if (m) map.set(m[1] + ',' + m[2], parseInt(m[3], 10));
}

for (const [row, col] of targets) {
	const off = map.get(row + ',' + col);
	if (off === undefined) {
		console.log(`r${row}c${col}: not a written cell (blank / moved over)`);
		continue;
	}
	console.log(`r${row}c${col} -> byte ${off} of ${buf.length} (${entry})`);
	/* context: collapse the raw bytes, keep ESC sequences readable */
	let ctx = '';
	for (let k = Math.max(0, off - 90); k < Math.min(buf.length, off + 8); k++) {
		const c = buf[k];
		ctx += c === 27 ? '<ESC>' :
		       c >= 32 && c < 127 ? String.fromCharCode(c) :
		       '[' + c.toString(16).padStart(2, '0') + ']';
	}
	console.log('  context: ' + ctx);
	/* the SGR/t sequence immediately before this char */
	const s = Math.max(0, off - 40);
	let tail = '';
	for (let k = s; k < off; k++) {
		const c = buf[k];
		tail += c === 27 ? '<ESC>' :
		        c >= 32 && c < 127 ? String.fromCharCode(c) :
		        '[' + c.toString(16).padStart(2, '0') + ']';
	}
	console.log('  pre-40B: ' + tail);
}

fs.rmSync(work, { recursive: true, force: true });
