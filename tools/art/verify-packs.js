#!/usr/bin/env node
/* 核对 16colo.rs 合集解压完整性：对比 mega zip 中央目录里的 .zip 条目
 * 与目标目录的实际文件（支持 ZIP64）。用法:
 *   node tools/art/verify-packs.js [mega.zip] [目录]
 * 默认: ~/16colo-packs.zip  vs  scripts/art/packs
 */
const fs = require('fs');
const path = require('path');

const zipPath = process.argv[2] || path.join(process.env.HOME, '16colo-packs.zip');
const destDir = process.argv[3] || path.join(__dirname, '..', '..', 'scripts', 'art', 'packs');

/* 读取 ZIP64-aware 的中央目录 */
function readCentralDir(zipPath) {
	const f = fs.openSync(zipPath, 'r');
	const size = fs.fstatSync(f).size;
	const tail = Buffer.alloc(Math.min(size, 1024 * 1024));
	fs.readSync(f, tail, 0, tail.length, size - tail.length);
	let eocdRel = -1;
	for (let i = tail.length - 22; i >= 0; i--)
		if (tail.readUInt32LE(i) === 0x06054b50) { eocdRel = i; break; }
	if (eocdRel < 0) throw new Error('not a zip archive');
	let cdOff = tail.readUInt32LE(eocdRel + 16);
	let cdSize = tail.readUInt32LE(eocdRel + 12);
	const count = tail.readUInt16LE(eocdRel + 10);
	if (cdOff === 0xFFFFFFFF || cdSize === 0xFFFFFFFF) {
		const locRel = eocdRel - 20;
		if (tail.readUInt32LE(locRel) !== 0x07064b50) throw new Error('ZIP64 locator not found');
		const z64Off = Number(tail.readBigUInt64LE(locRel + 8));
		const z64Rel = z64Off - (size - tail.length);
		if (z64Rel < 0 || z64Rel + 56 > tail.length) throw new Error('ZIP64 EOCD outside tail');
		cdSize = Number(tail.readBigUInt64LE(z64Rel + 40));
		cdOff = Number(tail.readBigUInt64LE(z64Rel + 48));
	}
	const cd = Buffer.alloc(cdSize);
	fs.readSync(f, cd, 0, cdSize, cdOff);
	fs.closeSync(f);
	return { cd, count };
}

const { cd, count } = readCentralDir(zipPath);
const expected = {};   // 年份 -> 数量
const missing = [];
let off = 0;
for (let i = 0; i < count; i++) {
	const nlen = cd.readUInt16LE(off + 28), elen = cd.readUInt16LE(off + 30), clen = cd.readUInt16LE(off + 32);
	const name = cd.toString('latin1', off + 46, off + 46 + nlen);
	const m = name.match(/^16colo-packs\/(\d+)\/(.+\.zip)$/i);
	if (m) {
		expected[m[1]] = (expected[m[1]] || 0) + 1;
		const p = path.join(destDir, m[1], m[2]);
		if (!fs.existsSync(p))
			missing.push(name);
	}
	off += 46 + nlen + elen + clen;
}
let total = 0;
for (const y of Object.keys(expected).sort()) {
	const n = expected[y];
	total += n;
	console.log(y + ': ' + n + (missing.some(m => m.includes('/' + y + '/')) ? '  ← 有缺失' : ' ✓'));
}
console.log('---');
console.log('mega zip .zip 条目: ' + total);
console.log('目标目录实际: ' + (fs.existsSync(destDir) ?
	fs.readdirSync(destDir, { recursive: true }).filter(f => /\.zip$/i.test(f)).length : 0) + ' 个');
if (missing.length) {
	console.log('缺失 ' + missing.length + ' 个:');
	missing.forEach(n => console.log('  ' + n));
	process.exit(1);
}
console.log('完整 ✓');
