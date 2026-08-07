#!/usr/bin/env python3
"""Verify Saved HD v2 has all files from kept directories (no accidental omissions)."""
import subprocess, os, sys, shutil, tempfile

TMP = tempfile.mkdtemp(prefix='verify_')
EA = os.environ.copy(); EA['HOME'] = TMP+'/a'; os.makedirs(TMP+'/a')
EB = os.environ.copy(); EB['HOME'] = TMP+'/b'; os.makedirs(TMP+'/b')

MONTHS = {b'Jan',b'Feb',b'Mar',b'Apr',b'May',b'Jun',b'Jul',b'Aug',b'Sep',b'Oct',b'Nov',b'Dec'}

def hr(a, e):
    return subprocess.run(a, capture_output=True, env=e)

def file_list(vol, env, root=b':'):
    """Return dict of path->size for all files under root."""
    result = {}
    def scan(path):
        r = hr(['hls','-la',path], env)
        if r.returncode != 0: return
        for raw in r.stdout.split(b'\n'):
            L = raw.strip()
            if not L or L.startswith(b'Volume') or b'bytes free' in L: continue
            P = L.split()
            if len(P) < 6: continue
            f = P[0]
            for i, x in enumerate(P):
                if x in MONTHS:
                    year = P[i+2]  # year or HH:MM token
                    if year:
                        pos = raw.find(P[i])
                        if pos >= 0:
                            yp = raw.find(year, pos)
                            if yp >= 0:
                                nb = raw[yp + len(year) + 1:].rstrip(b'\r ')
                            else:
                                nb = b' '.join(P[i+3:]).strip()
                        else:
                            nb = b' '.join(P[i+3:]).strip()
                    else:
                        nb = b''
                    full = path + b':' + nb
                    if f.startswith(b'd') or f.startswith(b'D'):
                        scan(full)
                    elif f.startswith(b'f') or f.startswith(b'fi'):
                        try:
                            sz = int(P[2]) + int(P[3])
                            result[full] = sz
                        except: pass
                    break
    scan(root)
    return result

KEEP = [
    b':Developer:ADB Parser', b':Developer:BBEdit 2.1.3',
    b':Developer:BlueSCSI Toolbox', b':Developer:Debugger!',
    b':Developer:Gestalt!', b':Developer:machid',
    b':Developer:MacsBug 6.6.3', b':Developer:Memory Mapper',
    b':Developer:Microsoft QuickBASIC', b':Developer:Mini vMac Extras',
    b":Developer:Programmer's Key", b':Developer:ResEdit',
    b':Developer:Resorcerer 1.2.5', b':Developer:scuzEMU',
    b':Developer:Swatch', b':Developer:THINK C',
    b':Developer:THINK Pascal', b':Developer:ZoneRanger',
    b':Games:3Wiz!', b':Games:Another World', b':Games:Battle Chess',
    b':Games:Blobbo Lite', b':Games:Bolo', b':Games:ChainShot!',
    b':Games:Civilization', b':Games:Continuum', b':Games:Dark Castle',
    b':Games:Glider', b':Games:Hellcats Over the Pacific',
    b':Games:Indy and The Last Crusade', b':Games:Lemmings',
    b':Games:Missile Command', b':Games:NS-SHAFT', b':Games:ok - A sheep game',
    b':Games:Pararena', b':Games:Pipe Dream', b':Games:Prince of Persia',
    b':Games:Risk', b':Games:Scarab of Ra', b':Games:Shufflepuck Caf\x8e',
    b':Games:SimCity', b':Games:Snood', b':Games:Sokoban',
    b':Games:Solarian II', b':Games:Spectre', b':Games:Starbound',
    b':Games:Strategic Conquest 3.0', b':Games:Strategic Conquest 4.0.1',
    b':Games:Strategic Conquest Plus', b':Games:StuntCopter', b':Games:Tetris',
    b':Games:The Oregon Trail', b':Games:The Secret of Monkey Island',
    b':Graphics:Adobe Photoshop 1.0', b':Graphics:Claris CAD',
    b':Graphics:GraphicConverter', b":Graphics:Kai's Power Tips",
    b':Graphics:Kid Pix', b':Graphics:MacDraft 1.21', b':Graphics:MacDraw 1.9',
    b':Graphics:MacPaint 1.5', b':Graphics:MacPaint 2.0',
    b':Graphics:MacPaint Intro Graphics', b':Graphics:UltraPaint',
    b':Productivity:ClarisWorks', b':Productivity:FileMaker II',
    b':Productivity:MacWrite 2.2', b':Productivity:Microsoft Excel 4.0',
    b':Productivity:Microsoft Word', b':Productivity:Nisus Writer',
    b':Productivity:Reflex',
    b':Utilities:About', b':Utilities:BinHex 4.0', b':Utilities:Compact Pro',
    b':Utilities:Disk Copy', b':Utilities:Disk First Aid',
    b':Utilities:More About This Macintosh', b':Utilities:PCalc 1.0.2',
    b':Utilities:SmartLaunch', b':Utilities:Speedometer',
    b':Utilities:TattleTech', b':Utilities:TechTool 1.0.4',
]

src_vol = sys.argv[1] if len(sys.argv) > 1 else "macintosh/disk/Saved HD.hda"
dst_vol = sys.argv[2] if len(sys.argv) > 2 else "macintosh/disk/Saved HD v2.hda"

# Extract volumes
subprocess.run(['dd', 'if='+src_vol, 'of='+TMP+'/s.img', 'bs=512', 'skip=96', 'count=2252800'], capture_output=True)
subprocess.run(['dd', 'if='+dst_vol, 'of='+TMP+'/d.img', 'bs=512', 'skip=96'], capture_output=True)

hr(['hmount', TMP+'/s.img'], EA)
hr(['hmount', TMP+'/d.img'], EB)

print(f"Scanning {len(KEEP)} kept directories...")
missing = {}
present = True
total_orig = 0
total_miss = 0

for d in KEEP:
    name = d.decode('macroman', errors='replace')
    print(f"\r  {name[:55]}", end='')
    
    orig_files = file_list('src', EA, d)
    dest_files = file_list('dst', EB, d)
    total_orig += len(orig_files)
    
    for path, sz in orig_files.items():
        # Construct the same path key for dest
        if path not in dest_files:
            rel = path.decode('macroman', errors='replace')
            if d in path:
                rel = path[len(d)+1:].decode('macroman', errors='replace')
            if name not in missing:
                missing[name] = []
            missing[name].append((rel, sz))
            total_miss += sz

print()
hr(['humount'], EA)
hr(['humount'], EB)
shutil.rmtree(TMP, ignore_errors=True)

if not missing:
    print(f"\n✓ ALL {total_orig} files present in v2. No omissions!")
    sys.exit(0)

print(f"\n× {sum(len(v) for v in missing.values())} files missing ({total_miss/1024:.1f} KB)")
print(f"  Total in original: {total_orig} files")
for d in sorted(missing):
    files = missing[d]
    print(f"\n  {d}: {len(files)} files missing, {sum(f[1] for f in files)/1024:.1f} KB")
    for name, sz in sorted(files)[:5]:
        print(f"    {name} ({sz/1024:.1f} KB)")
    if len(files) > 5:
        print(f"    ... and {len(files)-5} more")
