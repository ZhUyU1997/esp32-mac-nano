'use strict';

/*
 * diff-lib.js — the single implementation of the vterm-ans vs
 * libansilove cell-diff comparison. Used by both test-art.js (batch)
 * and compare.js one (single file) so every tool reports identical
 * numbers. Also decodes the PNGs the two renderers write.
 *
 * Algorithm: decode both PNGs to RGB, quantise every pixel to the
 * nearest of the 16 standard indices using each renderer's own palette,
 * aggregate each 8x16 cell to its two most frequent indices (fg/bg),
 * normalise bright/dark away (idx % 8) and compare the sets. Row
 * alignment is 1:1 (no tolerance): with the ED 2 cursor-home fix both
 * renderers draw the same row grid.
 */
const zlib = require('zlib');
let sharp = null;
try { sharp = require('sharp'); } catch (e) { /* optional accel */ }

/* CGA / ANSI.SYS palette. libvterm's pen.c now renders the same table,
 * so both sides quantise with the same colours. */
const VTERM_PAL = [
	[0, 0, 0], [170, 0, 0], [0, 170, 0], [170, 85, 0],
	[0, 0, 170], [170, 0, 170], [0, 170, 170], [170, 170, 170],
	[85, 85, 85], [255, 85, 85], [85, 255, 85], [255, 255, 85],
	[85, 85, 255], [255, 85, 255], [85, 255, 255], [255, 255, 255],
];
const ANSI_PAL = VTERM_PAL;

/* Decode the RGB PNGs vterm-ans / ansilove write. vterm-ans writes
 * 8-bit RGB (colour type 2); ansilove writes palette PNGs (colour type
 * 3, 4- or 8-bit indices). Handles all five scanline filters. Returns
 * { w, h, data } (RGB bytes). */
function decodePngRgb(p) {
	const w = p.readUInt32BE(16);
	const h = p.readUInt32BE(20);
	const bitDepth = p[24];
	const colourType = p[25];
	if (colourType !== 2 && colourType !== 3)
		throw new Error('png colour type ' + colourType + ' unsupported');
	let off = 8;
	const ihdrLen = p.readUInt32BE(off);
	off += 12 + ihdrLen;
	const idat = [];
	let palette = null;
	while (off + 8 <= p.length) {
		const len = p.readUInt32BE(off);
		const type = p.toString('latin1', off + 4, off + 8);
		if (type === 'IDAT') idat.push(p.subarray(off + 8, off + 8 + len));
		else if (type === 'PLTE') palette = p.subarray(off + 8, off + 8 + len);
		else if (type === 'IEND') break;
		off += 12 + len;
	}
	if (colourType === 3 && !palette)
		throw new Error('png palette missing');
	const raw = zlib.inflateSync(Buffer.concat(idat));
	const bpp = colourType === 2 ? 3 : 1; /* filter predictor bytes/pixel */
	const stride = colourType === 2 ? w * 3 :
	               bitDepth === 8 ? w : Math.ceil(w / 2);
	const img = Buffer.alloc(w * h * 3);
	let src = 0;
	let prev = Buffer.alloc(stride);
	for (let y = 0; y < h; y++) {
		const filter = raw[src++];
		const out = Buffer.alloc(stride);
		for (let x = 0; x < stride; x++) {
			const a = x >= bpp ? out[x - bpp] : 0;
			const b = prev[x];
			const c = x >= bpp ? prev[x - bpp] : 0;
			let v = raw[src + x];
			switch (filter) {
			case 0: break;
			case 1: v = (v + a) & 0xff; break;
			case 2: v = (v + b) & 0xff; break;
			case 3: v = (v + ((a + b) >> 1)) & 0xff; break;
			case 4: {
				const pv = a + b - c;
				const pa = Math.abs(pv - a), pb = Math.abs(pv - b), pc = Math.abs(pv - c);
				const pr = (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
				v = (v + pr) & 0xff;
				break;
			}
			default: throw new Error('png filter ' + filter + ' unsupported');
			}
			out[x] = v;
		}
		/* expand to RGB */
		if (colourType === 2) {
			out.copy(img, y * w * 3);
		} else if (bitDepth === 8) {
			for (let x = 0; x < w; x++) {
				const pi = out[x] * 3;
				img.set(palette.subarray(pi, pi + 3), y * w * 3 + x * 3);
			}
		} else { /* bitDepth 4: two 4-bit indices per byte */
			for (let x = 0; x < w; x++) {
				const byte = out[x >> 1];
				const idx = (x & 1) ? byte & 0x0f : byte >> 4;
				const pi = idx * 3;
				img.set(palette.subarray(pi, pi + 3), y * w * 3 + x * 3);
			}
		}
		prev = out;
		src += stride;
	}
	return { w, h, data: img };
}

/* async decode with libvips (sharp) when available: ~5x faster than the
 * pure-JS decodePngRgb and, being async, does not block the event loop
 * of the worker pool. Falls back to the sync decoder otherwise. */
async function decodePngRgbAsync(p) {
	if (sharp) {
		try {
			const { data, info } = await sharp(p).raw().toBuffer({ resolveWithObject: true });
			return { w: info.width, h: info.height, data };
		} catch (e) { /* malformed PNG: fall through to the JS decoder */ }
	}
	return decodePngRgb(p);
}

function nearestIdx(r, g, b, pal) {
	let bi = 0, bd = Infinity;
	for (let i = 0; i < 16; i++) {
		const d = Math.abs(r - pal[i][0]) + Math.abs(g - pal[i][1]) +
		          Math.abs(b - pal[i][2]);
		if (d < bd) { bd = d; bi = i; }
	}
	return bi;
}

/* quantiser with a per-image colour cache: rendered ANSI pixels are a
 * small set of repeated colours, so the 16-way distance search runs once
 * per unique RGB instead of once per pixel (cellDiff hot path) */
function makeQuantizer(pal) {
	const cache = new Map();
	return (r, g, b) => {
		const key = (r << 16) | (g << 8) | b;
		let idx = cache.get(key);
		if (idx === undefined) {
			idx = nearestIdx(r, g, b, pal);
			cache.set(key, idx);
		}
		return idx;
	};
}

/* the two most frequent palette indices in an 8x16 cell (= fg and bg) */
function cellTopIdx(img, q, x, y) {
	const freq = new Int16Array(16);
	for (let yy = 0; yy < 16; yy++)
		for (let xx = 0; xx < 8; xx++) {
			const i = ((y + yy) * img.w + (x + xx)) * 3;
			freq[q(img.data[i], img.data[i + 1], img.data[i + 2])]++;
		}
	const order = Array.from({ length: 16 }, (_, i) => i)
		              .sort((a, b) => freq[b] - freq[a] || a - b);
	return [order[0], order[1]];
}

/* returns { cols:[vterm-ans, ansilove], rows:[...], diff, total, rate,
 * bright, map? } where rate uses normalised (bright/dark-agnostic)
 * indices and bright counts cells whose bright/dark pairing differs.
 * Column counts must match (a layout drift). Rows are compared 1:1
 * (no alignment tolerance): with the ED 2 cursor-home fix both
 * renderers draw the same row grid, so any misalignment is a real
 * rendering difference. Trailing rows are compared at the min.
 * With `withMap`, map is a Uint8Array(rows*cols): 1 = cell differs. */
function cellDiff(v, a, withMap) {
	const cV = Math.floor(v.w / 8), rV = Math.floor(v.h / 16);
	const cA = Math.floor(a.w / 8), rA = Math.floor(a.h / 16);
	/* Column counts may differ when we follow a SAUCE-declared width
	 * that libansilove failed to detect (e.g. 79 vs 80). Compare the
	 * overlapping columns instead of failing outright: if the content
	 * matches, the width difference is a libansilove detection limit;
	 * if it does not, it is a real rendering difference. */
	const cols = Math.min(cV, cA);
	const rows = Math.min(rV, rA);
	if (rows <= 0 || cols <= 0)
		return { cols: [cV, cA], rows: [rV, rA], diff: 0, total: 0, rate: 1, bright: 0 };
	const norm = (i) => i & 7;
	let diff = 0, bright = 0;
	const map = withMap ? new Uint8Array(rows * cols) : null;
	const qv = makeQuantizer(VTERM_PAL), qa = makeQuantizer(ANSI_PAL);
	const vd = v.data, ad = a.data, vw = v.w, aw = a.w;
	for (let r = 0; r < rows; r++)
		for (let c = 0; c < cols; c++) {
			const x0 = c * 8, y0 = r * 16;
			/* fast reject: cells whose 8x16 raw RGB is byte-identical are
			 * the same by construction — skip quantisation entirely (most
			 * cells of a well-aligned pair are identical; bench: 84ms -> 8ms) */
			let sameBytes = true;
			for (let y = 0; y < 16 && sameBytes; y++) {
				const vo = (y0 + y) * vw + x0, ao = (y0 + y) * aw + x0;
				for (let x = 0; x < 8; x++) {
					const vi = (vo + x) * 3, ai = (ao + x) * 3;
					if (vd[vi] !== ad[ai] || vd[vi + 1] !== ad[ai + 1] || vd[vi + 2] !== ad[ai + 2]) {
						sameBytes = false;
						break;
					}
				}
			}
			if (sameBytes)
				continue;
			const vt = cellTopIdx(v, qv, x0, y0);
			const at = cellTopIdx(a, qa, x0, y0);
			const same = (norm(vt[0]) === norm(at[0]) && norm(vt[1]) === norm(at[1])) ||
			             (norm(vt[0]) === norm(at[1]) && norm(vt[1]) === norm(at[0]));
			if (!same) {
				diff++;
				if (map) map[r * cV + c] = 1;
				continue;
			}
			if ((vt[0] >= 8) !== (at[0] >= 8) || (vt[1] >= 8) !== (at[1] >= 8))
				bright++;
		}
	const res = { cols: [cV, cA], rows: [rV, rA], diff, total: cols * rows,
	              rate: diff / (cols * rows), bright };
	if (map) res.map = map;
	return res;
}

module.exports = { VTERM_PAL, ANSI_PAL, decodePngRgb, decodePngRgbAsync, nearestIdx, cellTopIdx, cellDiff };
