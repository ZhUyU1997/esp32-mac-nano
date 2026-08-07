---
name: disk-ops
description: Disk image operations with djjr, hfsutils, and hfs-sync.sh
---

# Disk Operations with djjr + hfsutils

Tools available in the project:
- `./tools/djjr` — Disk Jockey Jr (disk image creator/converter/analyzer)
- `hformat` / `hmount` / `hcopy` / `hls` / `hvol` / `hattrib` — hfsutils (need `HOME=writable_dir`)

## Analysis

### Analyze a disk image (works for both .img volume and .hda device images)
```bash
./tools/djjr analyze <image>
./tools/djjr analyze -f <image>    # also show file tree
```

### Mount an HFS volume to browse files
```bash
# Extract HFS partition from device image first
dd if=<image.hda> of=/tmp/vol.img bs=512 skip=96

# Then mount with hfsutils (HOME must point to a writable dir)
HOME=/tmp hmount /tmp/vol.img
HOME=/tmp hls -la          # list files
HOME=/tmp hls -la -a       # include invisible files
HOME=/tmp hvol             # volume info
HOME=/tmp hattrib -a <file>  # show invisibility flag
HOME=/tmp humount
```

## Conversion

### Volume image (.dsk/.img) → Device image (.hda) — for Snow emulator / BlueSCSI
```bash
./tools/djjr convert to-device <input.dsk> <output.hda>
```
⚠️ Only works for **HFS** volumes. MFS volumes are not supported.

### Device image (.hda) → Volume image (.img) — extract the HFS partition
```bash
./tools/djjr convert to-volume <input.hda> <output.img>
```

### Create a fresh device image
```bash
./tools/djjr create mac-device -sM <size_mb> --scsi-id <id> <output.hda>
```

## Shrinking a Device Image (e.g., 1.1 GB → 200 MB)

No tool can directly shrink HFS. Use copy-and-recreate:

```bash
# 1. Extract HFS volume
./tools/djjr convert to-volume large.hda /tmp/old_vol.img

# 2. Create smaller empty HFS volume
dd if=/dev/zero of=/tmp/new_vol.img bs=1M count=200
hformat -l "VolumeName" /tmp/new_vol.img

# 3. Copy all files (preserves data + resource forks)
HOME=/tmp bash scripts/hfs-sync.sh -v /tmp/old_vol.img /tmp/new_vol.img

# 4. Create new smaller device image and inject volume
./tools/djjr create mac-device -sM 200 --scsi-id 6 /tmp/new.hda
dd if=/tmp/new_vol.img of=/tmp/new.hda bs=512 seek=96 conv=notrunc

# 5. Replace the original
cp /tmp/new.hda <target.hda>
```

## Installing Mac Apps onto a Disk Image

```bash
# Build Mac apps
make mac-all

# Install into hd.img (from .dsk files in mac-app/)
# Copy hd.img.orig as base first:
cp macintosh/hd.img.orig macintosh/hd.img
# Then install apps:
for dsk in mac-app/*App/build/*.dsk; do
    app=$(basename "$dsk" .dsk)
    HOME=/tmp bash scripts/hfs-sync.sh "$dsk" macintosh/hd.img
done
```

## Boot Block (HFS Volume)

### Check if a disk is bootable
```bash
python3 -c "
with open('<disk>', 'rb') as f:
    f.seek(0xC000)  # block 96 = HFS partition start
    sig = int.from_bytes(f.read(2), 'big')
    print('BOOTABLE' if sig == 0x4C4B else 'DATA DISK')
"
```

HFS Boot Block structure:
| Offset | Field | Description |
|--------|-------|-------------|
| 0x000 | sbSig | 0x4C4B = "LK" = valid boot block |
| 0x00A | sbSysName | "System" as Pascal string |
| 0x01A | sbFinderName | "Finder" as Pascal string |
| 0x03A+ | sbCode | 68K boot code + more file names |

## Disk Image Format Reference

| Format | Extension | Description |
|--------|-----------|-------------|
| Device image | `.hda` | Full SCSI device with partition map + driver + HFS partition. **Used by Snow emulator.** |
| Volume image | `.dsk` / `.img` | Just the HFS volume, no partition table. Used by Mini vMac, Basilisk II. |
| Infinite Mac export | `.infinitemacdisk` | ZIP with dirty chunks + bitmap (differential). Needs Infinite Mac to reconstruct. |

## SCSI ID Convention (for this project)

| SCSI ID | File | Purpose |
|---------|------|---------|
| 6 | `hd.img` | Primary boot disk (checked first by Mac ROM) |
| 0 | `hd0.img` | Extra data disk |
| 1 | `hd1.img` | Extra data disk |
| ... | ... | ... |
| 7 | `hd7.img` | Extra data disk |

Unused SCSI IDs are simply skipped (no error).
