#!/usr/bin/env python3
"""
make_webflash.py — Package a browser-flashable firmware bundle.

Reads build/flasher_args.json (produced by idf.py build) for the exact
flash layout, then emits:

    webflash-<VERSION>.zip
    ├── manifest.json        ← version/chip/flash_settings/files(+sha256)
    ├── bootloader.bin
    ├── partition-table.bin
    ├── ota_data_initial.bin
    └── esp32-mac-nano.bin

The web flasher (web/flash.html) loads the zip, verifies SHA-256 and
flashes each file at its manifest address.

Usage:
    # After 'idf.py build'
    python3 tools/make_webflash.py --version 1.0.0 --output webflash-1.0.0.zip
"""

import argparse
import hashlib
import json
import os
import zipfile


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def make_webflash(build_dir="build", version="0.0.0", output="webflash.zip"):
    flasher_args_path = os.path.join(build_dir, "flasher_args.json")
    with open(flasher_args_path, "r", encoding="utf-8") as f:
        flasher_args = json.load(f)

    flash_files = flasher_args["flash_files"]  # {"0x0": "bootloader/bootloader.bin", ...}
    flash_settings = flasher_args.get("flash_settings", {})

    files = []
    for offset_str, rel_path in sorted(flash_files.items(), key=lambda kv: int(kv[0], 16)):
        abs_path = os.path.join(build_dir, rel_path)
        with open(abs_path, "rb") as f:
            data = f.read()
        files.append({
            "name": os.path.basename(rel_path),
            "address": offset_str,
            "size": len(data),
            "sha256": sha256_hex(data),
        })
        print(f"  {offset_str:>8s}  {os.path.basename(rel_path):<24s} {len(data):>10d} B")

    manifest = {
        "version": version,
        "chip": flasher_args.get("extra_esptool_args", {}).get("chip", "esp32s3"),
        "flash_settings": flash_settings,
        "files": files,
    }

    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("manifest.json", json.dumps(manifest, indent=2))
        for entry in files:
            src = os.path.join(build_dir, flash_files[entry["address"]])
            zf.write(src, entry["name"])

    size = os.path.getsize(output)
    print(f"  manifest: {len(json.dumps(manifest))} B")
    print(f"Wrote {output} ({size} bytes, {size/1024:.1f} KB)")
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Package browser-flashable firmware bundle")
    parser.add_argument("--build-dir", default="build", help="ESP-IDF build directory")
    parser.add_argument("--version", default="0.0.0", help="Firmware version tag")
    parser.add_argument("--output", default="webflash.zip", help="Output zip path")
    args = parser.parse_args()
    raise SystemExit(make_webflash(args.build_dir, args.version, args.output))
