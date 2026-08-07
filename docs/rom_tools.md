# ROM Customization & Tools (Quick Reference)

This project uses two approaches to modify classic Mac ROM/UI behavior:

- Runtime patching: patch the ROM image or in-memory data during emulator/firmware boot (e.g. [rom.c](../main/core/macplus/core/rom.c)).
- Offline patching: edit ROM images or the System file’s resource fork, then write the modified artifacts back to disk images (e.g. the DSAT workflow used here).

## Tool categories

### Resource editors

For visual editing of Classic Mac OS resources (`'ICON'`, `'STR '`, `'STR#'`, `'FONT'`, `'CURS'`, etc.):

- ResEdit
- Resorcerer

Good for:
- Icons/strings/fonts stored as standard resources.
- Not ideal for “raw” ROM data stored at fixed offsets (not represented as resources).

### Hex editors

For byte-level edits of ROM/image files (fixed-offset bitmaps/tables/machine code):

- Classic Mac: FEdit / FEdit Plus
- Modern hosts: any hex editor (HxD, 0xED, etc.)

Notes:
- ROM edits often require checksum handling (varies by model/ROM version; some tools automate this).

### ROM patching / flashing toolchains

More end-to-end workflows (bootable, flashable, rollback-friendly):

- BMOW ROM-inator ecosystem (common in vintage Mac ROM SIMM mods)
- dougg3 Mac ROM SIMM / patch tooling (ROM patch + programmer tooling)

These typically automate repacking, offset fixups, and checksums.

## This repo: offline DSAT patch for “Welcome”

Welcome text and icon placement come from the `DSAT` resource in the System file’s resource fork (ROM fetches it via `GetResource('DSAT', ...)` and uses it to draw).

Repo scripts:

- [dsat-tool.js](../scripts/dsat-tool.js): dump/parse/patch `DSAT` from an HFS image (via MacBinary).
- [patch-hd10-welcome.sh](../scripts/patch-hd10-welcome.sh): reset `hd10.img` to HEAD and apply a minimal Welcome text + one icon Rect offset.

Centering offset:

- `dh = (DISP_WIDTH - 512) / 2`
- For `DISP_WIDTH=640`, `dh=64`

## References

- BMOW: Hacking the Happy Mac: https://www.bigmessowires.com/2015/02/05/hacking-the-happy-mac/
- dougg3/mac-rom-simm-programmer: https://github.com/dougg3/mac-rom-simm-programmer
