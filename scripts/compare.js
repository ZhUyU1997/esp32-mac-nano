#!/usr/bin/env node
/*
 * compare.js — ANSI art 对比工具（合并自 compare-one.js / compare-list.js）。
 *
 * 子命令:
 *   node scripts/compare.js one                渲染第一个 FAIL（/tmp/art-compare.log）
 *   node scripts/compare.js one pack entry     如 1993/acdu0193.zip TDT-CE1.ANS
 *   node scripts/compare.js one file.ans       松散文件
 *   node scripts/compare.js one --cut N ...    只取前 N 字节（二分定位）
 *     输出: <project-root>/compare.png（50% 三列：ansilove | vterm-ans | diff map）
 *
 *   node scripts/compare.js list [list.txt] [--limit N] [--concurrency N]
 *     按 list.txt（pack/entry 每行）批量跑 test-art.js --compare；
 *     坏 CRC / 缺条目跳过，结果日志留在临时目录。
 */
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');
const { spawnSync } = require('child_process');
const artlib = require('./art-lib');
const diff = require('./diff-lib');

const ROOT = path.join(__dirname, '..');
const PACKS = path.join(ROOT, 'scripts/art/packs');
const ANSILOVE = '/usr/bin/ansilove';
const VTERM = path.join(ROOT, 'build/linux/x86_64/release/vterm-ans');
const OUT = path.join(ROOT, 'compare.png');
const LOG = '/tmp/art-compare.log';
const SCALE = 0.5;

/* ---- one: single-file compare ----------------------------------------- */

/* pick the art bytes: explicit pack+entry, a loose file, or the first
 * FAIL from the last full run. `--cut N` keeps only the first N bytes
 * (prefix bisection: cut before a suspected sequence and compare).
 * Only the target entry is inflated (the whole-pack readZip is the slow path). */
function pickOne(argv) {
	let cut = 0;
	const ci = argv.indexOf('--cut');
	if (ci >= 0 && ci + 1 < argv.length) {
		cut = parseInt(argv[ci + 1], 10);
		argv.splice(ci, 2);
	}
	let buf, name0;
	if (argv.length >= 2) {
		const pack = path.join(PACKS, argv[0]);
		const entries = artlib.parseZip(fs.readFileSync(pack));
		const e = entries.find(x => x.name === argv[1] ||
		                           x.name.endsWith('/' + argv[1]));
		if (!e) throw new Error('entry not found: ' + argv[1] + ' in ' + argv[0]);
		buf = e.method === 8 ? zlib.inflateRawSync(e.data) : e.data;
		name0 = argv[0] + '/' + e.name;
	} else if (argv.length === 1) {
		buf = fs.readFileSync(argv[0]);
		name0 = path.basename(argv[0]);
	} else {
		/* fall back to the first FAIL in the log */
		buf = null;
		for (const line of fs.readFileSync(LOG, 'utf8').split('\n')) {
			/* unified name: "pack / entry" (no spaces inside either) */
			const m = line.match(/^FAIL (\S+) \/ (\S+): /);
			if (!m) continue;
			try {
				const pack = path.join(PACKS, m[1]);
				const entries = artlib.parseZip(fs.readFileSync(pack));
				const e = entries.find(x => x.name === m[2]);
				if (e) {
					buf = e.method === 8 ? zlib.inflateRawSync(e.data) : e.data;
					name0 = m[1] + '/' + e.name;
					break;
				}
			} catch (err) { /* broken pack: try next FAIL */ }
		}
		if (buf === null) throw new Error('no usable FAIL found in ' + LOG);
	}
	if (cut > 0) {
		buf = buf.slice(0, cut);
		name0 += ' [cut ' + cut + ']';
	}
	return { buf, name: name0 };
}

function run(cmd, args) {
	const r = spawnSync(cmd, args, { encoding: 'utf8' });
	if (r.error) throw r.error;
	return r;
}

function cmdOne(argv) {
	const work = fs.mkdtempSync(path.join(os.tmpdir(), 'compare-one-'));
	try {
		const art = pickOne(argv);
		const ans = path.join(work, 'a.png');
		const vtm = path.join(work, 'v.png');
		const artPath = path.join(work, 'art.ans');
		fs.writeFileSync(artPath, art.buf);

		/* vterm-ans honours SAUCE cols (40..200, else 80); ansilove misses
		 * narrow SAUCE widths, so force ansilove to the same width the
		 * vterm side will use — otherwise non-80 art compares shifted. */
		let cols = 80;
		const si = art.buf.indexOf('SAUCE00');
		if (si >= 0) {
			const c = art.buf[si + 7 + 89] | (art.buf[si + 7 + 90] << 8);
			if (c >= 40 && c <= 200) cols = c;
		}
		run(ANSILOVE, [artPath, '-o', ans, '-q', '-c', String(cols)]);
		const vr = run(VTERM, [artPath, '-o', vtm, '--cols', String(cols)]);
		if (vr.status !== 0)
			console.error('vterm-ans stderr: ' + (vr.stderr || '').trim());

		/* cell-diff via the shared node implementation (same numbers as
		 * test-art.js), then PIL only composites the three-column image */
		const cmp = diff.cellDiff(diff.decodePngRgb(fs.readFileSync(ans)),
		                         diff.decodePngRgb(fs.readFileSync(vtm)), true);
		const aSz = diff.decodePngRgb(fs.readFileSync(ans));
		const vSz = diff.decodePngRgb(fs.readFileSync(vtm));
		console.log('ansilove ' + aSz.w + 'x' + aSz.h + ' (' + (aSz.h / 16) +
		            ' rows) | vterm-ans ' + vSz.w + 'x' + vSz.h + ' (' + (vSz.h / 16) + ' rows)');
		console.log('cell diff: ' + (cmp.rate * 100).toFixed(1) + '% (' + cmp.diff +
		            '/' + cmp.total + ')' + (cmp.bright ? ' bright=' + cmp.bright : ''));
		const mapJson = path.join(work, 'map.json');
		fs.writeFileSync(mapJson, JSON.stringify(cmp.map ? [...cmp.map] : []));
		const py = `
import json, sys
from PIL import Image, ImageDraw
import numpy as np
ans = ${JSON.stringify(ans)}
vtm = ${JSON.stringify(vtm)}
out = ${JSON.stringify(OUT)}
map_file = ${JSON.stringify(mapJson)}
a = Image.open(ans).convert('RGB')
v = Image.open(vtm).convert('RGB')
rows_a, rows_v = a.size[1]//16, v.size[1]//16
cols = a.size[0]//8
common = max(rows_a, rows_v)  # show full height of both (min would hide overflow)
# diff map from the node-computed cell map (column mismatch: all red)
flat = np.array(json.load(open(map_file)), dtype=bool)
if flat.size == common * cols:
    cells = flat.reshape(common, cols)
else:
    cells = np.ones((common, cols), dtype=bool)
red = np.array([255,60,60], dtype=np.uint8)
dmarr = np.broadcast_to(np.where(cells[:,None,:,None,None], red[None,None,None,None,:], 0),
                        (common, 16, cols, 8, 3)).copy()
dm = Image.fromarray(dmarr.transpose(0,1,2,3,4).reshape(common*16, cols*8, 3))
s = ${SCALE}
ta = a.crop((0, 0, a.size[0], common*16)).resize((int(a.size[0]*s), int(common*16*s)), Image.NEAREST)
tv = v.crop((0, 0, v.size[0], common*16)).resize((int(v.size[0]*s), int(common*16*s)), Image.NEAREST)
tdm = dm.resize((int(a.size[0]*s), int(common*16*s)), Image.NEAREST)
bar = 30
h = ta.height + 30 + 16  # bottom padding so the last row mark fits
left_x = bar             # left content column
right_x = bar + ta.width + 40  # right content column
dm_x = right_x + ta.width + 40   # diff map column, right after the vterm-ans column
c = Image.new('RGB', (dm_x + tdm.width, h), (255,255,255))
d = ImageDraw.Draw(c)
def rowmarks(d, x, rows):
    for r in range(0, rows + 5, 5):
        # centre the 11px label on its 8px row (row top = 24 + r*8)
        d.text((x, 24 + int(r*16*s) - 1), str(r), fill=(120,120,120))
# row-number gutter sits left of each content column
rowmarks(d, left_x - bar + 2, common)
rowmarks(d, right_x - bar + 2, common)
d.text((left_x + 4, 4), 'ansilove %dx%d (%d rows)' % (a.size[0], a.size[1], rows_a), fill=(0,0,0))
d.text((right_x - bar + 4, 4), 'vterm-ans(ANSI.SYS) %dx%d (%d rows)' % (v.size[0], v.size[1], rows_v), fill=(0,0,0))
d.text((dm_x + 4, 4), 'diff map: %d/%d cells (%.1f%%)' % (${cmp.diff}, ${cmp.total}, ${(cmp.rate*100).toFixed(1)}), fill=(0,0,0))
c.paste(ta, (left_x, 24))
c.paste(tv, (right_x, 24))
c.paste(tdm, (dm_x, 24))
c.save(out)
`;
		const pr = run('python3', ['-c', py]);
		if (pr.status !== 0)
			console.error('python3: ' + (pr.stderr || '').trim());

		console.log('file:   ' + art.name + ' (' + art.buf.length + ' B)');
		console.log(pr.stdout ? pr.stdout.trim() : '');
		console.log('compare: ' + OUT);
	} finally {
		fs.rmSync(work, { recursive: true, force: true });
	}
}

/* ---- list: batch compare ---------------------------------------------- */

function cmdList(argv) {
	let listFile = path.join(ROOT, 'list.txt');
	let limit = 0;
	let concurrency = 8;
	for (let i = 0; i < argv.length; i++) {
		if (argv[i] === '--limit' && i + 1 < argv.length) limit = parseInt(argv[++i], 10);
		else if (argv[i] === '--concurrency' && i + 1 < argv.length) concurrency = parseInt(argv[++i], 10);
		else listFile = argv[i];
	}
	const lines = fs.readFileSync(listFile, 'utf8').split('\n').map(l => l.trim()).filter(Boolean);
	if (limit && lines.length > limit)
		lines.length = limit;
	console.log('compare list: ' + lines.length + ' entries from ' + listFile);

	const work = fs.mkdtempSync(path.join(os.tmpdir(), 'art-subset-'));
	/* extract with the shared JS zip code (art-lib): bzip2/LZMA/implode
	 * packs were already re-packed to store/deflate by unzip-packs.js, so
	 * Node's zlib covers every entry here — no python needed */
	let ok = 0, skip = 0;
	const packCache = new Map();
	for (const line of lines) {
		const idx = line.indexOf('.zip/');
		if (idx < 0) {
			skip++;
			continue;
		}
		const pack = line.slice(0, idx + 4);
		const entry = line.slice(idx + 5);
		try {
			let entries = packCache.get(pack);
			if (!entries) {
				entries = artlib.readZip(fs.readFileSync(path.join(PACKS, pack)));
				packCache.set(pack, entries);
			}
			const e = entries.find(x => x.name === entry);
			if (!e) {
				skip++;
				continue;
			}
			/* keep the pack/entry tree so test-art.js sees the "pack / entry"
			 * name (zip mode and loose mode then log one format) */
			const target = path.join(work, pack, entry);
			fs.mkdirSync(path.dirname(target), { recursive: true });
			fs.writeFileSync(target, e.buf);
			ok++;
		} catch (err) {
			skip++;
		}
	}
	console.log('extracted ' + ok + ', skipped ' + skip + ' (broken CRC / missing)');

	const log = path.join(work, 'result.log');
	const r = spawnSync('node', [
		path.join(ROOT, 'scripts/test-art.js'),
		'--render', '--compare',
		'--concurrency', String(concurrency),
		'--log', log, work,
	], { stdio: 'inherit' });
	if (r.error) {
		console.error('compare list: ' + r.error.message);
		process.exit(1);
	}
	console.log('log: ' + log);
}

/* ---- dispatch ---------------------------------------------------------- */

function usage() {
	console.error(
	    'usage: node scripts/compare.js <one|list> [...]\n' +
	    '  one [pack entry | file.ans] [--cut N]   single-file compare -> compare.png\n' +
	    '  list [list.txt] [--limit N] [--concurrency N]   batch via test-art.js');
	process.exit(1);
}

const cmd = process.argv[2];
try {
	if (cmd === 'one') cmdOne(process.argv.slice(3));
	else if (cmd === 'list') cmdList(process.argv.slice(3));
	else usage();
} catch (err) {
	console.error('compare ' + (cmd || '?') + ': ' + err.message);
	process.exit(1);
}
