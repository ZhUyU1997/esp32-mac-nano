#!/usr/bin/env bash
# Build Saved HD v2.hda from Saved HD.hda
# Keeps only Mac Plus 4 MB-compatible software.
set -euo pipefail

SRC="macintosh/disk/Saved HD.hda"
DST="macintosh/disk/hd0.img"
VOL_MB=170
FORCE=0

while [ $# -gt 0 ]; do
  case "$1" in -f) FORCE=1;; -s) VOL_MB="$2"; shift;; *) echo "Usage: $0 [-f] [-s MB]"; exit 0;; esac
  shift
done
[ -f "$SRC" ] || { echo "Missing $SRC"; exit 1; }
[ -f "$DST" ] && [ "$FORCE" -eq 0 ] && { echo "$DST exists, use -f"; exit 1; }

OV=macintosh/disk/.oldv.img
NV=macintosh/disk/.newv.img

rm -f "$DST" "$OV" "$NV"
cp "$SRC" "$DST"
echo "[1/5] Copied source"

dd if="$DST" of="$OV" bs=512 skip=96 status=none
echo "[2/5] Volume extracted"

dd if=/dev/zero of="$NV" bs=1M count=$VOL_MB status=none
export HOME=/tmp
hformat -l "Saved HD" "$NV" 2>/dev/null
echo "[3/5] Empty ${VOL_MB}MB volume created"

echo "[4/5] Copying directories..."
python3 << PYEOF
import subprocess, os, shutil

T = 'macintosh/disk/.pycp'
for d in ['/a','/b','/t']: os.makedirs(T+d, exist_ok=True)

EA = os.environ.copy(); EA['HOME'] = T+'/a'
EB = os.environ.copy(); EB['HOME'] = T+'/b'
OV = 'macintosh/disk/.oldv.img'
NV = 'macintosh/disk/.newv.img'
M = {b'Jan',b'Feb',b'Mar',b'Apr',b'May',b'Jun',b'Jul',b'Aug',b'Sep',b'Oct',b'Nov',b'Dec'}

def hr(a, e):
    r = subprocess.run(a, capture_output=True, env=e)
    if r.returncode != 0:
        err = r.stderr.decode(errors='replace').strip()
        if err:
            c = a[0].decode('ascii','replace') if isinstance(a[0],bytes) else a[0]
            args = ' '.join(x.decode('macroman','replace') if isinstance(x,bytes) else str(x) for x in a[1:])
            print(f'  ERR: {c} {args[:60]} -> {err[:80]}')
    return r

def cp(sp, dp):
    r = hr(['hls','-la',sp], EA)
    for raw in r.stdout.split(b'\n'):
        L = raw.strip()
        if not L or L.startswith(b'Volume') or b'bytes free' in L: continue
        P = L.split()
        if len(P) < 6: continue
        f = P[0]
        for i, x in enumerate(P):
            if x in M:
                # Find name by locating year token (preserves trailing spaces)
                year = P[i+2] if i+2 < len(P) else None
                if year:
                    month_pos = raw.find(P[i])
                    if month_pos >= 0:
                        year_pos = raw.find(year, month_pos)
                        if year_pos >= 0:
                            nb = raw[year_pos + len(year) + 1:].rstrip(b'\r')
                        else:
                            nb = b' '.join(P[i+3:]).strip()
                    else:
                        nb = b' '.join(P[i+3:]).strip()
                else:
                    nb = b''
                fs = sp + b':' + nb
                fd = dp + b':' + nb
                if f.startswith(b'd') or f.startswith(b'D'):
                    hr(['hmkdir',fd], EB)
                    cp(fs, fd)
                elif f.startswith(b'f') or f.startswith(b'fi'):
                    hr(['hcopy','-m',fs,T+'/t/'], EA)
                    for fn in os.listdir(T+'/t/'):
                        hr(['hcopy','-m',T+'/t/'+fn,fd], EB)
                        os.remove(T+'/t/'+fn)
                break

hr(['hmount',OV], EA)
hr(['hmount',NV], EB)

DIRS = [
    b':Developer:ADB Parser',b':Developer:BBEdit 2.1.3',
    b':Developer:BlueSCSI Toolbox',b':Developer:Debugger!',
    b':Developer:Gestalt!',b':Developer:machid',
    b':Developer:MacsBug 6.6.3',b':Developer:Memory Mapper',
    b':Developer:Microsoft QuickBASIC',b':Developer:Mini vMac Extras',
    b":Developer:Programmer's Key",b':Developer:ResEdit',
    b':Developer:Resorcerer 1.2.5',b':Developer:scuzEMU',
    b':Developer:Swatch',b':Developer:THINK C',b':Developer:THINK Pascal',b':Developer:ZoneRanger',
    b':Games:3Wiz!',b':Games:Another World',b':Games:Battle Chess',
    b':Games:Blobbo Lite',b':Games:Bolo',b':Games:ChainShot!',
    b':Games:Civilization',b':Games:Continuum',b':Games:Dark Castle',
    b':Games:Glider',b':Games:Hellcats Over the Pacific',
    b':Games:Indy and The Last Crusade',b':Games:Lemmings',
    b':Games:Missile Command',b':Games:NS-SHAFT',b':Games:ok - A sheep game',
    b':Games:Pararena',b':Games:Pipe Dream',b':Games:Prince of Persia',
    b':Games:Risk',b':Games:Scarab of Ra',b':Games:Shufflepuck Caf\x8e',b':Games:SimCity',b':Games:Snood',
    b':Games:Sokoban',b':Games:Solarian II',b':Games:Spectre',
    b':Games:Starbound',b':Games:Strategic Conquest 3.0',
    b':Games:Strategic Conquest 4.0.1',b':Games:Strategic Conquest Plus',
    b':Games:StuntCopter',b':Games:Tetris',b':Games:The Oregon Trail',
    b':Games:The Secret of Monkey Island',
    b':Graphics:Adobe Photoshop 1.0',b':Graphics:Claris CAD',
    b':Graphics:GraphicConverter',b":Graphics:Kai's Power Tips",
    b':Graphics:Kid Pix',b':Graphics:MacDraft 1.21',b':Graphics:MacDraw 1.9',
    b':Graphics:MacPaint 1.5',b':Graphics:MacPaint 2.0',
    b':Graphics:MacPaint Intro Graphics',b':Graphics:UltraPaint',
    b':Productivity:ClarisWorks',b':Productivity:FileMaker II',
    b':Productivity:MacWrite 2.2',b':Productivity:Microsoft Excel 4.0',
    b':Productivity:Microsoft Word',b':Productivity:Nisus Writer',
    b':Productivity:Reflex',
    b':Utilities:About',b':Utilities:BinHex 4.0',b':Utilities:Compact Pro',
    b':Utilities:Disk Copy',b':Utilities:Disk First Aid',
    b':Utilities:More About This Macintosh',b':Utilities:PCalc 1.0.2',
    b':Utilities:SmartLaunch',b':Utilities:Speedometer',
    b':Utilities:TattleTech',b':Utilities:TechTool 1.0.4',
]

total = len(DIRS)
# Create top-level dirs first
for d in DIRS:
    top = b':' + d.split(b':')[1]
    hr(['hmkdir',top], EB)

for i, d in enumerate(DIRS):
    hr(['hmkdir',d], EB)
    cp(d, d)
    print(f"\r  [{i+1}/{total}] {d.decode('macroman',errors='replace')[:50]}", end='')

hr(['humount'],EA); hr(['humount'],EB)
print(f"  {len(DIRS)} dirs copied")
shutil.rmtree(T, ignore_errors=True)
print("  done")
PYEOF

./tools/djjr create mac-device -sM $VOL_MB --scsi-id 6 "$DST" 2>/dev/null
dd if="$NV" of="$DST" bs=512 seek=96 conv=notrunc status=none
rm -f "$OV" "$NV"
echo "[5/5] Device image created"

echo ""
echo "=== Result ==="
ls -lh "$DST"
hmount "$DST" 2>/dev/null
hvol 2>/dev/null | grep free
hls -la ":" 2>/dev/null | grep "^[dfi]"
humount 2>/dev/null
