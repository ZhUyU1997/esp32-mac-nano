/* art-lib.js — shared ANSI-art traversal and analysis helpers.
 *
 * Used by scripts/telnet_bash_srv.js (the gallery server) and
 * scripts/test-vterm-ans.js (batch converter tests) so both agree on
 * what counts as an art file and how wide it is:
 *
 *   - CP437 byte -> UTF-8 conversion (gallery send path)
 *   - SAUCE record parsing (declared columns; trailing-record stripping)
 *   - width detection: SAUCE declaration wins, otherwise a cursor
 *     simulation (CUP/CUF/CUB/save-restore/CR/LF/TAB tracking the
 *     rightmost written column). Streaming byte counts are useless for
 *     ANSI art — pieces write in segments after cursor jumps, so a naive
 *     line count both over- and under-counts (ABYSS.ANS: 157 vs real 77,
 *     AKO.ANS: 53 vs real 115).
 *   - zip parsing (central directory + deflate entries) and recursive
 *     pack-dir scanning
 */
'use strict';

const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const ART_RE = /\.(ans|ice)$/i;
const ART_ALL_RE = /\.(ans|ice|nfo|txt)$/i;

/* ---- CP437 -> UTF-8 ---------------------------------------------------- */

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

const k_utf8_check = new TextDecoder('utf-8', { fatal: true });
function isUtf8(buf) {
	try {
		k_utf8_check.decode(buf);
		return true;
	} catch (e) {
		return false;
	}
}

/* ---- SAUCE ------------------------------------------------------------- */

/* Declared columns (TInfo1 at +96), or 0 if there is no record. */
function sauceWidth(buf) {
	const s = buf.toString('latin1');
	const i = s.lastIndexOf('SAUCE');
	if (i > 0 && buf.length - i === 128) {
		const ver = s.slice(i + 5, i + 7);
		if (ver === '00' || ver === '01')
			return buf.readUInt16LE(i + 96);
	}
	return 0;
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

/* ---- width detection --------------------------------------------------- */

/* Real width from a cursor simulation: emulate CUP/CUF/CUB/CUU/CUD,
 * ANSI.SYS save/restore, CR/LF/TAB and track the rightmost written
 * column of every line. See the file header for why streaming counts
 * are wrong for ANSI art. */
function estimateWidth(buf) {
	let col = 0, row = 0, maxw = 0, i = 0;
	let savedCol = 0, savedRow = 0;
	const rows = new Map(); /* row -> rightmost written col + 1 */
	const touch = () => {
		const r = rows.get(row) || 0;
		if (col + 1 > r) rows.set(row, col + 1);
	};
	while (i < buf.length) {
		const c = buf[i];
		if (c === 0x1b) {
			if (buf[i + 1] === 0x5b) {
				/* CSI: params, then the final byte */
				let j = i + 2;
				const params = [];
				let cur = -1;
				while (j < buf.length) {
					const ch = buf[j];
					if (ch >= 0x30 && ch <= 0x39) {
						if (cur < 0) cur = 0;
						cur = cur * 10 + (ch - 0x30);
					} else if (ch === 0x3b) {
						params.push(cur);
						cur = -1;
					} else break;
					j++;
				}
				if (cur >= 0) params.push(cur);
				const final = buf[j];
				const p1 = params[0] < 0 ? 1 : params[0];
				const p2 = params[1] < 0 ? 1 : params[1];
				if (final === 0x48 || final === 0x66) { row = p1 - 1; col = p2 - 1; }
				else if (final === 0x43) col += p1;      /* CUF */
				else if (final === 0x44) col = Math.max(0, col - p1); /* CUB */
				else if (final === 0x41) row -= p1;      /* CUU */
				else if (final === 0x42) row += p1;      /* CUD */
				else if (final === 0x73) { savedRow = row; savedCol = col; }
				else if (final === 0x75) { row = savedRow; col = savedCol; }
				i = j + 1;
				continue;
			}
			i += 2; /* ESC + one char (DECSC/RC, charset, keypad, ...) */
			continue;
		}
		if (c === 0x0a) { row++; col = 0; }
		else if (c === 0x0d) { col = 0; }
		else if (c === 0x09) { col += 8; }
		else if (c >= 0x20 && c !== 0x7f) { touch(); col++; }
		i++;
	}
	for (const r of rows.values())
		if (r > maxw) maxw = r;
	return maxw;
}

/* Authoritative width: SAUCE declaration when present, else the cursor
 * simulation. */
function fileWidth(buf) {
	return sauceWidth(buf) || estimateWidth(buf);
}

/* ---- zip --------------------------------------------------------------- */

/* Parse the central directory. Returns [{ name, method, data }] where
 * data is a subarray of the input buffer (not yet inflated). */
function parseZip(buf) {
	/* EOCD: PK\x05\x06, at least 22 B, within the last 64 KiB + 22 */
	let eocdOff = -1;
	for (let i = buf.length - 22; i >= Math.max(0, buf.length - 65557); i--) {
		if (buf.readUInt32LE(i) === 0x06054b50) {
			eocdOff = i;
			break;
		}
	}
	if (eocdOff < 0)
		throw new Error('not a zip archive (no end-of-central-directory)');
	const count = buf.readUInt16LE(eocdOff + 10);
	const cdOff = buf.readUInt32LE(eocdOff + 16);
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
		const lhNameLen = buf.readUInt16LE(lho + 26);
		const lhExtraLen = buf.readUInt16LE(lho + 28);
		const data = buf.subarray(lho + 30 + lhNameLen + lhExtraLen,
		                         lho + 30 + lhNameLen + lhExtraLen + csize);
		entries.push({ name, method, data });
		off += 46 + nlen + elen + clen;
	}
	return entries;
}

/* Synchronous: inflate every entry (gallery server style). Returns
 * [{ name, buf }]; entries with unsupported methods are dropped. */
function readZip(buf) {
	const entries = [];
	for (const e of parseZip(buf)) {
		if (e.method === 8)
			entries.push({ name: e.name, buf: zlib.inflateRawSync(e.data) });
		else if (e.method === 0)
			entries.push({ name: e.name, buf: e.data });
		else {
			/* old PKZIP methods (6=implode, 12=bzip2, 14=lzma, ...) are not
			 * supported by Node's zlib: skip the entry, keep the pack */
			console.log('art: skip ' + e.name + ' (zip method ' + e.method + ' unsupported)');
		}
	}
	return entries;
}

function inflateAsync(data) {
	return new Promise((resolve, reject) => {
		zlib.inflateRaw(data, (err, out) => err ? reject(err) : resolve(out));
	});
}

/* Async: read a zip and inflate every entry (entries within one pack are
 * decompressed in parallel). Returns [{ name, buf }]. */
async function readZipAsync(buf) {
	const entries = parseZip(buf);
	const tasks = entries.map(e =>
		e.method === 8 ? inflateAsync(e.data)
		: e.method === 0 ? Promise.resolve(e.data)
		: Promise.reject(new Error('zip method ' + e.method + ' unsupported')));
	const bufs = await Promise.allSettled(tasks);
	return entries.map((e, i) =>
		bufs[i].status === 'fulfilled' ? { name: e.name, buf: bufs[i].value } : null)
		.filter(e => e !== null);
}

/* ---- scanning ---------------------------------------------------------- */

/* Recursively collect art files and zip packs under the given sources
 * (directories, zips or single files). Returns [{ name, path, zip }];
 * zip names keep their <year>/ prefix for the show log. */
function collectUnits(sources, allExts) {
	const units = [];
	const re = allExts ? ART_ALL_RE : ART_RE;
	const addZip = (p, display) => {
		const m = display.match(/^(\d{4})\//);
		units.push({ name: display, path: p, zip: true, year: m ? m[1] : null });
	};
	const scanDir = (dir, root) => {
		for (const f of fs.readdirSync(dir).sort()) {
			const p = path.join(dir, f);
			if (fs.statSync(p).isDirectory()) {
				scanDir(p, root);
			} else if (re.test(f)) {
				units.push({ name: path.relative(root, p), path: p, zip: false, year: null });
			} else if (/\.zip$/i.test(f)) {
				/* display relative to the root --art dir so nested packs
				 * keep their <year>/ prefix */
				addZip(p, path.relative(root, p));
			}
		}
	};
	for (const src of sources) {
		const st = fs.statSync(src);
		if (st.isDirectory()) {
			scanDir(src, src);
		} else if (/\.zip$/i.test(src)) {
			addZip(src, path.basename(src));
		} else {
			units.push({ name: path.basename(src), path: src, zip: false, year: null });
		}
	}
	return units;
}

/* Gallery piece: strip SAUCE, check width on the raw CP437 bytes (the
 * cursor simulation counts bytes; a UTF-8 conversion would double-count
 * the high-half glyphs), then CP437->UTF-8 unless already UTF-8. Returns
 * null when the piece is wider than maxWidth. */
function convertPiece(name, buf, maxWidth) {
	buf = stripSauce(buf);
	const w = fileWidth(buf);
	let out = buf;
	if (/\.(ans|ice|txt)$/i.test(name) && !isUtf8(buf))
		out = cp437ToUtf8(buf);
	let rows = 1;
	for (const ch of out.toString('utf8')) {
		if (ch === '\n')
			rows++;
	}
	if (w > maxWidth) {
		console.log('art: skip ' + name + ' (width ' + w + ' > ' + maxWidth + ')');
		return null;
	}
	if (rows > 30)
		console.log('art: ' + name + ' is ' + rows + ' rows tall (>30: scrolls on the vterm)');
	console.log('art: loaded ' + name + ' (' + out.length + ' B, ' + w + 'x' + rows + ')');
	return { name, buf: out, w, rows };
}

module.exports = {
	ART_RE,
	ART_ALL_RE,
	CP437_TO_UTF8,
	cp437ToUtf8,
	isUtf8,
	sauceWidth,
	stripSauce,
	estimateWidth,
	fileWidth,
	parseZip,
	readZip,
	readZipAsync,
	collectUnits,
	convertPiece,
};
