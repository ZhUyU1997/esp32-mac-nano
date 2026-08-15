/* sauce.h — SAUCE record parser for ANSI art files.
 *
 * SAUCE ("Standard Architecture for Universal Comment Extensions") is the
 * 128-byte metadata block appended to BBS-era art files (ANSi/PCBoard/
 * NFO/...) by the editor that produced them. It carries the title, author,
 * declared grid size, font code and the iCE-colour flag — everything an
 * art renderer (libansilove-style) needs that the byte stream itself does
 * not provide.
 *
 * Layout (from the SAUCE specification):
 *   ID(5) "SAUCE" Version(2) Title(35) Author(20) Group(20) Date(8)
 *   FileSize(4 LE) DataType(1) FileType(1) TInfo1(2 LE)=columns
 *   TInfo2(2 LE)=rows TInfo3(2 LE)=font TInfo4(2 LE) Comments(4 LE)
 *   Flags(1, bit0 = iCE colours) + 19 unused bytes = 128 total.
 * An optional COMNT record (255 B per line) sits between the art and the
 * SAUCE record: "COMNT"(5) + line count(2 LE) + count*255 bytes.
 */
#ifndef SAUCE_H
#define SAUCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SAUCE_RECORD_LEN 128
#define SAUCE_COMNT_LEN 255

/* DataType field values */
#define SAUCE_DT_NONE 0
#define SAUCE_DT_CHAR 1   /* character-based art (ANSi/ASCII/... ) */

/* FileType values for SAUCE_DT_CHAR */
#define SAUCE_FT_ANSI 1   /* ANSi */
#define SAUCE_FT_ASCII 2  /* ASCII */
#define SAUCE_FT_ANSI_ICE 3 /* ANSi with iCE colours */
#define SAUCE_FT_PCB 4    /* PCBoard */
#define SAUCE_FT_AVATAR 6
#define SAUCE_FT_NFO 7    /* NFO */

/* TInfo3 font codes (subset relevant to ANSi) */
#define SAUCE_FONT_IBM_VGA 0
#define SAUCE_FONT_AMIGA_TOPAZ 1
#define SAUCE_FONT_AMIGA_TOPAZ2 2
#define SAUCE_FONT_AMIGA_POTNOODLE 3

typedef struct {
	bool present;
	char title[36];   /* +1 for NUL */
	char author[21];
	char group[21];
	char date[9];     /* YYYYMMDD */
	uint32_t file_size; /* declared by the editor, may be 0 */
	uint8_t data_type;
	uint8_t file_type;
	uint16_t columns; /* TInfo1 */
	uint16_t rows;    /* TInfo2 */
	uint16_t font;    /* TInfo3 */
	uint16_t aspect;   /* TInfo4: aspect ratio */
	uint8_t flags;    /* bit0 = iCE colours */
	size_t data_len;  /* art bytes before the SAUCE (+ COMNT) record */
} sauce_t;

/* Locate and parse the trailing SAUCE record of buf. On success returns
 * true, fills out (fields NUL-terminated, strings space-trimmed) and sets
 * out->data_len to the length of the art bytes (record and any COMNT
 * block excluded). On failure returns false and leaves out->present = 0. */
bool sauce_parse(const uint8_t *buf, size_t len, sauce_t *out);

#endif
