#!/usr/bin/env node
/*
 * diff-image.js — three-panel ANSI render comparison image (pure JS).
 *
 * Replaces the embedded python/PIL block that compare.js used to run:
 *   left   = ansilove PNG
 *   middle = vterm-ans PNG
 *   right  = per-cell diff map (red = differing cell, grey = row-count gap)
 * plus row-number gutters and a header line per panel.
 *
 * Usage:
 *   node tools/art/diff-image.js <ans.png> <vtm.png> <map.json> <out.png>
 *       [--title-middle STR] [--scale N]
 *
 * map.json: flat array of booleans (cell diff map, min-rows * cols),
 * written by compare.js / test-art.js from diff-lib's cellDiff().
 */
const fs = require('fs');
const path = require('path');
const sharp = require('sharp');
const { decodePngRgb } = require('./lib/diff-lib');

const CELL_W = 8, CELL_H = 16; /* 640x512 / 80x32 */

function buildDiffMapPng(ansW, ansH, mapJson, rowsCommon, cols) {
	const flat = JSON.parse(fs.readFileSync(mapJson, 'utf8'));
	const cells = new Uint8Array(rowsCommon * cols);
	const minRows = Math.min(flat.length / cols, rowsCommon);
	if (Number.isInteger(minRows) && minRows > 0) {
		for (let i = 0; i < minRows * cols; i++) cells[i] = flat[i] ? 1 : 0;
	}
	/* grey marker for rows the taller renderer has beyond the shorter one */
	for (let r = Math.floor(minRows); r < rowsCommon; r++)
		for (let c = 0; c < cols; c++) cells[r * cols + c] = 2;

	const raw = Buffer.alloc(rowsCommon * CELL_H * cols * CELL_W * 3);
	const red = [255, 60, 60], grey = [150, 150, 150];
	for (let r = 0; r < rowsCommon; r++) {
		for (let c = 0; c < cols; c++) {
			const col = cells[r * cols + c] === 1 ? red : cells[r * cols + c] === 2 ? grey : null;
			if (!col) continue;
			for (let y = 0; y < CELL_H; y++) {
				const base = ((r * CELL_H + y) * cols * CELL_W + c * CELL_W) * 3;
				for (let x = 0; x < CELL_W; x++) {
					raw[base + x * 3] = col[0];
					raw[base + x * 3 + 1] = col[1];
					raw[base + x * 3 + 2] = col[2];
				}
			}
		}
	}
	return raw;
}

function svgHeader(width, lines) {
	/* lines: array of { x, y, text } with y at the text baseline */
	const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;');
	let t = '';
	for (const l of lines) t += `<text x="${l.x}" y="${l.y}" font-size="13" fill="#333">${esc(l.text)}</text>\n`;
	return Buffer.from(`<svg width="${width}" height="24">${t}</svg>`);
}

function svgGutter(texts, scale) {
	/* row-number labels; texts: array of { x, row } (row = cell row) */
	let t = '';
	for (const g of texts) {
		const y = 24 + Math.round(g.row * CELL_H * scale) + Math.round(8 * scale);
		t += `<text x="${g.x}" y="${y}" font-size="10" fill="#777">${g.row}</text>\n`;
	}
	return Buffer.from(`<svg width="30" height="${24 + 16}">${t}</svg>`);
}

async function renderDiffImage(opts) {
	const { ansPng, vtmPng, mapJson, out, titleMiddle, scale = 1 } = opts;
	const a = decodePngRgb(fs.readFileSync(ansPng));
	const v = decodePngRgb(fs.readFileSync(vtmPng));
	const cols = a.w / CELL_W;
	const rowsA = a.h / CELL_H, rowsV = v.h / CELL_H;
	const common = Math.max(rowsA, rowsV);
	const minRows = Math.min(rowsA, rowsV);
	const H = common * CELL_H, W = cols * CELL_W;
	const sH = Math.round(H * scale), sW = Math.round(W * scale);

	/* diff map as raw RGB (full height; grey marks the row-count gap) */
	const dmRaw = buildDiffMapPng(a.w, a.h, mapJson, common, cols);
	const dm = await sharp(dmRaw, { raw: { width: W, height: H, channels: 3 } })
		.resize(sW, sH, { kernel: 'nearest' }).png().toBuffer();

	/* panel images, cropped to common height then scaled */
	const mkPanel = (img, h) => {
		const pH = Math.min(h, H);
		const sPH = Math.round(pH * scale);
		return sharp(img, { raw: { width: a.w, height: h, channels: 3 } })
			.extract({ left: 0, top: 0, width: a.w, height: pH })
			.resize(sW, sPH, { kernel: 'nearest' }).png().toBuffer();
	};
	const ta = await mkPanel(a.data, a.h);
	const tv = await mkPanel(v.data, v.h);

	const bar = 30, gapX = 40;
	const leftX = bar, midX = bar + sW + gapX, dmX = midX + sW + gapX;
	const Wtotal = dmX + sW, Htotal = sH + 24 + 16;

	const canvas = sharp({ create: { width: Wtotal, height: Htotal, channels: 3, background: { r: 255, g: 255, b: 255 } } });
	const composites = [
		{ input: ta, left: leftX, top: 24 },
		{ input: tv, left: midX, top: 24 },
		{ input: dm, left: dmX, top: 24 },
	];
	/* row-number gutters: one SVG strip per panel, composited on top */
	for (const [x] of [[leftX], [midX]]) {
		const labels = [];
		for (let r = 0; r <= common + 4; r += 5) labels.push({ row: r });
		const svg = svgGutter(labels, scale);
		composites.push({ input: svg, left: x - bar, top: 0 });
	}
	const header = svgHeader(Wtotal, [
		{ x: leftX + 4, y: 16, text: `ansilove ${a.w}x${a.h} (${rowsA} rows)` },
		{ x: midX + 4, y: 16, text: `${titleMiddle || 'vterm-ans'} ${v.w}x${v.h} (${rowsV} rows)` },
		{ x: dmX + 4, y: 16, text: `diff map: ${opts.diff}/${opts.total} (${(opts.rate * 100).toFixed(1)}%)` },
	]);
	composites.push({ input: header, left: 0, top: 0 });

	await canvas.composite(composites).png().toFile(out);
	return { rowsA, rowsV, Wtotal, Htotal };
}

if (require.main === module) {
	const [,, ansPng, vtmPng, mapJson, out] = process.argv;
	const i = process.argv.indexOf('--title-middle');
	const titleMiddle = i > 0 ? process.argv[i + 1] : undefined;
	const s = process.argv.indexOf('--scale');
	const scale = s > 0 ? Number(process.argv[s + 1]) : 1;
	renderDiffImage({ ansPng, vtmPng, mapJson, out, titleMiddle, scale })
		.then(() => console.log('diff-image:', out))
		.catch((e) => { console.error(e.message); process.exit(1); });
}

module.exports = { renderDiffImage };
