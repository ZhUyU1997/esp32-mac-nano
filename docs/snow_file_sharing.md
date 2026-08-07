# Snow “File Sharing” (BlueSCSI Toolbox Protocol) — Summary

This repository keeps a local checkout of the Snow emulator at `ref/snow` (a symlink to the upstream project). Snow’s **File sharing** feature is not AFP/SMB networking. It implements the **BlueSCSI Toolbox** vendor command protocol over an emulated SCSI device, allowing easy file transfer between a host folder and the emulated classic Mac OS system.

## What it does (user-facing)

- You choose a **shared host directory** in the Snow UI: `Tools → File sharing → Select folder…`.
- In the emulated Mac OS, you run the “BlueSCSI SD Transfer” tool (Snow can insert a toolbox floppy via `Tools → File sharing → Insert toolbox floppy`).
- The Mac tool can:
  - List files in the shared host folder.
  - Download files from the host folder into the emulated system.
  - Upload files from the emulated system into the host folder.

Prerequisites (per Snow docs): emulate a model with SCSI and attach at least one SCSI device.

## How it’s wired internally (high level)

1. UI selects a shared directory and sends `EmulatorCommand::SetSharedDir(Option<PathBuf>)`.
2. The emulator forwards this to the SCSI controller (`scsi.set_shared_dir(path)`), which constructs a `BlueSCSI` toolbox handler bound to that directory.
3. When the emulated Mac issues SCSI commands with opcodes in the vendor range `0xD0..=0xD9`, Snow intercepts them and routes them to the toolbox handler instead of a normal SCSI target.

## Protocol coverage in Snow

Snow’s toolbox handler supports (at least) these BlueSCSI Toolbox v0-style commands:

- `0xD0` List files in the shared directory (entries are name-sorted and dotfiles are hidden).
- `0xD2` Count files.
- `0xD1` Read a file by index with an offset (block-based reads).
- `0xD3 / 0xD4 / 0xD5` Host-write path: create file, write file chunks, finalize/sync.
- `0xD6` Toggle toolbox debug logging.
- `0xD9` Query metadata/capabilities (API version and feature flags like larger transfers).

This design makes Snow behave like a BlueSCSI device for file transfer, while keeping the actual data source/sink as a regular host filesystem folder.
