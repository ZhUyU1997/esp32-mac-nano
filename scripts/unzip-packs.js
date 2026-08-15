#!/usr/bin/env node
/*
 * unzip-packs.js — 16colo.rs 整站合集解压 + pack 修复（JS 版，替代 unzip-packs.sh）。
 *
 * 流式读取 mega zip（ZIP64，9GB 不全读内存），解出 16colo-packs/<年份>/*.zip
 * 到 scripts/art/packs/<年份>/，然后对每个 pack 验证并修复两类问题：
 *
 *   1. 截断（缺中央目录/EOCD）——下载中断的包：本地文件头链表完整，
 *      按 streaming 格式流式恢复全部条目，重新打包成完整 zip 覆盖；
 *   2. 含不支持的压缩方法（method 6=implode、1=shrink、14=LZMA）——
 *      用系统 unzip 解到临时目录（unzip 支持这些老格式），重新打包覆盖。
 *
 * 用法:
 *   node scripts/unzip-packs.js                  # 默认 ~/16colo-packs.zip
 *   node scripts/unzip-packs.js /path/x.zip      # 指定 mega zip
 *   node scripts/unzip-packs.js -n               # 增量：跳过已存在的 pack
 *   node scripts/unzip-packs.js --repair-only    # 不重新解压，只修复现有 packs
 */
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');
const { execFileSync, spawnSync } = require('child_process');
const artlib = require('./art-lib');

const DEST = path.join(__dirname, 'art', 'packs');

/* ---- ZIP64-aware central directory (mirrors verify-packs.js) ----------- */

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
	let count = tail.readUInt16LE(eocdRel + 10);
	if (cdOff === 0xFFFFFFFF || cdSize === 0xFFFFFFFF || count === 0xFFFF) {
		const locRel = eocdRel - 20;
		if (tail.readUInt32LE(locRel) !== 0x07064b50)
			throw new Error('ZIP64 locator not found');
		const z64Off = Number(tail.readBigUInt64LE(locRel + 8));
		const z64Rel = z64Off - (size - tail.length);
		if (z64Rel < 0 || z64Rel + 56 > tail.length)
			throw new Error('ZIP64 EOCD outside tail');
		count = Number(tail.readBigUInt64LE(z64Rel + 32));
		cdSize = Number(tail.readBigUInt64LE(z64Rel + 40));
		cdOff = Number(tail.readBigUInt64LE(z64Rel + 48));
	}
	const cd = Buffer.alloc(cdSize);
	fs.readSync(f, cd, 0, cdSize, cdOff);
	fs.closeSync(f);
	return { cd, count };
}

/* Parse the central directory entries (only the fields we need). Resolves
 * ZIP64 extensions (usize/csize/lho = 0xFFFFFFFF live in the 0x0001 extra
 * field; the 9 GB mega zip uses them for every entry). */
function cdEntries(cd, count) {
	const entries = [];
	let off = 0;
	for (let i = 0; i < count; i++) {
		if (cd.readUInt32LE(off) !== 0x02014b50)
			break;
		const method = cd.readUInt16LE(off + 10);
		const nlen = cd.readUInt16LE(off + 28);
		const elen = cd.readUInt16LE(off + 30);
		const clen = cd.readUInt16LE(off + 32);
		let csize = cd.readUInt32LE(off + 20);
		let lho = cd.readUInt32LE(off + 42);
		const name = cd.toString('latin1', off + 46, off + 46 + nlen);
		if (csize === 0xFFFFFFFF || lho === 0xFFFFFFFF) {
			const extra = off + 46 + nlen;
			let x = extra;
			while (x + 4 <= extra + elen) {
				const hid = cd.readUInt16LE(x);
				const hsize = cd.readUInt16LE(x + 2);
				if (hid === 0x0001) {
					/* ZIP64 extra order: usize, csize, lho, disk */
					let p = x + 4;
					const u64 = () => { const v = Number(cd.readBigUInt64LE(p)); p += 8; return v; };
					if (cd.readUInt32LE(off + 24) === 0xFFFFFFFF) u64();
					if (csize === 0xFFFFFFFF) csize = u64();
					if (lho === 0xFFFFFFFF) lho = u64();
					break;
				}
				x += 4 + hsize;
			}
		}
		entries.push({ name, method, csize, lho });
		off += 46 + nlen + elen + clen;
	}
	return entries;
}

/* ---- extraction from the mega zip (streamed, no full-file read) -------- */

function readEntry(fd, lho, method, csize) {
	const lh = Buffer.alloc(30);
	fs.readSync(fd, lh, 0, 30, lho);
	if (lh.readUInt32LE(0) !== 0x04034b50)
		throw new Error('bad local header at ' + lho);
	const nlen = lh.readUInt16LE(26);
	const elen = lh.readUInt16LE(28);
	const data = Buffer.alloc(csize);
	fs.readSync(fd, data, 0, csize, lho + 30 + nlen + elen);
	if (method === 8)
		return zlib.inflateRawSync(data);
	if (method === 0)
		return data;
	throw new Error('mega entry method ' + method + ' unsupported');
}

/* ---- repack: write every entry back as a fresh deflated zip ------------ */

function writeZip(entries, outPath) {
	const chunks = [];
	const cd = [];
	let off = 0;
	for (const e of entries) {
		const name = Buffer.from(e.name, 'latin1');
		const data = zlib.deflateRawSync(e.buf, { level: 9 });
		const lh = Buffer.alloc(30);
		lh.writeUInt32LE(0x04034b50, 0);  /* local file header */
		lh.writeUInt16LE(20, 4);          /* version needed */
		lh.writeUInt16LE(0, 6);           /* flags (names are latin1) */
		lh.writeUInt16LE(8, 8);           /* method: deflate */
		lh.writeUInt16LE(0, 10);          /* mod time */
		lh.writeUInt16LE(0, 12);          /* mod date */
		lh.writeUInt32LE(0, 14);          /* crc (unused) */
		lh.writeUInt32LE(data.length, 18);   /* csize */
		lh.writeUInt32LE(e.buf.length, 22);  /* usize */
		lh.writeUInt16LE(name.length, 26);   /* name len */
		lh.writeUInt16LE(0, 28);             /* extra len */
		chunks.push(lh, name, data);
		const ce = Buffer.alloc(46);
		ce.writeUInt32LE(0x02014b50, 0);  /* central directory entry */
		ce.writeUInt16LE(20, 4);          /* version made by */
		ce.writeUInt16LE(20, 6);          /* version needed */
		ce.writeUInt16LE(0, 8);           /* flags */
		ce.writeUInt16LE(8, 10);          /* method */
		ce.writeUInt16LE(0, 12);          /* mod time */
		ce.writeUInt16LE(0, 14);          /* mod date */
		ce.writeUInt32LE(0, 16);          /* crc */
		ce.writeUInt32LE(data.length, 20);   /* csize */
		ce.writeUInt32LE(e.buf.length, 24);  /* usize */
		ce.writeUInt16LE(name.length, 28);   /* name len */
		ce.writeUInt16LE(0, 30);             /* extra len */
		ce.writeUInt16LE(0, 32);             /* comment len */
		ce.writeUInt16LE(0, 34);             /* disk start */
		ce.writeUInt16LE(0, 36);             /* internal attrs */
		ce.writeUInt32LE(0, 38);             /* external attrs */
		ce.writeUInt32LE(off, 42);           /* local header offset */
		cd.push(ce, name);
		off += 30 + name.length + data.length;
	}
	const cdSize = cd.reduce((s, c) => s + c.length, 0);
	const eocd = Buffer.alloc(22);
	eocd.writeUInt32LE(0x06054b50, 0);
	eocd.writeUInt16LE(0, 4);             /* disk number */
	eocd.writeUInt16LE(0, 6);             /* disk with CD */
	eocd.writeUInt16LE(entries.length, 8);
	eocd.writeUInt16LE(entries.length, 10);
	eocd.writeUInt32LE(cdSize, 12);
	eocd.writeUInt32LE(off, 16);
	fs.writeFileSync(outPath, Buffer.concat([...chunks, ...cd, eocd]));
}

/* ---- repair ------------------------------------------------------------ */

/* Stream-recover a truncated pack (no central directory): walk the local
 * file header chain, decompress what is readable, repack as a full zip. */
function streamRepack(zipPath) {
	const d = fs.readFileSync(zipPath);
	const files = [];
	let off = 0;
	let recovered = 0, failed = 0;
	while (off + 30 <= d.length) {
		if (d.readUInt32LE(off) !== 0x04034b50) {
			if (d.readUInt32LE(off) === 0x08074b50) off += 16; /* data descriptor */
			else break;
			continue;
		}
		const method = d.readUInt16LE(off + 8);
		const csize = d.readUInt32LE(off + 18);
		const nlen = d.readUInt16LE(off + 26);
		const elen = d.readUInt16LE(off + 28);
		const name = d.toString('latin1', off + 30, off + 30 + nlen);
		const data = d.subarray(off + 30 + nlen + elen, off + 30 + nlen + elen + csize);
		if (data.length < csize) {
			failed++;
			break; /* truncated mid-entry */
		}
		try {
			files.push({ name, buf: method === 0 ? Buffer.from(data) : zlib.inflateRawSync(data) });
			recovered++;
		} catch (e) {
			failed++;
		}
		off += 30 + nlen + elen + csize;
	}
	if (!files.length)
		return { status: 'unrepairable', reason: 'no readable entries' };
	writeZip(files, zipPath + '.fixed');
	return { status: 'fixed', recovered, failed, total: files.length };
}

/* Repack a pack whose entries use old compression methods via system
 * unzip (supports implode/shrink/LZMA), then rewrite as a deflate zip. */
function unzipRepack(zipPath) {
	const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'pack-fix-'));
	try {
		execFileSync('unzip', ['-o', '-q', zipPath, '-d', tmp], { stdio: 'pipe' });
	} catch (e) {
		return { status: 'unrepairable', reason: 'unzip failed: ' + e.message };
	}
	const files = [];
	(function walk(dir, prefix) {
		for (const f of fs.readdirSync(dir)) {
			const p = path.join(dir, f);
			const rel = prefix ? prefix + '/' + f : f;
			if (fs.statSync(p).isDirectory())
				walk(p, rel);
			else
				files.push({ name: rel, buf: fs.readFileSync(p) });
		}
	})(tmp, '');
	fs.rmSync(tmp, { recursive: true, force: true });
	writeZip(files, zipPath + '.fixed');
	return { status: 'fixed', total: files.length };
}

/* Extract to a temp dir and collect the files (shared by the unzip and
 * python fallbacks). */
function collectDir(tmp) {
	const files = [];
	(function walk(dir, prefix) {
		for (const f of fs.readdirSync(dir)) {
			const p = path.join(dir, f);
			const rel = prefix ? prefix + '/' + f : f;
			if (fs.statSync(p).isDirectory())
				walk(p, rel);
			else
				files.push({ name: rel, buf: fs.readFileSync(p) });
		}
	})(tmp, '');
	return files;
}

/* Repack a pack with LZMA (zip method 14) entries via python3's zipfile,
 * which can decode them (Node zlib and Info-ZIP unzip cannot). */
function lzmaRepack(zipPath) {
	const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'pack-fix-'));
	const script =
	    'import sys, zipfile, os\n' +
	    'z = zipfile.ZipFile(sys.argv[1])\n' +
	    'for i in z.infolist():\n' +
	    '    p = os.path.join(sys.argv[2], i.filename)\n' +
	    '    os.makedirs(os.path.dirname(p), exist_ok=True)\n' +
	    '    with open(p, "wb") as f: f.write(z.read(i))\n';
	try {
		execFileSync('python3', ['-c', script, zipPath, tmp], { stdio: 'pipe' });
	} catch (e) {
		fs.rmSync(tmp, { recursive: true, force: true });
		return { status: 'unrepairable', reason: 'python lzma failed: ' + e.message };
	}
	const files = collectDir(tmp);
	fs.rmSync(tmp, { recursive: true, force: true });
	writeZip(files, zipPath + '.fixed');
	return { status: 'fixed', total: files.length };
}

/* Validate one pack and repair it if needed. Returns a status object. */
function repairPack(zipPath) {
	let buf;
	try {
		buf = fs.readFileSync(zipPath);
	} catch (e) {
		return { status: 'unrepairable', reason: 'unreadable: ' + e.message };
	}
	let entries;
	try {
		entries = artlib.parseZip(buf);
	} catch (e) {
		/* not a parseable zip: stream-recover if it looks like one, else
		 * move the junk file (0-byte, PHP, ...) out of the packs */
		if (buf.length < 4 || buf.toString('latin1', 0, 2) !== 'PK') {
			const quar = path.join(__dirname, 'art', 'quarantine');
			fs.mkdirSync(quar, { recursive: true });
			const dest = path.join(quar, path.basename(zipPath));
			if (!fs.existsSync(dest))
				fs.renameSync(zipPath, dest);
			else
				fs.rmSync(zipPath, { force: true });
			return { status: 'quarantined', reason: 'not a zip (' + buf.length + ' B)' };
		}
		const r = streamRepack(zipPath);
		return Object.assign({ pack: path.basename(zipPath), issue: 'truncated' }, r);
	}
	const unsupported = entries.filter(e => e.method !== 0 && e.method !== 8);
	if (unsupported.length === 0)
		return { pack: path.basename(zipPath), status: 'ok' };
	const methods = [...new Set(unsupported.map(e => e.method))];
	/* LZMA (14) needs python3; implode/shrink (6/1) need Info-ZIP unzip */
	const r = methods.every(m => m === 14)
		? lzmaRepack(zipPath)
		: unzipRepack(zipPath);
	if (r.status === 'unrepairable' && methods.includes(14)) {
		const r2 = lzmaRepack(zipPath);
		return Object.assign({ pack: path.basename(zipPath), issue: 'methods ' + methods.join(',') }, r2);
	}
	return Object.assign(
		{ pack: path.basename(zipPath), issue: 'methods ' + methods.join(',') }, r);
}

/* Extract a .rar pack from the mega zip, decompress it with unrar-free
 * (the mega entry itself is never modified) and rewrite it as a .zip in
 * the packs tree, next to the original name. */
function rarToZip(fd, e) {
	const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'rar-'));
	try {
		const rarPath = path.join(tmp, 'in.rar');
		fs.writeFileSync(rarPath, readEntry(fd, e.lho, e.method, e.csize));
		const outDir = path.join(tmp, 'out');
		fs.mkdirSync(outDir);
		/* 7z handles both RAR4 and RAR5. Some RAR5 entries use methods
		 * 7z 16.02 cannot decode (Unsupported Method) — the rest of the
		 * archive is still extracted, so accept whatever came out. */
		const zr = spawnSync('7z', ['x', '-y', '-o' + outDir, rarPath], { stdio: 'pipe' });
		const files = collectDir(outDir);
		if (!files.length)
			throw new Error('7z extracted nothing: ' + (zr.stderr || zr.error || '').toString().slice(0, 120));

		const dest = path.join(DEST, e.year, e.baseName.replace(/\.rar$/i, '.zip'));
		fs.mkdirSync(path.dirname(dest), { recursive: true });
		writeZip(files, dest);
		return { status: 'fixed', total: files.length, dest };
	} finally {
		fs.rmSync(tmp, { recursive: true, force: true });
	}
}

/* ---- main -------------------------------------------------------------- */

let zipPath = path.join(process.env.HOME, '16colo-packs.zip');
let incremental = false;
let repairOnly = false;
for (const a of process.argv.slice(2)) {
	if (a === '-n') incremental = true;
	else if (a === '--repair-only') repairOnly = true;
	else zipPath = a;
}

if (!repairOnly) {
	if (!fs.existsSync(zipPath)) {
		console.error('找不到 ' + zipPath);
		process.exit(1);
	}
	console.log('解压 ' + zipPath + ' -> ' + DEST + '（模式: ' +
	            (incremental ? '增量' : '覆盖') + '）');
	fs.mkdirSync(DEST, { recursive: true });

	const { cd, count } = readCentralDir(zipPath);
	const entries = cdEntries(cd, count);
	const packs = [];
	let nRar = 0;
	for (const e of entries) {
		const m = e.name.match(/^16colo-packs\/(\d+)\/(.+\.(zip|rar))$/i);
		if (!m) continue;
		e.year = m[1];
		e.baseName = m[2];
		e.isRar = /\.rar$/i.test(m[2]);
		if (e.isRar) nRar++;
		packs.push(e);
	}
	console.log('mega zip 含 ' + packs.length + ' 个压缩包（' + nRar + ' 个 rar 将转为 zip）');

	const fd = fs.openSync(zipPath, 'r');
	let extracted = 0, skipped = 0;
	for (const e of packs) {
		if (e.isRar) {
			const dest = path.join(DEST, e.year, e.baseName.replace(/\.rar$/i, '.zip'));
			if (incremental && fs.existsSync(dest)) {
				skipped++;
				continue;
			}
			try {
				const r = rarToZip(fd, e);
				if (r.status === 'fixed') {
					extracted++;
					console.log('  rar→zip ' + e.year + '/' + e.baseName + '（' + r.total + ' 条）');
				} else {
					console.log('  rar 转换失败 ' + e.name + ': ' + r.reason);
				}
			} catch (err) {
				console.log('  rar 转换失败 ' + e.name + ': ' + err.message);
			}
			continue;
		}
		const rel = e.name.replace(/^16colo-packs\//, '');
		const dest = path.join(DEST, rel);
		if (incremental && fs.existsSync(dest)) {
			skipped++;
			continue;
		}
		try {
			const buf = readEntry(fd, e.lho, e.method, e.csize);
			fs.mkdirSync(path.dirname(dest), { recursive: true });
			fs.writeFileSync(dest, buf);
			extracted++;
		} catch (err) {
			console.log('  解压失败 ' + rel + ': ' + err.message);
		}
	}
	fs.closeSync(fd);
	console.log('解压完成: ' + extracted + ' 个（跳过 ' + skipped + '）');
}

/* repair pass: validate every pack in place, rewrite broken ones */
console.log('验证并修复 packs...');
const packs = [];
(function scan(p) {
	for (const f of fs.readdirSync(p)) {
		const fp = path.join(p, f);
		if (fs.statSync(fp).isDirectory()) scan(fp);
		else if (/\.zip$/i.test(f)) packs.push(fp);
	}
})(DEST);

const counts = { ok: 0, fixed: 0, quarantined: 0, unrepairable: 0 };
for (const p of packs) {
	const r = repairPack(p);
	if (r.status === 'ok') {
		counts.ok++;
	} else if (r.status === 'fixed') {
		counts.fixed++;
		fs.renameSync(p + '.fixed', p);
		console.log('  修复 ' + path.basename(p) + '（' + r.issue + '）: ' +
		            (r.recovered !== undefined ? '恢复 ' + r.recovered + '/' + (r.recovered + r.failed) + ' 条' :
		             '重打包 ' + r.total + ' 条'));
	} else if (r.status === 'quarantined') {
		counts.quarantined++;
		console.log('  隔离 ' + path.basename(p) + '（' + r.reason + '）');
	} else {
		counts.unrepairable++;
		console.log('  无法修复 ' + path.basename(p) + ': ' + r.reason);
	}
}
console.log('修复完成: ' + counts.ok + ' 健康, ' + counts.fixed + ' 已修复, ' +
            counts.quarantined + ' 已隔离, ' + counts.unrepairable + ' 无法修复（' +
            packs.length + ' 个 pack）');
