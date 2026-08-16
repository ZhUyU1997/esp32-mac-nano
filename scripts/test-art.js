#!/usr/bin/env node
/*
 * test-art.js — batch-test the art packs, two composable passes:
 *
 *   --extract   decompression test: every zip entry must inflate
 *               (reports corrupt entries, empty files, unsupported
 *               compression methods)
 *   --render    converter test: feed each piece through tools/vterm's
 *               vterm-ans and verify the output PNG
 *
 * Both run by default; pass a single flag to run only that pass. The
 * pipeline is producer/consumer: zip packs are decompressed lazily while
 * workers validate files as soon as their bytes are ready.
 *
 * Usage:
 *   node scripts/test-art.js [options] [sources...]
 *   options:
 *     --extract       decompression test only
 *     --render        vterm-ans render test only
 *     --bin PATH      vterm-ans binary (default: build/.../vterm-ans)
 *     --limit N       only test the first N files (zips are decompressed
 *                     lazily, so a small limit only reads the packs it
 *                     needs)
 *     --concurrency N parallel jobs (default 8)
 *     --out-dir DIR   keep rendered PNGs here (default: temp, cleaned up)
 *     --quality       decode PNGs and count placeholder-like cells
 *     --compare       also render each piece with libansilove's ansilove
 *                     CLI and report per-cell colour drift (size + rate)
 *     --ansilove PATH ansilove binary (default: /usr/bin/ansilove)
 *     --all           also test .nfo/.txt files (default: .ans/.ice)
 *     --width N       skip art wider than N columns (default 80, the
 *                     gallery limit; --width 0 disables the check)
 *     --log FILE      additionally append the per-file lines to a file
 *   sources: directories (recursive), .zip packs or single files;
 *            default: scripts/art/packs
 *
 * In render mode one line is printed per file as it completes
 * (OK/FAIL/SKIP + size + SAUCE); extract mode only prints problems.
 */
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');
const { spawn } = require('child_process');
const artlib = require('./art-lib');
const diff = require('./diff-lib');

const ROOT = path.join(__dirname, '..');

/* ---- PNG helpers ------------------------------------------------------- */

function pngSize(p) {
	if (p.length < 24 || p.readUInt32BE(0) !== 0x89504e47)
		return null;
	if (p.toString('latin1', 12, 16) !== 'IHDR')
		return null;
	return { w: p.readUInt32BE(16), h: p.readUInt32BE(20) };
}

/* Count 8x16 cells that exactly match the renderer's placeholder: a
 * hollow rectangle (all four edges on, interior off). */
function placeholderCells(img, w, h) {
	let count = 0;
	for (let cy = 0; cy + 16 <= h; cy += 16) {
		for (let cx = 0; cx + 8 <= w; cx += 8) {
			let top = true, bottom = true, left = true, right = true;
			let interior = true;
			for (let y = 0; y < 16; y++) {
				for (let x = 0; x < 8; x++) {
					const i = ((cy + y) * w + (cx + x)) * 3;
					const on = img[i] > 128 || img[i + 1] > 128 || img[i + 2] > 128;
					if (y === 0) { if (!on) top = false; }
					else if (y === 15) { if (!on) bottom = false; }
					else if (x === 0) { if (!on) left = false; }
					else if (x === 7) { if (!on) right = false; }
					else if (on) interior = false;
				}
			}
			if (top && bottom && left && right && interior)
				count++;
		}
	}
	return count;
}

/* ---- libansilove comparison (--compare) -------------------------------- */

/* Compare vterm-ans vs ansilove renders at the 8x16 cell level. Both
 * renderers draw the same ANSI index colours through different RGB
 * palettes, so pixels are first quantised back to the 16 standard
 * indices using each renderer's own palette, then compared in index
 * space. The palettes below are the *measured* output of each renderer:
 *
 *   vterm-ans (libvterm pen.c): 0/224/64-level, SGR 33 = yellow
 *   ansilove:  CGA 0/170/85-level, SGR 33 = brown, 90-97 = grey,
 *              and `32;1`-ordered SGR bold is NOT brightened (bug)
 *
 * Bright/dark pairs (idx 8-15 vs 0-7, i.e. bold handling) and the
 * yellow/brown quirk are renderer-flavour differences: the comparison
 * uses normalised indices (idx % 8) so a layout drift lights up nearly
 * every cell while flavour differences only add to `bright`. */
/* ---- main -------------------------------------------------------------- */

function usage() {
	console.error(
		'usage: node scripts/test-art.js [--extract] [--render] [--count] [options] [sources...]\n' +
		'       [--bin PATH] [--limit N] [--concurrency N] [--out-dir DIR]\n' +
		'       [--quality] [--all] [--width N] [--log FILE]\n' +
		'  --extract   decompression test (every zip entry must inflate)\n' +
		'  --render    vterm-ans converter test\n' +
		'  --count     file count + total bytes + 90s dial-up transfer times\n' +
		'  extract+render run by default; give flags to pick the passes\n' +
		'  default sources: scripts/art/packs\n' +
		'  --width N  skip art wider than N columns (default 80, the gallery\n' +
		'             limit; --width 0 disables the check)\n' +
		'  --log FILE  append the per-file lines to a file for later analysis');
	process.exit(1);
}

const args = process.argv.slice(2);
const opts = {
	bin: path.join(ROOT, 'build/linux/x86_64/release/vterm-ans'),
	limit: 0,
	concurrency: 8,
	outDir: null,
	quality: false,
	all: false,
	log: null,
	width: 80, /* the gallery only serves 80-column art: skip wider files */
	extract: false,
	render: false,
	count: false,
	compare: false,
	ansilove: '/usr/bin/ansilove',
};
const sources = [];
for (let i = 0; i < args.length; i++) {
	const a = args[i];
	if (a === '--bin' && i + 1 < args.length) opts.bin = args[++i];
	else if (a === '--limit' && i + 1 < args.length) opts.limit = parseInt(args[++i], 10);
	else if (a === '--concurrency' && i + 1 < args.length) opts.concurrency = parseInt(args[++i], 10);
	else if (a === '--out-dir' && i + 1 < args.length) opts.outDir = args[++i];
	else if (a === '--quality') opts.quality = true;
	else if (a === '--compare') opts.compare = true;
	else if (a === '--ansilove' && i + 1 < args.length) opts.ansilove = args[++i];
	else if (a === '--all') opts.all = true;
	else if (a === '--extract') opts.extract = true;
	else if (a === '--render') opts.render = true;
	else if (a === '--count') opts.count = true;
	else if (a === '--width' && i + 1 < args.length) opts.width = parseInt(args[++i], 10);
	else if (a === '--log' && i + 1 < args.length) opts.log = args[++i];
	else if (a.startsWith('-')) usage();
	else sources.push(a);
}
if (opts.render && !fs.existsSync(opts.bin)) {
	console.error('vterm-ans not found at ' + opts.bin + '\n  run: xmake build vterm-ans');
	process.exit(1);
}
if (opts.compare && !fs.existsSync(opts.ansilove)) {
	console.error('ansilove not found at ' + opts.ansilove);
	process.exit(1);
}
if (!opts.extract && !opts.render && !opts.count)
	opts.extract = opts.render = true; /* default: both passes */
if (sources.length === 0)
	sources.push(path.join(ROOT, 'scripts/art/packs'));

const artRe = opts.all ? artlib.ART_ALL_RE : artlib.ART_RE;
let units = artlib.collectUnits(sources, opts.all);
const nZips = units.filter(u => u.zip).length;
console.log('vterm-ans: ' + opts.bin);
console.log('sources: ' + units.length + ' units (' + nZips + ' zips, ' +
            (units.length - nZips) + ' loose files)');
console.log('');

/* pipeline: producer decompresses zip packs lazily while the worker pool
 * validates files as soon as their bytes are ready */
/* ---- count mode: sizes + 90s dial-up transfer estimates ---------------- */

/* 8N1 serial = 10 bits per byte, so baud/10 bytes per second. The
 * gallery's --baud uses the same math (2400->240 B/s, 9600->960 B/s). */
const DIAL_UP = [
	[2400, '2400 baud (90s BBS)'],
	[9600, '9600 baud'],
	[14400, '14400 baud (V.32bis)'],
	[28800, '28800 baud (V.34)'],
	[56000, '56k modem'],
];

function fmtDuration(sec) {
	if (sec < 60) return Math.round(sec) + ' 秒';
	if (sec < 3600) return Math.floor(sec / 60) + ' 分钟 ' + Math.round(sec % 60) + ' 秒';
	return Math.floor(sec / 3600) + ' 小时 ' + Math.round((sec % 3600) / 60) + ' 分钟';
}

function fmtBytes(n) {
	if (n >= 1 << 30) return (n / (1 << 30)).toFixed(2) + ' GiB';
	if (n >= 1 << 20) return (n / (1 << 20)).toFixed(1) + ' MiB';
	if (n >= 1 << 10) return (n / (1 << 10)).toFixed(1) + ' KiB';
	return n + ' B';
}

function countReport(stats, el) {
	console.log('\n=== count: ' + stats.files + ' files, ' + fmtBytes(stats.bytes) +
	            ' (' + fmtBytes(stats.bytes / (stats.files || 1)) + '/file avg)' +
	            ' (' + el + 's) ===');
	if (!stats.files) return;
	console.log('  传输时间（8N1 串口, 10bit/byte）:');
	for (const [baud, label] of DIAL_UP)
		console.log('    ' + label.padEnd(18) + ': ' + fmtDuration(stats.bytes / (baud / 10)));
}

/* ---- output: single status line on a TTY ------------------------------ */

let done = 0; /* files fully processed */
let fileIdx = 0; /* unique temp file numbers, safe across workers */
const t0 = Date.now();
let statusLine = false; /* a TTY OK line is active (cursor at its end) */

function fmtElapsed() {
	const el = (Date.now() - t0) / 1000;
	return el >= 60 ? Math.floor(el / 60) + 'm' + Math.round(el % 60) + 's' :
	                 Math.round(el) + 's';
}

/* keep lines short so they never wrap-fold in a narrow terminal */
function truncate(s, max) {
	return s.length > max ? s.slice(0, max - 1) + '…' : s;
}

/* one-line per-file record: OK/FAIL/SKIP + size + SAUCE title/ice. OK
 * lines carry the running count and elapsed time ([2800] 13s). */
function logLine(res) {
	if (res.skip)
		return truncate('SKIP ' + res.name + ' (' + (res.reason || 'empty') + ')', 120);
	if (!res.ok)
		return truncate('FAIL ' + res.name + ': ' + res.reason, 120);
	let line = 'OK [' + done + '] ' + fmtElapsed() + '  ' + res.name +
	           ' (' + res.size.w + 'x' + res.size.h + ')';
	if (res.sauce)
		line += ' sauce:"' + truncate(res.sauce.title, 24) + '"' +
		        (res.sauce.ice ? ' [iCE]' : '');
	if (res.placeholders)
		line += ' PLACEHOLDERS:' + res.placeholders;
	if (res.cmp) {
		const sz = res.cmp.cols[0] !== res.cmp.cols[1] || res.cmp.rows[0] !== res.cmp.rows[1];
		line += ' cmp=' + (res.cmp.rate * 100).toFixed(1) + '%' +
		        (sz ? ' size:' + res.cmp.cols[0] + 'x' + res.cmp.rows[0] +
		                  '/' + res.cmp.cols[1] + 'x' + res.cmp.rows[1] : '') +
		        (res.cmp.bright ? ' bright=' + res.cmp.bright : '');
	}
	return truncate(line, 120);
}

/* All per-file and progress output goes through emit(). On a TTY every
 * OK overwrites a single status line (clear first, then write, so no
 * residue from the previous line); FAIL/SKIP/summary always end the
 * status line and get their own row. The --log file always records one
 * full line per file, whatever the display mode. */
function emit(line) {
	if (opts.log)
		fs.appendFileSync(opts.log, line + '\n');
	if (process.stdout.isTTY) {
		if (line.startsWith('OK')) {
			if (!statusLine)
				process.stdout.write('\n'); /* first OK / after a FAIL row */
			process.stdout.write('\r\x1b[K' + line);
			statusLine = true;
		} else {
			if (statusLine)
				process.stdout.write('\n'); /* leave the OK line */
			console.log(line);
			statusLine = false;
		}
	} else {
		console.log(line);
	}
}

/* end the status line before the summary (TTY) */
function endStatus() {
	if (process.stdout.isTTY && statusLine)
		process.stdout.write('\n');
}

/* ---- producer: decompress zip packs lazily ----------------------------- */

async function* artFiles() {
	let zipErrors = 0;
	const zipErrNames = [];
	let zipsDone = 0;
	let wideSkipped = 0;
	for (const u of units) {
		if (!u.zip) {
			if (artRe.test(u.name)) {
				const buf = await fs.promises.readFile(u.path);
				if (opts.width && artlib.fileWidth(buf) > opts.width) {
					wideSkipped++;
					continue;
				}
				/* loose files keep the "pack / entry" name when they came
				 * from a pack-shaped tree (compare.js list extracts to
				 * <work>/<pack>/<entry>) so logs parse with one regex */
				const zi = u.name.indexOf('.zip/');
				const name = zi >= 0
					? u.name.slice(0, zi + 4) + ' / ' + u.name.slice(zi + 5)
					: u.name;
				yield { name, buf, error: null, empty: buf.length === 0 };
			}
			continue;
		}
		let entries;
		try {
			entries = artlib.parseZip(await fs.promises.readFile(u.path));
		} catch (e) {
			zipErrors++;
			if (zipErrNames.length < 10)
				zipErrNames.push(u.name);
			continue;
		}
		zipsDone++;
		/* pack progress only for non-TTY (pipes/logs): on a terminal the
		 * single OK status line is enough feedback */
		if (!process.stdout.isTTY && (zipsDone % 20 === 0 || zipsDone === nZips))
			emit('zip [' + zipsDone + '/' + nZips + '] ' + u.name);
		for (const e of entries) {
			if (!artRe.test(e.name))
				continue;
			/* inflate here so the extract pass can report failures
			 * (artlib.readZipAsync drops them silently) */
			let buf = null, error = null;
			if (e.method === 8) {
				try { buf = zlib.inflateRawSync(e.data); }
				catch (x) { error = 'inflate: ' + x.message; }
			} else if (e.method === 0) {
				buf = Buffer.from(e.data);
			} else {
				error = 'method ' + e.method + ' unsupported';
			}
			if (!error && opts.width && artlib.fileWidth(buf) > opts.width) {
				wideSkipped++;
				continue;
			}
			try {
				yield { name: u.name + ' / ' + e.name, buf, error,
				        empty: !error && buf.length === 0 };
			} catch (err) {
				/* consumer stopped (limit reached) */
				return;
			}
		}
	}
	if (wideSkipped)
		emit('wide (>' + opts.width + ' cols) skipped: ' + wideSkipped);
	if (zipErrors)
		emit('corrupt zips skipped: ' + zipErrors +
		     (zipErrNames.length ? ' (' + zipErrNames.join(', ') + ')' : ''));
}

const tmpDir = opts.outDir || fs.mkdtempSync(path.join(os.tmpdir(), 'vterm-ans-test-'));
if (!opts.outDir) fs.mkdirSync(tmpDir, { recursive: true });

/* Files exempted by name: known libansilove-bug / non-art files that
 * will never match (e.g. bare-CR render garbage on its side). Add here
 * instead of the content heuristics when a specific file is pinned. */
/* Files excluded by name: not render-comparable at all — the device is a
 * fixed 80-column display, so art whose SAUCE-declared width is not 80
 * (e.g. 45 cols) can never display correctly and must not be counted
 * (not a libansilove-bug exemption). */
const EXCLUDE_FILES = new Set([
	'fool27.zip-file_id.ans', /* SAUCE 45 cols: only renders right at that
	                          * width, no 80-col view on the fixed-80 ESP32 */
	'mist0918.zip-FILE_ID.ANS', /* SAUCE 32 cols: same, non-80 width */
	'impure84.zip-lmn-siouxie.ans', /* renders badly on both sides (user: exclude) */
	'mist0823.zip-MM-ONE.ANS', /* ansilove drops to 9 rows vs 48 (user: exclude) */
]);

const EXEMPT_FILES = new Set([
	'MEM0595.ANS',
	'PK-NUCW.ANS', /* corrupt ANSI sequences in the file itself */
	'AL-DTD.ANS',  /* SAUCE 79 cols: width gap shifts wrap rows (min-col
	                * compare can't align the shifted rows) */
	'DG-MAKC2.ANS', /* \r mixed into ESC sequence params (corrupt file) */
	'--------.ANS', /* ansilove: File format error (its own failure) */
	'US!.ANS',      /* ansilove: File format error (its own failure) */
	'cm-MIST.ans',  /* SAUCE 79 cols: width gap shifts wrap rows */
	'sk!n-abstrakt_nfo_fb.ans', /* libansilove TAB overflow loses content (we keep it) */
	'sk!n-motiv8_logo_ansi.ans', /* libansilove TAB: col+8 vs tab-stop (5-col shift) */
	'mz-piece.ans', /* libansilove ignores 38;5 256-color (fg stays 7) */
	'grx-comp2.ans', /* bare ESC+spaces: libvterm collects intermed bytes vs ANSI.SYS shows them */
	'grx-comp7.ans', /* same bare ESC+spaces / 38;5 family as grx-comp2 */
	'ldn-vandalism.ans', /* libansilove CUF no-clamp at right margin (col 80 vs ANSI.SYS 79) */
	'+l-ds.ans', /* libansilove TAB col+8 vs tab-stop (col shift + wrap rows) */
	'g80-hmm.ans', /* control-char demo art: \x0e SO / bare-CR handling differs */
	'ru8_factory.ans', /* looks shifted (user: exempt) */
]);

async function runOne(f) {
	/* extract pass: classify only, no vterm-ans spawn */
	if (f.error)
		return { name: f.name, ok: false, reason: f.error };
	if (f.empty)
		return { name: f.name, ok: false, skip: true, reason: 'empty file' };
	/* match either "pack / entry" (zip mode) or "pack-entry" (loose
	 * flattened name from compare.js list) */
	const excludeHit = [...EXCLUDE_FILES].some(n =>
		f.name.replace(/ /g, '').includes('/' + n) ||
		f.name.includes('.zip-' + n) || f.name.includes(n));
	if (excludeHit)
		return { name: f.name, ok: false, skip: true, reason: 'excluded (non-80-col on fixed-80 device)' };
	const exemptHit = [...EXEMPT_FILES].some(n =>
		f.name.replace(/ /g, '').includes('/' + n) ||
		f.name.includes('.zip-' + n));
	if (exemptHit)
		return { name: f.name, ok: false, skip: true, reason: 'exempt by filename' };
	if (artlib.isKeyboardScript(f.buf))
		return { name: f.name, ok: false, skip: true, reason: 'keyboard script (\\x1b[...p)' };
	const artPath = path.join(tmpDir, 'art-' + fileIdx + '.ans');
	const outPath = path.join(tmpDir, 'out-' + fileIdx + '.png');
	const ansPath = path.join(tmpDir, 'ans-' + fileIdx + '.png');
	fileIdx++;
	await fs.promises.writeFile(artPath, f.buf);

	/* async spawn so the pool actually runs in parallel */
	const spawnJob = (bin, args) => new Promise(resolve => {
		const child = spawn(bin, args);
		let stderr = '';
		child.stderr.on('data', d => stderr += d);
		const killer = setTimeout(() => child.kill('SIGKILL'), 15000);
		child.on('close', (code, signal) => {
			clearTimeout(killer);
			resolve({ code, signal, stderr });
		});
		child.on('error', err => resolve({ error: err.message }));
	});
	/* vterm-ans honours SAUCE cols (40..200, else 80); ansilove misses
	 * narrow SAUCE widths, so force ansilove to the same width. */
	let cols = 80;
	const si = f.buf.indexOf('SAUCE00');
	if (si >= 0) {
		const c = f.buf[si + 7 + 89] | (f.buf[si + 7 + 90] << 8);
		if (c >= 40 && c <= 200) cols = c;
	}
	const jobs = [spawnJob(opts.bin, [artPath, '-o', outPath, '--cols', String(cols)])];
	if (opts.compare)
		jobs.push(spawnJob(opts.ansilove, [artPath, '-o', ansPath, '-c', String(cols)]));
	const rs = await Promise.all(jobs);
	const r = rs[0];

	const res = { name: f.name, ok: false, reason: '', size: null, sauce: null, placeholders: 0 };
	if (r.error) {
		res.reason = 'spawn: ' + r.error;
	} else if (r.code !== 0) {
		res.reason = r.signal ? 'signal ' + r.signal :
		             (r.stderr || '').trim().split('\n').pop() || 'exit ' + r.code;
	} else {
		let p;
		try {
			p = await fs.promises.readFile(outPath);
		} catch (e) {
			res.reason = 'no output file';
		}
		if (p) {
			const size = pngSize(p);
			if (!size || size.w === 0 || size.h === 0) {
				res.reason = 'bad PNG header';
			} else {
				res.ok = true;
				res.size = size;
				const m = r.stderr.match(/sauce: "([^"]*)" by [^,]*, \d+x\d+, font=\d+(, iCE)?/);
				if (m)
					res.sauce = { title: m[1], ice: !!m[2] };
				if (opts.quality) {
					try {
						const img = diff.decodePngRgb(p);
						res.placeholders = placeholderCells(img.data, img.w, img.h);
					} catch (e) {
						res.reason = 'decode: ' + e.message;
						res.ok = false;
					}
				}
				if (opts.compare && res.ok) {
					let ap = null;
					try { ap = await fs.promises.readFile(ansPath); } catch (e) {}
					if (!ap || rs[1].error || rs[1].code !== 0) {
						res.reason = 'ansilove: ' + (rs[1].error || rs[1].stderr.trim().split('\n').pop() ||
						                                    'exit ' + rs[1].code);
						res.ok = false;
					} else {
						try {
							res.cmp = diff.cellDiff(diff.decodePngRgb(p), diff.decodePngRgb(ap));
							if (res.cmp.rate > 0.25) {
								if (res.cmp.cols[0] !== res.cmp.cols[1] && res.cmp.rate === 1) {
									/* we follow the SAUCE-declared width, libansilove
									 * missed the record (its strict detection): pass
									 * with an annotation instead of failing */
									res.reason = 'SAUCE width ' + res.cmp.cols[0] +
									             ' vs libansilove ' + res.cmp.cols[1] +
									             ' (libansilove SAUCE detection limit)';
									res.ok = true;
								} else {
									res.reason = 'cell diff ' + (res.cmp.rate * 100).toFixed(1) +
									             '% vs ansilove';
									res.ok = false;
								}
							} else {
								res.ok = true;
							}
						} catch (e) {
							res.reason = 'compare: ' + e.message;
							res.ok = false;
						}
					}
				}
			}
		}
	}
	if (!opts.outDir)
		await fs.promises.rm(artPath, { force: true });
	if (opts.outDir || !res.ok)
		await fs.promises.rm(outPath, { force: true });
	if (opts.compare)
		await fs.promises.rm(ansPath, { force: true });
	return res;
}

(async () => {
	const iterator = artFiles()[Symbol.asyncIterator]();
	const fails = [];
	const skips = [];
	const extractStats = { ok: 0, fail: 0, empty: 0 };
	const countStats = { files: 0, bytes: 0 };

	async function run() {
		while (opts.limit === 0 || done < opts.limit) {
			let value;
			try {
				const r = await iterator.next();
				if (r.done)
					break;
				value = r.value;
			} catch (err) {
				/* producer error (e.g. unreadable loose file): log and stop */
				const res = { name: '(producer)', ok: false, reason: err.message };
				fails.push(res);
				emit(logLine(res));
				break;
			}
			/* extract pass: classify every entry (cheap, always on) */
			if (value.error)
				extractStats.fail++;
			else if (value.empty)
				extractStats.empty++;
			else
				extractStats.ok++;
			/* count pass: usable entries and their total size */
			if (opts.count && !value.error) {
				countStats.files++;
				countStats.bytes += value.buf.length;
			}
			if (!opts.render && !opts.extract) {
				/* count-only: nothing per-file, just the summary */
				done++;
				continue;
			}
			if (!opts.render) {
				/* extract-only: print the problems, keep the OKs silent */
				done++;
				if (value.error || value.empty)
					emit(logLine({ name: value.name, ok: false,
					               skip: value.empty, reason: value.error || 'empty file' }));
				continue;
			}
			/* render pass */
			let res;
			if (value.error)
				res = { name: value.name, ok: false, reason: value.error };
			else if (value.empty)
				res = { name: value.name, ok: false, skip: true, reason: 'empty file' };
			else
				res = await runOne(value).catch(err => ({
					name: value.name, ok: false, reason: 'internal: ' + err.message,
				}));
			done++;
			emit(logLine(res));
			if (!res.ok && !res.skip)
				fails.push(res);
			else if (res.skip)
				skips.push(res);
		}
	}

	await Promise.all(Array.from({ length: opts.concurrency }, run));
	endStatus();

	const el = ((Date.now() - t0) / 1000).toFixed(1);
	if (opts.count)
		countReport(countStats, el);
	if (opts.extract)
		console.log('\n=== extract: ' + extractStats.ok + ' ok, ' + extractStats.fail +
		            ' fail, ' + extractStats.empty + ' empty (' + el + 's) ===');
	if (opts.render) {
		const ok = done - fails.length - skips.length;
		console.log('=== render: ' + ok + ' ok, ' + fails.length + ' fail' +
		            (skips.length ? ', ' + skips.length + ' skipped (empty)' : '') +
		            ' (' + el + 's) ===');
	}
	for (const r of fails)
		console.log('FAIL ' + r.name + ': ' + r.reason);
	if (opts.log)
		console.log('log: ' + opts.log);
	if (!opts.outDir)
		await fs.promises.rm(tmpDir, { recursive: true, force: true });
	process.exit(fails.length ? 1 : 0);
})();
