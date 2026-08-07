#!/usr/bin/env node

/*
 * dsat-tool.js
 *
 * Goal:
 * - Quickly inspect the Classic Mac OS "DSAT" (Deep Shit Alert Table) resource,
 *   which controls the "Welcome to Macintosh" / SysError alert layout data
 *   (text points, icon rects, etc.).
 *
 * Why this works (high level):
 * - The ROM loads a 'DSAT' resource (GetResource('DSAT', ...)) and stores its
 *   pointer into low-memory DSAlertTab ($02BA). Then ROM routines (see P16/P17
 *   in plus-rom-listing.asm) scan the DSAT table by entry id and consume the
 *   per-entry payload (Point/Rect/text/other).
 *
 * This script provides two workflows:
 *  1) dump:
 *     - Uses hfsutils (hmount/hcopy -m) to extract a Mac file as MacBinary
 *       from an HFS disk image. We default to extracting ":System Folder:System"
 *       from macintosh/hd.img.
 *     - hfsutils stores its working directory state in $HOME (files like .hcwd),
 *       so we force HOME to a temp directory to avoid permission issues.
 *
 *  2) parse:
 *     - Parses MacBinary (data fork + resource fork).
 *     - Parses resource fork structure (type list + reference list + data blobs).
 *     - Locates a target resource (default: type=DSAT, id=0) and decodes it.
 *
 * DSAT decoding implemented here:
 * - The entry format used by the ROM lookup routine P16 is:
 *     u16 count
 *     repeat count times:
 *       s16 entryId
 *       u16 dataLen
 *       u8[dataLen] data
 * - For convenience, we try to interpret some entry payloads:
 *   - "Point + C string": [s16 v][s16 h][bytes...][0x00]
 *   - "2x long + word":  [u32][u32][u16]  (common in small 10-byte entries)
 *
 * Notes:
 * - Classic Mac uses big-endian everywhere; all integers here are parsed as BE.
 * - String decoding uses latin1 (binary-safe) because classic resources can
 *   contain MacRoman; for now we only treat ASCII-printable strings as "text".
 */

const fs = require("fs");
const os = require("os");
const path = require("path");
const { spawnSync } = require("child_process");

function u16be(buf, off) {
  return buf.readUInt16BE(off);
}

function u24be(buf, off) {
  return (buf[off] << 16) | (buf[off + 1] << 8) | buf[off + 2];
}

function u32be(buf, off) {
  return buf.readUInt32BE(off);
}

function pad128(n) {
  return (n + 127) & ~127;
}

function fourccToString(u32) {
  const b0 = (u32 >>> 24) & 0xff;
  const b1 = (u32 >>> 16) & 0xff;
  const b2 = (u32 >>> 8) & 0xff;
  const b3 = u32 & 0xff;
  return String.fromCharCode(b0, b1, b2, b3);
}

function parseMacBinary(buf) {
  // MacBinary I/II: 128-byte header, followed by Data Fork (padded to 128),
  // followed by Resource Fork (padded to 128). Length fields are BE u32.
  if (buf.length < 128) throw new Error("MacBinary: file too small");
  if (buf[0] !== 0x00) throw new Error("MacBinary: header[0] must be 0");
  const nameLen = buf[1];
  if (nameLen < 1 || nameLen > 63) throw new Error("MacBinary: invalid name length");
  if (buf[74] !== 0x00 || buf[82] !== 0x00) throw new Error("MacBinary: not a valid MB1/MB2 header");
  const dataLen = u32be(buf, 83);
  const rsrcLen = u32be(buf, 87);
  const dataOff = 128;
  const rsrcOff = dataOff + pad128(dataLen);
  if (rsrcOff + rsrcLen > buf.length) throw new Error("MacBinary: resource fork out of range");
  return {
    name: buf.slice(2, 2 + nameLen).toString("binary"),
    dataFork: buf.slice(dataOff, dataOff + dataLen),
    resourceFork: buf.slice(rsrcOff, rsrcOff + rsrcLen),
  };
}

function parseResourceFork(rsrc) {
  // Resource fork format:
  // - Header: dataOff/mapOff/dataLen/mapLen
  // - Data section: [u32 payloadLen][payload...]
  // - Map section: includes Type List + Reference List
  // This is the classic Toolbox resource format used by ResEdit/Resource Manager.
  if (rsrc.length < 16) throw new Error("ResourceFork: too small");
  const dataOff = u32be(rsrc, 0);
  const mapOff = u32be(rsrc, 4);
  const dataLen = u32be(rsrc, 8);
  const mapLen = u32be(rsrc, 12);
  if (dataOff + dataLen > rsrc.length) throw new Error("ResourceFork: data section out of range");
  if (mapOff + mapLen > rsrc.length) throw new Error("ResourceFork: map section out of range");
  if (!rsrc.slice(mapOff, mapOff + 16).equals(rsrc.slice(0, 16))) {
    throw new Error("ResourceFork: header copy mismatch");
  }
  const typeListOff = u16be(rsrc, mapOff + 24);
  const nameListOff = u16be(rsrc, mapOff + 26);
  const typeListAbs = mapOff + typeListOff;
  const nameListAbs = mapOff + nameListOff;
  if (typeListAbs + 2 > mapOff + mapLen) throw new Error("ResourceFork: type list out of map");
  if (nameListAbs > mapOff + mapLen) throw new Error("ResourceFork: name list out of map");

  const nTypes = u16be(rsrc, typeListAbs) + 1;
  const types = [];
  const typeEntryBase = typeListAbs + 2;
  for (let i = 0; i < nTypes; i++) {
    const to = typeEntryBase + i * 8;
    if (to + 8 > mapOff + mapLen) throw new Error("ResourceFork: type entry out of map");
    const type = rsrc.slice(to, to + 4).toString("binary");
    const nRes = u16be(rsrc, to + 4) + 1;
    const refListOff = u16be(rsrc, to + 6);
    types.push({ type, nRes, refListAbs: typeListAbs + refListOff });
  }

  const resources = [];
  for (const t of types) {
    for (let i = 0; i < t.nRes; i++) {
      const ro = t.refListAbs + i * 12;
      if (ro + 12 > mapOff + mapLen) throw new Error("ResourceFork: ref entry out of map");
      const id = (rsrc.readInt16BE(ro) | 0);
      const nameOff = (rsrc.readInt16BE(ro + 2) | 0);
      const attr = rsrc[ro + 4];
      const dataRel = u24be(rsrc, ro + 5);
      const handle = u32be(rsrc, ro + 8);
      const dataAbs = dataOff + dataRel;
      if (dataAbs + 4 > dataOff + dataLen) throw new Error("ResourceFork: resource data out of range");
      const payloadLen = u32be(rsrc, dataAbs);
      const payloadAbs = dataAbs + 4;
      if (payloadAbs + payloadLen > dataOff + dataLen) throw new Error("ResourceFork: payload out of range");
      let name = null;
      if (nameOff !== -1) {
        const no = nameListAbs + nameOff;
        if (no < mapOff + mapLen) {
          const nl = rsrc[no];
          if (no + 1 + nl <= mapOff + mapLen) name = rsrc.slice(no + 1, no + 1 + nl).toString("binary");
        }
      }
      resources.push({
        type: t.type,
        id,
        name,
        attr,
        handle,
        payloadOffset: payloadAbs,
        payload: rsrc.slice(payloadAbs, payloadAbs + payloadLen),
      });
    }
  }

  return { dataOff, mapOff, dataLen, mapLen, resources };
}

function decodeDSAT(buf) {
  // DSAT "table" as consumed by ROM routine P16:
  //   u16 count
  //   [ (s16 id, u16 dataLen, u8[dataLen] data) ... ] * count
  if (buf.length < 2) throw new Error("DSAT: too small");
  const n = u16be(buf, 0);
  let p = 2;
  const entries = [];
  for (let i = 0; i < n; i++) {
    if (p + 4 > buf.length) throw new Error("DSAT: truncated entry header");
    const entryHeaderOffset = p;
    const id = (buf.readInt16BE(p) | 0);
    const delta = u16be(buf, p + 2);
    p += 4;
    if (p + delta > buf.length) throw new Error("DSAT: truncated entry data");
    const dataOffset = p;
    const data = buf.slice(p, p + delta);
    entries.push({ id, delta, entryHeaderOffset, dataOffset, data });
    p += delta;
  }
  return entries;
}

function maybeDecodePointAndText(data) {
  // Heuristic: Point = { v: int16, h: int16 } followed by a 0-terminated ASCII string.
  if (data.length < 4) return null;
  const v = data.readInt16BE(0);
  const h = data.readInt16BE(2);
  if (v < -64 || v > 1024 || h < -64 || h > 2048) return null;
  const rest = data.slice(4);
  let end = rest.indexOf(0);
  if (end === -1) end = Math.min(rest.length, 63);
  const raw = rest.slice(0, end);
  const text = raw.toString("latin1");
  if (!text) return { v, h, text: "" };
  const printable = [...text].every((c) => c >= " " && c <= "~");
  if (!printable) return { v, h, text: "" };
  return { v, h, text };
}

function maybeDecodeTwoLongsPlusWord(data) {
  // Some DSAT entries are just small fixed-size records: 4 + 4 + 2 = 10 bytes.
  if (data.length !== 10) return null;
  const a = u32be(data, 0);
  const b = u32be(data, 4);
  const w = u16be(data, 8);
  return { a: `0x${a.toString(16).padStart(8, "0")}`, b: `0x${b.toString(16).padStart(8, "0")}`, w };
}

function maybeDecodeRect(data) {
  if (data.length < 8) return null;
  const top = data.readInt16BE(0);
  const left = data.readInt16BE(2);
  const bottom = data.readInt16BE(4);
  const right = data.readInt16BE(6);
  const ok =
    top >= -64 &&
    top <= 2048 &&
    left >= -64 &&
    left <= 2048 &&
    bottom >= -64 &&
    bottom <= 2048 &&
    right >= -64 &&
    right <= 2048 &&
    top < bottom &&
    left < right;
  if (!ok) return null;
  return { top, left, bottom, right };
}

function runHfsutilsExtractMacBinary(imagePath, hfsPath, outPath) {
  // hfsutils keeps per-user state under $HOME (e.g. .hcwd). In sandboxed/CI
  // environments $HOME can be read-only, so force it to a temp directory.
  const env = { ...process.env, HOME: path.join(os.tmpdir(), `hfsutils-${process.pid}`) };
  fs.mkdirSync(env.HOME, { recursive: true });

  let r = spawnSync("hmount", [imagePath], { env, stdio: "pipe" });
  if (r.status !== 0) throw new Error(`hmount failed: ${r.stderr.toString() || r.stdout.toString()}`);

  r = spawnSync("hcopy", ["-m", hfsPath, outPath], { env, stdio: "pipe" });
  spawnSync("humount", [], { env, stdio: "ignore" });
  if (r.status !== 0) throw new Error(`hcopy failed: ${r.stderr.toString() || r.stdout.toString()}`);
}

function runHfsutilsInstallMacBinary(imagePath, macBinaryPath, hfsDestPath) {
  const env = { ...process.env, HOME: path.join(os.tmpdir(), `hfsutils-${process.pid}`) };
  fs.mkdirSync(env.HOME, { recursive: true });

  let r = spawnSync("hmount", [imagePath], { env, stdio: "pipe" });
  if (r.status !== 0) throw new Error(`hmount failed: ${r.stderr.toString() || r.stdout.toString()}`);

  r = spawnSync("hcopy", ["-m", macBinaryPath, hfsDestPath], { env, stdio: "pipe" });
  spawnSync("humount", [], { env, stdio: "ignore" });
  if (r.status !== 0) throw new Error(`hcopy failed: ${r.stderr.toString() || r.stdout.toString()}`);
}

function usage() {
  const s = [
    "Usage:",
    "  node scripts/dsat-tool.js dump [--image <hd.img>] [--file <hfs-path>]",
    "  node scripts/dsat-tool.js parse --macbinary <file> [--type DSAT] [--id 0]",
    "  node scripts/dsat-tool.js patch-welcome --macbinary <file> [--out <file>] (--dh <n> | --width <w>) [--also-rect]",
    "  node scripts/dsat-tool.js patch-icons --macbinary <file> [--out <file>] (--dh <n> | --width <w>) [--rect <t,l,b,r>]",
    "  node scripts/dsat-tool.js install --image <hd.img> --macbinary <file> [--dest <hfs-path>]",
    "",
    "Examples:",
    '  node scripts/dsat-tool.js dump --image macintosh/hd.img --file :\"System Folder\":System',
    "  node scripts/dsat-tool.js parse --macbinary /tmp/System.bin",
    "  node scripts/dsat-tool.js patch-welcome --macbinary /tmp/System.bin --width 640",
    "  node scripts/dsat-tool.js patch-icons --macbinary /tmp/System.bin --dh 128 --rect 89,56,121,88",
    "  node scripts/dsat-tool.js install --image macintosh/hd.img --macbinary /tmp/System.patched.bin --dest :\"System Folder\":System",
  ];
  console.error(s.join("\n"));
}

function main() {
  const argv = process.argv.slice(2);
  const cmd = argv[0];
  if (!cmd) {
    usage();
    process.exit(2);
  }

  if (cmd === "dump") {
    let image = "macintosh/hd.img";
    let file = ":System Folder:System";
    for (let i = 1; i < argv.length; i++) {
      if (argv[i] === "--image" && argv[i + 1]) {
        image = argv[++i];
      } else if (argv[i] === "--file" && argv[i + 1]) {
        file = argv[++i];
      } else {
        usage();
        process.exit(2);
      }
    }
    const out = path.join(os.tmpdir(), `System-${process.pid}.bin`);
    runHfsutilsExtractMacBinary(image, file, out);
    console.log(out);
    return;
  }

  if (cmd === "parse") {
    let macbin = null;
    let type = "DSAT";
    let id = 0;
    for (let i = 1; i < argv.length; i++) {
      if (argv[i] === "--macbinary" && argv[i + 1]) {
        macbin = argv[++i];
      } else if (argv[i] === "--type" && argv[i + 1]) {
        type = argv[++i];
      } else if (argv[i] === "--id" && argv[i + 1]) {
        id = parseInt(argv[++i], 10);
      } else {
        usage();
        process.exit(2);
      }
    }
    if (!macbin) {
      usage();
      process.exit(2);
    }
    const mb = parseMacBinary(fs.readFileSync(macbin));
    const rf = parseResourceFork(mb.resourceFork);
    const res = rf.resources.find((r) => r.type === type && r.id === id);
    if (!res) {
      console.error(`Not found: type=${type} id=${id}`);
      process.exit(1);
    }
    if (type === "DSAT") {
      const entries = decodeDSAT(res.payload).map((e) => {
        const pt = maybeDecodePointAndText(e.data);
        const ptrs = maybeDecodeTwoLongsPlusWord(e.data);
        return {
          id: e.id,
          delta: e.delta,
          point: pt ? { h: pt.h, v: pt.v } : null,
          text: pt ? pt.text : null,
          ptrs: ptrs,
          dataHex: e.data.toString("hex"),
        };
      });
      console.log(JSON.stringify({ macbinary: mb.name, resource: { type, id }, entries }, null, 2));
    } else {
      console.log(res.payload.toString("hex"));
    }
    return;
  }

  if (cmd === "patch-welcome") {
    let macbin = null;
    let outPath = null;
    let dh = null;
    let width = null;
    let alsoRect = false;

    for (let i = 1; i < argv.length; i++) {
      if (argv[i] === "--macbinary" && argv[i + 1]) {
        macbin = argv[++i];
      } else if (argv[i] === "--out" && argv[i + 1]) {
        outPath = argv[++i];
      } else if (argv[i] === "--dh" && argv[i + 1]) {
        dh = parseInt(argv[++i], 10);
      } else if (argv[i] === "--width" && argv[i + 1]) {
        width = parseInt(argv[++i], 10);
      } else if (argv[i] === "--also-rect") {
        alsoRect = true;
      } else {
        usage();
        process.exit(2);
      }
    }

    if (!macbin) {
      usage();
      process.exit(2);
    }
    if (dh === null) {
      if (width === null) {
        usage();
        process.exit(2);
      }
      dh = Math.floor((width - 512) / 2);
    }
    if (!Number.isFinite(dh)) {
      throw new Error("patch-welcome: invalid dh");
    }
    if (!outPath) outPath = macbin.replace(/(\.bin)?$/i, ".patched.bin");

    const fileBuf = fs.readFileSync(macbin);
    const mb = parseMacBinary(fileBuf);
    const rf = parseResourceFork(mb.resourceFork);
    const dsat = rf.resources.find((r) => r.type === "DSAT" && r.id === 0);
    if (!dsat) throw new Error("patch-welcome: DSAT id=0 not found");

    const dsatPayload = Buffer.from(dsat.payload);
    const entries = decodeDSAT(dsatPayload);
    let changedPoints = 0;
    let changedRects = 0;

    for (const e of entries) {
      const pt = maybeDecodePointAndText(e.data);
      if (pt && pt.text && pt.text.includes("Welcome to Macintosh")) {
        const oldH = pt.h;
        const newH = oldH + dh;
        dsatPayload.writeInt16BE(newH, e.dataOffset + 2);
        changedPoints++;
      }
      if (alsoRect) {
        const r = maybeDecodeRect(e.data);
        if (r) {
          dsatPayload.writeInt16BE(r.left + dh, e.dataOffset + 2);
          dsatPayload.writeInt16BE(r.right + dh, e.dataOffset + 6);
          changedRects++;
        }
      }
    }

    const patchedRsrc = Buffer.from(mb.resourceFork);
    dsatPayload.copy(patchedRsrc, dsat.payloadOffset);

    const dataLen = mb.dataFork.length;
    const rsrcLen = patchedRsrc.length;
    const outBuf = Buffer.alloc(128 + pad128(dataLen) + pad128(rsrcLen));
    fileBuf.copy(outBuf, 0, 0, 128);
    outBuf.writeUInt32BE(dataLen, 83);
    outBuf.writeUInt32BE(rsrcLen, 87);
    mb.dataFork.copy(outBuf, 128);
    patchedRsrc.copy(outBuf, 128 + pad128(dataLen));

    fs.writeFileSync(outPath, outBuf);
    console.log(JSON.stringify({ out: outPath, dh, changedPoints, changedRects }, null, 2));
    return;
  }

  if (cmd === "patch-icons") {
    let macbin = null;
    let outPath = null;
    let dh = null;
    let width = null;
    let rectFilter = null;

    for (let i = 1; i < argv.length; i++) {
      if (argv[i] === "--macbinary" && argv[i + 1]) {
        macbin = argv[++i];
      } else if (argv[i] === "--out" && argv[i + 1]) {
        outPath = argv[++i];
      } else if (argv[i] === "--dh" && argv[i + 1]) {
        dh = parseInt(argv[++i], 10);
      } else if (argv[i] === "--width" && argv[i + 1]) {
        width = parseInt(argv[++i], 10);
      } else if (argv[i] === "--rect" && argv[i + 1]) {
        const parts = argv[++i].split(",").map((x) => parseInt(x.trim(), 10));
        if (parts.length !== 4 || parts.some((x) => !Number.isFinite(x))) throw new Error("patch-icons: invalid --rect");
        rectFilter = { top: parts[0], left: parts[1], bottom: parts[2], right: parts[3] };
      } else {
        usage();
        process.exit(2);
      }
    }

    if (!macbin) {
      usage();
      process.exit(2);
    }
    if (dh === null) {
      if (width === null) {
        usage();
        process.exit(2);
      }
      dh = Math.floor((width - 512) / 2);
    }
    if (!Number.isFinite(dh)) {
      throw new Error("patch-icons: invalid dh");
    }
    if (!outPath) outPath = macbin.replace(/(\.bin)?$/i, ".icons.patched.bin");
    if (!rectFilter) rectFilter = { top: 89, left: 56, bottom: 121, right: 88 };

    const fileBuf = fs.readFileSync(macbin);
    const mb = parseMacBinary(fileBuf);
    const rf = parseResourceFork(mb.resourceFork);
    const dsat = rf.resources.find((r) => r.type === "DSAT" && r.id === 0);
    if (!dsat) throw new Error("patch-icons: DSAT id=0 not found");

    const dsatPayload = Buffer.from(dsat.payload);
    const entries = decodeDSAT(dsatPayload);

    let changedIcons = 0;
    const changed = [];

    for (const e of entries) {
      if (e.delta !== 136) continue;
      const r = maybeDecodeRect(e.data);
      if (!r) continue;
      if (
        r.top !== rectFilter.top ||
        r.left !== rectFilter.left ||
        r.bottom !== rectFilter.bottom ||
        r.right !== rectFilter.right
      ) {
        continue;
      }

      dsatPayload.writeInt16BE(r.left + dh, e.dataOffset + 2);
      dsatPayload.writeInt16BE(r.right + dh, e.dataOffset + 6);
      changedIcons++;
      changed.push({ id: e.id, before: r, after: { top: r.top, left: r.left + dh, bottom: r.bottom, right: r.right + dh } });
    }

    const patchedRsrc = Buffer.from(mb.resourceFork);
    dsatPayload.copy(patchedRsrc, dsat.payloadOffset);

    const dataLen = mb.dataFork.length;
    const rsrcLen = patchedRsrc.length;
    const outBuf = Buffer.alloc(128 + pad128(dataLen) + pad128(rsrcLen));
    fileBuf.copy(outBuf, 0, 0, 128);
    outBuf.writeUInt32BE(dataLen, 83);
    outBuf.writeUInt32BE(rsrcLen, 87);
    mb.dataFork.copy(outBuf, 128);
    patchedRsrc.copy(outBuf, 128 + pad128(dataLen));

    fs.writeFileSync(outPath, outBuf);
    console.log(JSON.stringify({ out: outPath, dh, rectFilter, changedIcons, changed }, null, 2));
    return;
  }

  if (cmd === "install") {
    let image = null;
    let macbin = null;
    let dest = ':"System Folder":System';

    for (let i = 1; i < argv.length; i++) {
      if (argv[i] === "--image" && argv[i + 1]) {
        image = argv[++i];
      } else if (argv[i] === "--macbinary" && argv[i + 1]) {
        macbin = argv[++i];
      } else if (argv[i] === "--dest" && argv[i + 1]) {
        dest = argv[++i];
      } else {
        usage();
        process.exit(2);
      }
    }
    if (!image || !macbin) {
      usage();
      process.exit(2);
    }
    runHfsutilsInstallMacBinary(image, macbin, dest);
    console.log(JSON.stringify({ image, dest, macbinary: macbin }, null, 2));
    return;
  }

  usage();
  process.exit(2);
}

main();
