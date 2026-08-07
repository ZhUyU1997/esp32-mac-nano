#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMG_REL="macintosh/hd10.img"
IMG="${ROOT_DIR}/${IMG_REL}"
DSAT_TOOL="${ROOT_DIR}/scripts/dsat-tool.js"
DEST=':System Folder:System'

DH="${DH:-128}"
WIDTH="${WIDTH:-}"
RECT="${RECT:-89,56,121,88}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dh)
      DH="$2"
      shift 2
      ;;
    --width)
      WIDTH="$2"
      shift 2
      ;;
    --rect)
      RECT="$2"
      shift 2
      ;;
    --image)
      IMG="$2"
      shift 2
      ;;
    --dest)
      DEST="$2"
      shift 2
      ;;
    *)
      echo "Usage: $0 [--dh N | --width W] [--rect t,l,b,r] [--image path] [--dest hfsPath]" >&2
      echo "Env: DH, WIDTH, RECT" >&2
      exit 2
      ;;
  esac
done

if [[ -n "${WIDTH}" ]]; then
  DH="$(( (WIDTH - 512) / 2 ))"
fi

cd "${ROOT_DIR}"

git restore --source=HEAD -- "${IMG_REL}"

SYSBIN="$(node "${DSAT_TOOL}" dump --image "${IMG}" --file "${DEST}")"
node "${DSAT_TOOL}" patch-welcome --macbinary "${SYSBIN}" --dh "${DH}"
node "${DSAT_TOOL}" install --image "${IMG}" --macbinary "${SYSBIN%.bin}.patched.bin" --dest "${DEST}"

SYSBIN2="$(node "${DSAT_TOOL}" dump --image "${IMG}" --file "${DEST}")"
node "${DSAT_TOOL}" patch-icons --macbinary "${SYSBIN2}" --dh "${DH}" --rect "${RECT}"
node "${DSAT_TOOL}" install --image "${IMG}" --macbinary "${SYSBIN2%.bin}.icons.patched.bin" --dest "${DEST}"

SYSBIN3="$(node "${DSAT_TOOL}" dump --image "${IMG}" --file "${DEST}")"
node "${DSAT_TOOL}" parse --macbinary "${SYSBIN3}" > /tmp/dsat.hd10.current.json
node - <<'NODE'
const fs = require("fs");
const j = JSON.parse(fs.readFileSync("/tmp/dsat.hd10.current.json", "utf8"));
const welcome = j.entries.find((e) => e.text && e.text.includes("Welcome to Macintosh"));
const icons = j.entries
  .filter((e) => e.delta === 136)
  .map((e) => {
    const hex = e.dataHex.slice(0, 16);
    const s16 = (x) => ((x & 0x8000) ? x - 0x10000 : x);
    const top = s16(parseInt(hex.slice(0, 4), 16));
    const left = s16(parseInt(hex.slice(4, 8), 16));
    const bottom = s16(parseInt(hex.slice(8, 12), 16));
    const right = s16(parseInt(hex.slice(12, 16), 16));
    return { id: e.id, top, left, bottom, right };
  });
console.log(JSON.stringify({ dh: process.env.DH || null, welcome: welcome ? { id: welcome.id, point: welcome.point } : null, iconEntries: icons }, null, 2));
NODE
