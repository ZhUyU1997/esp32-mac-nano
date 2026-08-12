#!/usr/bin/env python3
"""Regenerate libvterm width tables from glibc wcwidth measurements.

Usage:
  gcc -O2 -o /tmp/wcwprobe scripts/wcwprobe.c
  python3 - <<'EOF'
  with open('/tmp/allcps.txt','w') as f:
      for cp in range(0x110000):
          f.write(f'{cp:X}\\n')
  EOF
  /tmp/wcwprobe /tmp/allcps.txt > /tmp/allwcw.txt
  python3 scripts/gen-width-tables.py /tmp/allwcw.txt

Writes:
  libvterm/src/fullwidth.inc   (wcwidth() == 2 ranges)
  libvterm/src/combining.inc   (wcwidth() == 0 ranges, excluding controls)
  libvterm/src/false_zero.inc  (codepoints Kuhn's table marks 0-wide but
                                glibc reports > 0)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load_widths(path):
    w = {}
    for line in open(path):
        cp_s, w_s = line.split()
        w[int(cp_s, 16)] = int(w_s)
    return w


def to_ranges(cps):
    runs = []
    prev = None
    for cp in sorted(cps):
        if prev is not None and cp == prev[1] + 1:
            prev[1] = cp
        else:
            prev = [cp, cp]
            runs.append(prev)
    return runs


def write_inc(path, title, body_lines):
    lines = [
        f"/* {title}",
        " * Source: glibc wcwidth() (C.UTF-8), glibc 2.35 (Ubuntu) — the",
        " * same libc xterm links for column width. Regenerate with:",
        " *   gcc -O2 -o /tmp/wcwprobe scripts/wcwprobe.c",
        " *   python3 -c 'print all cps 0..0x10FFFF' | ... | /tmp/wcwprobe > w.txt",
        " *   python3 scripts/gen-width-tables.py w.txt  (writes the three .inc)",
        " */",
    ]
    lines.extend(body_lines)
    path.write_text("\n".join(lines) + "\n")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    w = load_widths(sys.argv[1])

    # fullwidth: wcwidth() == 2 (ranges only; unicode.c declares the array)
    wide = to_ranges([cp for cp, ww in w.items() if ww == 2])
    body = [f"/* {len(wide)} ranges, {sum(h - l + 1 for l, h in wide)} codepoints */"]
    body += [f"  {{ 0x{lo:x}, 0x{hi:x} }}," for lo, hi in wide]
    write_inc(ROOT / "libvterm/src/fullwidth.inc", "Double-width ranges.", body)

    # combining: wcwidth() == 0, excluding control chars
    zero = to_ranges([cp for cp, ww in w.items() if ww == 0 and cp >= 0xA0])
    body = ["static const struct interval combining_zero[] = {",
            f"/* {len(zero)} ranges, {sum(h - l + 1 for l, h in zero)} codepoints */"]
    body += [f"  {{ 0x{lo:x}, 0x{hi:x} }}," for lo, hi in zero]
    body.append("};")
    write_inc(ROOT / "libvterm/src/combining.inc",
              "Zero-width combining ranges.", body)

    # Kuhn false-zero: Kuhn's combining table marks these 0-wide, glibc says >0
    usrc = (ROOT / "libvterm/src/unicode.c").read_text()
    m = re.search(r"static const struct interval combining\[\] = \{(.*?)\};",
                  usrc, re.S)
    kuhn_zero = set()
    for lo_s, hi_s in re.findall(r"\{ (\w+), (\w+) \}", m.group(1)):
        kuhn_zero.update(range(int(lo_s, 16), int(hi_s, 16) + 1))
    false_zero = sorted(cp for cp in kuhn_zero
                        if w.get(cp, -1) > 0 and cp >= 0xA0)
    body = ["static const uint32_t kuhn_false_zero[] = {"]
    body += [f"  0x{cp:x}," for cp in false_zero]
    body.append("};")
    write_inc(ROOT / "libvterm/src/false_zero.inc",
              "Kuhn false-zero exceptions.", body)
    print("wrote fullwidth.inc / combining.inc / false_zero.inc")


if __name__ == "__main__":
    main()
