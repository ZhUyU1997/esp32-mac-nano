/* Kuhn false-zero exceptions.
 * Source: glibc wcwidth() (C.UTF-8), glibc 2.35 (Ubuntu) — the
 * same libc xterm links for column width. Regenerate with:
 *   gcc -O2 -o /tmp/wcwprobe scripts/wcwprobe.c
 *   python3 -c 'print all cps 0..0x10FFFF' | ... | /tmp/wcwprobe > w.txt
 *   python3 scripts/gen-width-tables.py w.txt  (writes the three .inc)
 */
static const uint32_t kuhn_false_zero[] = {
  0x600,
  0x601,
  0x602,
  0x603,
  0x6dd,
  0x6de,
  0x70f,
  0x1734,
  0x302e,
  0x302f,
};
