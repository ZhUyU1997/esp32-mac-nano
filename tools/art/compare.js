#!/usr/bin/env node
/*
 * compare.js — ANSI art 对比工具（合并自 compare-one.js / compare-list.js）。
 *
 * 子命令:
 *   node tools/art/compare.js one                渲染第一个 FAIL（/tmp/art-compare.log）
 *   node tools/art/compare.js one pack entry     如 1993/acdu0193.zip TDT-CE1.ANS
 *   node tools/art/compare.js one file.ans       松散文件
 *   node tools/art/compare.js one --cut N ...    只取前 N 字节（二分定位）
 *     输出: <project-root>/compare.png（50% 三列：ansilove | vterm-ans | diff map）
 *
 *   node tools/art/compare.js list [list.txt] [--limit N] [--concurrency N]
 *     按 list.txt（pack/entry 每行）批量跑 test-art.js --compare；
 *     坏 CRC / 缺条目跳过，结果日志留在临时目录。
 */
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');
const { spawnSync } = require('child_process');
const artlib = require('./lib/art-lib');
const diff = require('./lib/diff-lib');
const { ROOT, PACKS, ANSILOVE, VTERM } = require('./lib/config');
const { sauceCols, renderBoth } = require('./lib/render');

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
	let log = LOG;
	const li = argv.indexOf('--log');
	if (li >= 0 && li + 1 < argv.length) {
		log = argv[li + 1];
		argv.splice(li, 2);
	}
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
		for (const line of fs.readFileSync(log, 'utf8').split('\n')) {
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
		if (buf === null) throw new Error('no usable FAIL found in ' + log);
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
		const cols = sauceCols(art.buf);
		const { ansilove, vterm: vr } = renderBoth(artPath, work, cols);
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
		require('./diff-image')
			.renderDiffImage({
				ansPng: ans, vtmPng: vtm, mapJson, out: OUT,
				titleMiddle: 'vterm-ans(ANSI.SYS)',
				diff: cmp.diff, total: cmp.total, rate: cmp.rate,
				scale: SCALE,
			})
			.catch((e) => console.error('diff-image: ' + e.message));

		console.log('file:   ' + art.name + ' (' + art.buf.length + ' B)');
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
		path.join(ROOT, 'tools/art/test-art.js'),
		'--render', '--compare',
		'--concurrency', String(concurrency),
		'--log', log, work,
	], { stdio: 'inherit' });
	if (r.error) {
		console.error('compare list: ' + r.error.message);
		process.exit(1);
	}
	/* move the log to the fixed path (compare.js one reads it by
	 * default), then drop the work tree — no temp residue */
	const finalLog = path.join(os.tmpdir(), 'art-compare.log');
	try { fs.copyFileSync(log, finalLog); } catch (e) { /* no log */ }
	fs.rmSync(work, { recursive: true, force: true });
	console.log('log: ' + finalLog);
}

/* ---- dispatch ---------------------------------------------------------- */

function usage() {
	console.error(
	    'usage: node tools/art/compare.js <one|list> [...]\n' +
	    '  one [pack entry | file.ans] [--cut N] [--log FILE]   single-file compare -> compare.png\n' +
	    '      --log FILE   FAIL log for the no-arg fallback (default /tmp/art-compare.log)\n' +
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
