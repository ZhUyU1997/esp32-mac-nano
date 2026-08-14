#!/usr/bin/env node
/* Telnet-bash server with a real pty (full interactive bash).
 *
 * Usage:
 *   node telnet_bash_srv.js [port]                    interactive bash (default)
 *   node telnet_bash_srv.js [port] --test [pattern]   fixed-output benchmark
 *   node telnet_bash_srv.js [port] --art <dir|file>   ASCII-art gallery
 *   node telnet_bash_srv.js [port] --file <file>      single-file test mode
 *        (repeatable: merge several packs)
 *
 * Benchmark patterns (deterministic, for vterm render/blit profiling):
 *   fill   — clear + full 80x30 text screen every 500ms (full-render cost)
 *   scroll — one 80-char line every 50ms (scroll + partial-render cost)
 *   idle   — nothing (idle blit baseline)
 *   burst:<kind>:<mb> — send <mb> MiB of <kind> at full speed, then idle.
 *     kind = ascii | sgr | noscroll  (noscroll = CUP home, no scroll)
 *     Used to measure the telnet→ring→parse throughput end-to-end (no
 *     reflash needed: the ESP32 drain log reports parse/flush timing).
 *   --replay <file> — send a captured raw terminal stream (e.g. pi -c) on
 *     connect, as fast as possible. Deterministic A/B test: same bytes every
 *     run, so the ESP32 drain log is comparable across firmware versions.
 *
 * --art gallery mode: load every .ans/.ice/.utf8/.txt and .zip in the
 *   directory (or a single file/zip; 16colo.rs packs are stored as .zip
 *   and unzipped in memory with Node's zlib — no external tools), convert
 *   CP437 .ans/.ice to UTF-8 with an embedded table, strip SAUCE/COMNT
 *   records, skip pieces wider than 80 cols (they would wrap on the
 *   vterm), and cycle through them: the first piece is sent on connect,
 *   Enter from the client switches to the next one.
 *
 * --file test mode: send ONE converted file on connect; Enter re-sends it
 *   (edit the file, press Enter to re-check on the vterm). No gallery
 *   navigation, no pack scanning.
 *
 * Requires: npm install node-pty
 */
const net = require('net');
const path = require('path');
const pty = require('node-pty');
const fs = require('fs');

const args = process.argv.slice(2);
let port = 2324;
let testMode = false;
let testPattern = 'fill';
let replayFile = null;
let artDirs = [];
let testFile = null;
for (let i = 0; i < args.length; i++) {
	if (args[i] === '--test') {
		testMode = true;
		if (args[i + 1] && !args[i + 1].startsWith('--'))
			testPattern = args[++i];
	} else if (args[i] === '--replay') {
		replayFile = args[++i];
	} else if (args[i] === '--art') {
		artDirs.push(args[++i]);
	} else if (args[i] === '--file') {
		testFile = args[++i];
	} else if (/^\d+$/.test(args[i])) {
		port = parseInt(args[i], 10);
	}
}

/* ---- ASCII-art gallery mode (--art) -------------------------------- */

/* CP437 byte -> UTF-8 hex (from Python's cp437 codec; block/box-drawing
 * chars match the vterm's CP437 glyph table, e.g. 0xB0=░ 0xDB=█ 0xDA=┌). */
const CP437_TO_UTF8 = [
'00',
'01',
'02',
'03',
'04',
'05',
'06',
'07',
'08',
'09',
'0a',
'0b',
'0c',
'0d',
'0e',
'0f',
'10',
'11',
'12',
'13',
'14',
'15',
'16',
'17',
'18',
'19',
'1a',
'1b',
'1c',
'1d',
'1e',
'1f',
'20',
'21',
'22',
'23',
'24',
'25',
'26',
'27',
'28',
'29',
'2a',
'2b',
'2c',
'2d',
'2e',
'2f',
'30',
'31',
'32',
'33',
'34',
'35',
'36',
'37',
'38',
'39',
'3a',
'3b',
'3c',
'3d',
'3e',
'3f',
'40',
'41',
'42',
'43',
'44',
'45',
'46',
'47',
'48',
'49',
'4a',
'4b',
'4c',
'4d',
'4e',
'4f',
'50',
'51',
'52',
'53',
'54',
'55',
'56',
'57',
'58',
'59',
'5a',
'5b',
'5c',
'5d',
'5e',
'5f',
'60',
'61',
'62',
'63',
'64',
'65',
'66',
'67',
'68',
'69',
'6a',
'6b',
'6c',
'6d',
'6e',
'6f',
'70',
'71',
'72',
'73',
'74',
'75',
'76',
'77',
'78',
'79',
'7a',
'7b',
'7c',
'7d',
'7e',
'7f',
'c387',
'c3bc',
'c3a9',
'c3a2',
'c3a4',
'c3a0',
'c3a5',
'c3a7',
'c3aa',
'c3ab',
'c3a8',
'c3af',
'c3ae',
'c3ac',
'c384',
'c385',
'c389',
'c3a6',
'c386',
'c3b4',
'c3b6',
'c3b2',
'c3bb',
'c3b9',
'c3bf',
'c396',
'c39c',
'c2a2',
'c2a3',
'c2a5',
'e282a7',
'c692',
'c3a1',
'c3ad',
'c3b3',
'c3ba',
'c3b1',
'c391',
'c2aa',
'c2ba',
'c2bf',
'e28c90',
'c2ac',
'c2bd',
'c2bc',
'c2a1',
'c2ab',
'c2bb',
'e29691',
'e29692',
'e29693',
'e29482',
'e294a4',
'e295a1',
'e295a2',
'e29596',
'e29595',
'e295a3',
'e29591',
'e29597',
'e2959d',
'e2959c',
'e2959b',
'e29490',
'e29494',
'e294b4',
'e294ac',
'e2949c',
'e29480',
'e294bc',
'e2959e',
'e2959f',
'e2959a',
'e29594',
'e295a9',
'e295a6',
'e295a0',
'e29590',
'e295ac',
'e295a7',
'e295a8',
'e295a4',
'e295a5',
'e29599',
'e29598',
'e29592',
'e29593',
'e295ab',
'e295aa',
'e29498',
'e2948c',
'e29688',
'e29684',
'e2968c',
'e29690',
'e29680',
'ceb1',
'c39f',
'ce93',
'cf80',
'cea3',
'cf83',
'c2b5',
'cf84',
'cea6',
'ce98',
'cea9',
'ceb4',
'e2889e',
'cf86',
'ceb5',
'e288a9',
'e289a1',
'c2b1',
'e289a5',
'e289a4',
'e28ca0',
'e28ca1',
'c3b7',
'e28988',
'c2b0',
'e28899',
'c2b7',
'e2889a',
'e281bf',
'c2b2',
'e296a0',
'c2a0'
];

/* CP437 -> UTF-8. The control bytes pass through unchanged: the ESC
 * sequences and CR/LF in ANSI art must reach libvterm verbatim. */
function cp437ToUtf8(buf) {
	let hex = '';
	for (let i = 0; i < buf.length; i++)
		hex += CP437_TO_UTF8[buf[i]];
	return Buffer.from(hex, 'hex');
}

/* Strip a trailing SAUCE record (128 B, starts with "SAUCE00/01") and an
 * optional COMNT record (255 B) before it; also drop a trailing DOS EOF
 * (0x1A) / NUL padding. */
function stripSauce(buf) {
	const s = buf.toString('latin1');
	for (const m of ['SAUCE01', 'SAUCE00']) {
		const i = s.lastIndexOf(m);
		if (i > 0 && buf.length - i <= 400) {
			let cut = i;
			if (i >= 255 && s.slice(i - 255, i - 250) === 'COMNT')
				cut -= 255;
			buf = buf.subarray(0, cut);
			break;
		}
	}
	let n = buf.length;
	while (n > 0 && (buf[n - 1] === 0x1a || buf[n - 1] === 0))
		n--;
	return buf.subarray(0, n);
}

/* Load every art file in the given sources (directories or single files),
 * converting CP437 .ans/.ice to UTF-8 and skipping pieces wider than 80
 * columns. */
/* In-memory ZIP reader (no external unzip): parse the end-of-central-
 * directory record, walk the central directory, inflate each entry with
 * Node's zlib. Returns [{name, buf}]. */
function readZip(buf) {
	const zlib = require('zlib');
	/* EOCD: PK\x05\x06, at least 22 B, within the last 64 KiB + 22 */
	let eocd = -1;
	for (let i = buf.length - 22; i >= Math.max(0, buf.length - 65557); i--) {
		if (buf.readUInt32LE(i) === 0x06054b50) {
			eocd = i;
			break;
		}
	}
	if (eocd < 0)
		throw new Error('not a zip archive (no end-of-central-directory)');
	const count = buf.readUInt16LE(eocd + 10);
	const cdOff = buf.readUInt32LE(eocd + 16);
	const entries = [];
	let off = cdOff;
	for (let i = 0; i < count; i++) {
		if (buf.readUInt32LE(off) !== 0x02014b50)
			break; /* central directory signature */
		const method = buf.readUInt16LE(off + 10);
		const csize = buf.readUInt32LE(off + 20);
		const nlen = buf.readUInt16LE(off + 28);
		const elen = buf.readUInt16LE(off + 30);
		const clen = buf.readUInt16LE(off + 32);
		const lho = buf.readUInt32LE(off + 42);
		const name = buf.toString('latin1', off + 46, off + 46 + nlen);
		/* local file header precedes the data */
		const lhNameLen = buf.readUInt16LE(lho + 26);
		const lhExtraLen = buf.readUInt16LE(lho + 28);
		const data = buf.subarray(lho + 30 + lhNameLen + lhExtraLen,
		                         lho + 30 + lhNameLen + lhExtraLen + csize);
		if (method === 8)
			entries.push({ name: name, buf: zlib.inflateRawSync(data) });
		else if (method === 0)
			entries.push({ name: name, buf: data });
		else {
			/* old PKZIP methods (6=implode, 12=bzip2, 14=lzma, ...) are not
			 * supported by Node's zlib: skip the entry, keep the pack */
			console.log('art: skip ' + name + ' (zip method ' + method + ' unsupported)');
		}
		off += 46 + nlen + elen + clen;
	}
	return entries;
}

/* Collect art units from dirs (recursive), single files, or .zip archives:
 * each non-zip file is a one-piece unit, each .zip is a pack unit whose
 * pieces are decompressed lazily (a large collection of packs must not be
 * loaded into memory at once). */
function collectUnits(sources) {
	const units = [];
	const addZip = (p, display) => units.push({ kind: 'zip', name: display, path: p });
	const scanDir = (dir) => {
		for (const f of fs.readdirSync(dir).sort()) {
			const p = path.join(dir, f);
			if (fs.statSync(p).isDirectory()) {
				scanDir(p);
			} else if (/\.(ans|ice|utf8|txt)$/i.test(f)) {
				units.push({ kind: 'file', name: f, path: p });
			} else if (/\.zip$/i.test(f)) {
				addZip(p, path.relative(dir, p));
			}
		}
	};
	for (const src of sources) {
		if (fs.statSync(src).isDirectory()) {
			scanDir(src);
		} else if (/\.zip$/i.test(src)) {
			addZip(src, path.basename(src));
		} else {
			units.push({ kind: 'file', name: path.basename(src), path: src });
		}
	}
	return units;
}

/* Convert one art buffer: strip SAUCE, CP437->UTF-8, width/rows checks.
 * Returns the piece, or null if it is wider than 80 columns. */
/* BBS-era .ans/.ice/.txt are CP437 bytes; convert them unless they
 * are already valid UTF-8 (a UTF-8 text file must pass through). */
const k_utf8_check = new TextDecoder('utf-8', { fatal: true });
function isUtf8(buf) {
	try {
		k_utf8_check.decode(buf);
		return true;
	} catch (e) {
		return false;
	}
}

function convertPiece(name, buf) {
	buf = stripSauce(buf);
	if (/\.(ans|ice|txt)$/i.test(name) && !isUtf8(buf))
		buf = cp437ToUtf8(buf);
	let w = 0, maxw = 0, rows = 1;
	for (const ch of buf.toString('utf8')) {
		if (ch === '\n') {
			if (w > maxw) maxw = w;
			w = 0;
			rows++;
		} else {
			w++;
		}
	}
	if (w > maxw) maxw = w;
	if (maxw > 80) {
		console.log('art: skip ' + name + ' (width ' + maxw + ' > 80)');
		return null;
	}
	if (rows > 30)
		console.log('art: ' + name + ' is ' + rows + ' rows tall (>30: scrolls on the vterm)');
	console.log('art: loaded ' + name + ' (' + buf.length + ' B, ' + maxw + 'x' + rows + ')');
	return { name: name, buf: buf, w: maxw, rows: rows };
}

/* deterministic byte pattern for a burst (all bytes < 256, latin1-safe) */
function makePattern(kind) {
	let s = '';
	if (kind === 'ascii') {
		let line = '';
		for (let col = 0; col < 80; col++)
			line += String.fromCharCode(65 + col % 26);
		s = line + '\r\n';
	} else {
		for (let i = 0; i < 40; i++) {
			if (kind === 'noscroll')
				s += '\x1b[1;1H';
			s += '\x1b[38;2;' + (i % 255) + ';' + ((i * 3) % 255) + ';' +
			     ((i * 5) % 255) + 'mword' + String(i).padStart(3, '0') +
			     ' \x1b[0m\x1b]8;;\x07';
			if (kind === 'sgr')
				s += '\r\r\n';
		}
	}
	return Buffer.from(s, 'latin1');
}

function fillScreen(row0) {
	let out = '\x1b[2J\x1b[H';
	for (let row = 0; row < 30; row++) {
		let line = '';
		for (let col = 0; col < 80; col++)
			line += String.fromCharCode(65 + ((row0 + row) * 5 + col) % 26);
		out += line + '\r\n';
	}
	return out;
}

const artMode = artDirs.length > 0;
const artUnits = artMode ? collectUnits(artDirs) : null;
const fileMode = testFile !== null;
const filePiece = fileMode ? convertPiece(path.basename(testFile), fs.readFileSync(testFile)) : null;

const server = net.createServer((sock) => {
	if (replayFile) {
		console.log('client connected (replay: ' + replayFile + ')');
		const buf = fs.readFileSync(replayFile);
		console.log('replay: ' + buf.length + ' B');
		const t0 = Date.now();
		let off = 0;
		const writeNext = () => {
			while (off < buf.length) {
				if (!sock.write(buf.subarray(off, off + 4096))) {
					off += 4096;
					sock.once('drain', writeNext);
					return;
				}
				off += 4096;
			}
			const dt = Date.now() - t0;
			console.log('replay done: ' + buf.length + ' B in ' + dt + ' ms (' +
			            (buf.length / dt / 1000).toFixed(2) + ' MB/s, host-side)');
		};
		writeNext();
		sock.on('close', () => {});
		sock.on('error', () => {});
		return;
	}
	if (testMode) {
		console.log('client connected (test mode: ' + testPattern + ')');
		if (testPattern === 'idle') {
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		if (testPattern.startsWith('burst:')) {
			const parts = testPattern.split(':');
			const kind = parts[1] || 'ascii';
			const mb = parseFloat(parts[2] || '1');
			const buf = makePattern(kind);
			const total = Math.floor((mb * 1024 * 1024) / buf.length);
			console.log('burst ' + kind + ': pattern ' + buf.length + ' B x' + total +
			            ' = ' + (buf.length * total / 1048576).toFixed(2) + ' MiB');
			const t0 = Date.now();
			let i = 0;
			const writeNext = () => {
				while (i < total) {
					if (!sock.write(buf)) {
						i++;
						sock.once('drain', writeNext);
						return;
					}
					i++;
				}
				const dt = Date.now() - t0;
				console.log('burst done: ' + buf.length * total + ' B in ' + dt +
				            ' ms (' + (buf.length * total / dt / 1000).toFixed(1) + ' MB/s, host-side)');
			};
			writeNext();
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		let row0 = 0;
		const timer = testPattern === 'scroll'
			? setInterval(() => {
				let line = '';
				for (let col = 0; col < 80; col++)
					line += String.fromCharCode(65 + (row0 * 5 + col) % 26);
				sock.write(line + '\r\n');
				row0++;
			}, 50)
			: setInterval(() => {
				sock.write(fillScreen(row0));
				row0++;
			}, 500);
		sock.on('close', () => clearInterval(timer));
		sock.on('error', () => clearInterval(timer));
		return;
	}

	if (fileMode) {
		if (!filePiece) {
			console.log('client connected (file mode: piece skipped, nothing to send)');
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		console.log('client connected (file test: ' + filePiece.name + ' ' + filePiece.w + 'x' + filePiece.rows + ')');
		const send = () => {
			sock.write('\x1b[2J\x1b[H');
			sock.write(filePiece.buf);
		};
		send();
		/* Enter re-sends the (possibly edited) file */
		sock.on('data', (d) => {
			const s = d.toString('latin1');
			if (s.indexOf('\r') >= 0 || s.indexOf('\n') >= 0)
				send();
		});
		sock.on('close', () => {});
		sock.on('error', () => {});
		return;
	}

	if (artMode) {
		const nUnits = artUnits.length;
		if (nUnits === 0) {
			console.log('client connected (art gallery: no pieces found)');
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		console.log('client connected (art gallery: ' + nUnits + ' units)');
		let zipCache = null; /* the currently decompressed pack */
		let ui = 0;         /* current unit index */
		let pi = 0;         /* piece index within the unit */
		const loadPieces = (unit) => {
			/* decompress a pack lazily, keep only the current one */
			if (zipCache && zipCache.path === unit.path)
				return zipCache.pieces;
			console.log('art: pack ' + unit.name);
			const pieces = [];
			let entries;
			try {
				entries = readZip(fs.readFileSync(unit.path));
			} catch (e) {
				console.log('art: pack ' + unit.name + ' FAILED: ' + e.message);
				zipCache = { path: unit.path, pieces: [] };
				return zipCache.pieces;
			}
			for (const e of entries)
				if (/\.(ans|ice|utf8|txt)$/i.test(e.name)) {
					const p = convertPiece(path.basename(e.name), e.buf);
					if (p)
						pieces.push(p);
				}
			zipCache = { path: unit.path, pieces: pieces };
			return pieces;
		};
		const unitPieces = (unit) => {
			if (unit.kind === 'file') {
				/* convert the single file lazily (width filter may drop it) */
				if (!unit.piece) {
					const p = convertPiece(unit.name, fs.readFileSync(unit.path));
					unit.piece = p || null;
				}
				return unit.piece ? [unit.piece] : [];
			}
			return loadPieces(unit);
		};
		/* advance past units with no displayable pieces (e.g. a pack whose
		 * entries all use an unsupported zip method); false if nothing left */
		const seekPiece = () => {
			for (let g = 0; g < nUnits; g++) {
				const unit = artUnits[ui];
				if (unitPieces(unit).length > 0)
					return true;
				ui = (ui + 1) % nUnits;
				pi = 0;
			}
			return false;
		};
		const show = () => {
			if (!seekPiece()) {
				console.log('art: no displayable pieces');
				return;
			}
			const unit = artUnits[ui];
			const pieces = unitPieces(unit);
			const p = pieces[Math.min(pi, pieces.length - 1)];
			console.log('art: show [' + (pi + 1) + '/' + pieces.length + '] ' + p.name +
			            ' (' + p.w + 'x' + p.rows + ')' + (unit.kind === 'zip' ? ' @' + unit.name : ''));
			sock.write('\x1b[2J\x1b[H');
			sock.write(p.buf);
		};
		const next = () => {
			pi++;
			while (pi >= unitPieces(artUnits[ui]).length) {
				pi = 0;
				ui = (ui + 1) % nUnits;
			}
			show();
		};
		show();
		sock.on('data', (d) => {
			const s = d.toString('latin1');
			/* Enter from the client (the vterm sends CR) cycles to the
			 * next piece; any other keys are ignored */
			if (s.indexOf('\r') >= 0 || s.indexOf('\n') >= 0)
				next();
		});
		sock.on('close', () => {});
		sock.on('error', () => {});
		return;
	}

	console.log('client connected');
	const p = pty.spawn('/bin/bash', ['-i'], {
		name: 'xterm-256color',
		cols: 80,
		rows: 30,
		env: process.env,
	});
	p.onData((d) => {
		try { sock.write(d); } catch (e) {}
	});
	sock.on('data', (d) => p.write(d.toString('utf8')));
	sock.on('close', () => p.kill());
	sock.on('error', () => p.kill());
	p.onExit(() => {
		try { sock.destroy(); } catch (e) {}
	});
});

server.listen(port, '0.0.0.0', () => {
	let what = '';
	if (testMode)
		what = ' (test: ' + testPattern + ')';
	else if (fileMode)
		what = ' (file: ' + filePiece.name + ')';
	else if (artMode)
		what = ' (art gallery: ' + artUnits.length + ' units)';
	console.log('listening on :' + port + what);
});
