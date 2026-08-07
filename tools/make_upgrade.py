#!/usr/bin/env python3
"""
make_upgrade.py — Package upgrade.bin for SD card firmware + hd upgrade.

Usage:
    # After 'idf.py build'
    python3 tools/make_upgrade.py \
        --firmware build/esp32-mac-nano.bin \
        --hd hd.img \
        --output upgrade.bin

    # Firmware only:
    python3 tools/make_upgrade.py \
        --firmware build/esp32-mac-nano.bin \
        --output upgrade.bin

    # HD only:
    python3 tools/make_upgrade.py \
        --hd hd.img \
        --output upgrade.bin
"""

import argparse
import os
import struct

MAGIC = b"ESUP"
VERSION = 1
HEADER_SIZE = 10  # magic(4) + version(2) + count(2) + reserved(2)
ENTRY_SIZE = 16   # offset(4) + size(4) + name(8)


def make_upgrade(firmware_path=None, hd_path=None, output="upgrade.bin"):
    entries = []
    blobs = []

    if firmware_path:
        with open(firmware_path, "rb") as f:
            data = f.read()
        entries.append({
            "name": b"factory\0\0\0",  # We actually write to ota_0, but name references factory for clarity
            "data": data,
            "label": "firmware",
        })
        print(f"  firmware: {firmware_path} ({len(data)} bytes)")

    if hd_path:
        with open(hd_path, "rb") as f:
            data = f.read()
        entries.append({
            "name": b"hd\0\0\0\0\0\0\0\0",
            "data": data,
            "label": "hd",
        })
        print(f"  hd:       {hd_path} ({len(data)} bytes)")

    if not entries:
        print("Error: nothing to package. Use --firmware and/or --hd.")
        return 1

    # Build header + entry table + data
    header_size = HEADER_SIZE + len(entries) * ENTRY_SIZE
    offset = header_size

    # Entry table
    entry_table = b""
    for ent in entries:
        entry_table += struct.pack("<II", offset, len(ent["data"]))
        # Pad name to 8 bytes
        name = ent["name"][:8].ljust(8, b"\0")
        entry_table += name
        offset += len(ent["data"])

    # Header: magic(4) + version(2) + count(2)
    header = MAGIC + struct.pack("<HH", VERSION, len(entries))
    # CRC placeholder (2 bytes) — reserved, 0 for now
    header += struct.pack("<H", 0)

    # Assemble
    payload = b"".join(ent["data"] for ent in entries)
    upgrade = header + entry_table + payload

    with open(output, "wb") as f:
        f.write(upgrade)

    print(f"\n  output:   {output} ({len(upgrade)} bytes)")
    print(f"  entries:  {len(entries)}")
    for ent in entries:
        print(f"    - {ent['label']}: {len(ent['data'])} bytes")

    # Verify: print suggested partition sizes
    total_mb = len(upgrade) / (1024 * 1024)
    print(f"\n  Total upgrade package: {total_mb:.2f} MB")
    print(f"  Make sure factory/ota_0 partition (2M) can hold firmware ({len(entries[0]['data'])/1024:.0f} KB)" if firmware_path else "")
    return 0


def main():
    parser = argparse.ArgumentParser(description="Package upgrade.bin for SD card upgrade")
    parser.add_argument("--firmware", help="Path to firmware app binary (build/esp32-mac-nano.bin)")
    parser.add_argument("--hd", help="Path to hard disk image (hd.img)")
    parser.add_argument("--output", default="upgrade.bin", help="Output file (default: upgrade.bin)")
    args = parser.parse_args()

    if not args.firmware and not args.hd:
        parser.error("At least one of --firmware or --hd is required")

    return make_upgrade(firmware_path=args.firmware, hd_path=args.hd, output=args.output)


if __name__ == "__main__":
    exit(main())
