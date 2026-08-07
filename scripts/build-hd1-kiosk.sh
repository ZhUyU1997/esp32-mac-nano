#!/usr/bin/env bash
# Build hd1.img from HD30_512 - Mac Plus Kiosk.hda
# Removes System folder and Desktop cache files.
# Uses Python with MacRoman bytes for special characters in filenames.
set -euo pipefail

SRC="macintosh/disk/HD30_512 - Mac Plus Kiosk.hda"
DST="macintosh/disk/hd1.img"
TMP_VOL=macintosh/disk/.kiosk_tmp.img
FORCE=0
VOL_MB=20

while [ $# -gt 0 ]; do
  case "$1" in -f) FORCE=1;; -s) VOL_MB="$2"; shift;; *) echo "Usage: $0 [-f] [-s MB]"; exit 0;; esac
  shift
done
[ -f "$SRC" ] || { echo "Missing $SRC"; exit 1; }
[ -f "$DST" ] && [ "$FORCE" -eq 0 ] && { echo "$DST exists, use -f"; exit 1; }

rm -f "$DST" "$TMP_VOL"

echo "[1/4] Extracting HFS volume..."
dd if="$SRC" of="$TMP_VOL" bs=512 skip=96 status=none

echo "[2/4] Removing System and Desktop..."
python3 << PYEOF
import subprocess, os, shutil

T = 'macintosh/disk/.kiosk_pycp'
for d in ['/a','/b','/t']: os.makedirs(T+d, exist_ok=True)

EA = os.environ.copy(); EA['HOME'] = T+'/a'
M = {b'Jan',b'Feb',b'Mar',b'Apr',b'May',b'Jun',b'Jul',b'Aug',b'Sep',b'Oct',b'Nov',b'Dec'}
TV = 'macintosh/disk/.kiosk_tmp.img'

def hr(a, e):
    r = subprocess.run(a, capture_output=True, env=e)
    if r.returncode != 0:
        err = r.stderr.decode(errors='replace').strip()
        if err:
            c = a[0].decode('ascii','replace') if isinstance(a[0],bytes) else a[0]
            args = ' '.join(x.decode('macroman','replace') if isinstance(x,bytes) else str(x) for x in a[1:])
            print(f'  ERR: {c} {args[:60]} -> {err[:80]}')
    return r

def rm_r(path):
    r = hr(['hls','-la',path], EA)
    for raw in r.stdout.split(b'\n'):
        L = raw.strip()
        if not L or L.startswith(b'Volume') or b'bytes free' in L: continue
        P = L.split()
        if len(P) < 6: continue
        f = P[0]
        for i, x in enumerate(P):
            if x in M:
                year = P[i+2] if i+2 < len(P) else b''
                if year:
                    mp = raw.find(P[i])
                    if mp >= 0:
                        yp = raw.find(year, mp)
                        nb = raw[yp + len(year) + 1:].rstrip(b'\r ') if yp >= 0 else b' '.join(P[i+3:]).strip()
                    else:
                        nb = b' '.join(P[i+3:]).strip()
                else:
                    nb = b''
                full = path + b':' + nb
                if f.startswith(b'd') or f.startswith(b'D'):
                    rm_r(full)
                elif f.startswith(b'f') or f.startswith(b'fi'):
                    hr(['hdel',full], EA)
                break
    hr(['hrmdir',path], EA)

hr(['hmount', TV], EA)
rm_r(b':System')
for f in [':Desktop', ':Desktop DB', ':Desktop DF']:
    hr(['hdel', f], EA)
hr(['humount'], EA)
shutil.rmtree(T, ignore_errors=True)
print("  done")
PYEOF

echo "[3/4] Creating device image..."
./tools/djjr create mac-device -sM $VOL_MB --scsi-id 6 "$DST" 2>/dev/null

echo "[4/4] Injecting volume..."
dd if="$TMP_VOL" of="$DST" bs=512 seek=96 conv=notrunc status=none
rm -f "$TMP_VOL"

echo ""
echo "=== Result ==="
ls -lh "$DST"
./tools/djjr analyze "$DST" 2>/dev/null
hmount "$DST" 2>/dev/null
hvol 2>/dev/null | grep free
hls -la ":" 2>/dev/null | grep "^[dfi]"
humount 2>/dev/null
