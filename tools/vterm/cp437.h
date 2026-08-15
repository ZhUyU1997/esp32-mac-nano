/* cp437.h — CP437 (IBM PC code page 437) byte -> Unicode helpers.
 *
 * ANSI art files are CP437 byte streams. libvterm only accepts UTF-8,
 * so bytes 0x80-0xFF must be mapped to their Unicode code points before
 * feeding (the renderer maps them back to the VGA glyphs via
 * term_unicode_to_cp437). Bytes < 0x80 pass through unchanged — the ESC /
 * CR / LF that carry art structure must reach libvterm verbatim.
 */
#ifndef CP437_H
#define CP437_H

#include <stddef.h>
#include <stdint.h>

/* CP437 byte -> Unicode code point (0x80-0xFF; below 0x80 returns the
 * byte itself). */
uint32_t cp437_to_unicode(uint8_t b);

/* Convert a CP437 buffer to UTF-8 (worst case: 3 bytes per input byte).
 * Returns the number of bytes written to out. out may be NULL to count. */
size_t cp437_to_utf8(const uint8_t *in, size_t n, char *out, size_t cap);

#endif
