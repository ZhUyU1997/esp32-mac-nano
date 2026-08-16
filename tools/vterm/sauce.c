/* sauce.c — SAUCE record parser (see sauce.h for the layout). */
#include <stdio.h>
#include <string.h>

#include "sauce.h"

/* Locate a "SAUCE00"/"SAUCE01" record: first at the exact end of the
 * buffer (the normal case — editors append it last), then anywhere in the
 * final 400 bytes (tolerates trailing garbage). Returns the record
 * offset, or -1. */
static long sauce_locate(const uint8_t *buf, size_t len)
{
	if (len < SAUCE_RECORD_LEN)
		return -1;
	const uint8_t *rec = buf + len - SAUCE_RECORD_LEN;
	if (!memcmp(rec, "SAUCE00", 7) || !memcmp(rec, "SAUCE01", 7))
		return (long)(len - SAUCE_RECORD_LEN);
	size_t start = len > 400 ? len - 400 : 0;
	for (size_t i = start; i + SAUCE_RECORD_LEN <= len; i++) {
		if (!memcmp(buf + i, "SAUCE00", 7) || !memcmp(buf + i, "SAUCE01", 7))
			return (long)i;
	}
	return -1;
}

/* Strip the optional COMNT block between the art and the SAUCE record.
 * Spec layout: "COMNT"(5) + count(2 LE) + count*255 bytes, ending right
 * at the record. Falls back to the layout the telnet gallery server
 * detects (marker 255 B before the record -> strip 255 B). Returns the
 * art length. */
static size_t sauce_strip_comnt(const uint8_t *buf, size_t rec_off)
{
	for (int c = 1; c <= 16; c++) {
		size_t block = (size_t)c * SAUCE_COMNT_LEN + 7;
		if (rec_off < block)
			break;
		if (!memcmp(buf + rec_off - block, "COMNT", 5)) {
			uint16_t count = (uint16_t)(buf[rec_off - block + 5] |
			                            (buf[rec_off - block + 6] << 8));
			if (count == c)
				return rec_off - block;
		}
	}
	if (rec_off >= 262 && !memcmp(buf + rec_off - 262, "COMNT", 5))
		return rec_off - 262;
	if (rec_off >= 255 && !memcmp(buf + rec_off - 255, "COMNT", 5))
		return rec_off - 255;
	return rec_off;
}

/* Copy a fixed-width field, trimming trailing spaces/NULs. */
static void sauce_field(const uint8_t *buf, size_t off, size_t n, char *out)
{
	size_t end = n;
	while (end > 0 && (buf[off + end - 1] == ' ' || buf[off + end - 1] == 0))
		end--;
	memcpy(out, buf + off, end);
	out[end] = '\0';
}

bool sauce_parse(const uint8_t *buf, size_t len, sauce_t *out)
{
	memset(out, 0, sizeof(*out));
	long rec = sauce_locate(buf, len);
	if (rec < 0)
		return false;
	out->present = true;
	out->data_len = sauce_strip_comnt(buf, (size_t)rec);

	const uint8_t *r = buf + rec;
	/* Standard SAUCE record layout (128 bytes, ID "SAUCE00" at rec):
	 * ID(7) version(2) title(35) author(20) group(20) date(8)
	 * fileSize(4) dataType(1) fileType(1) TInfo1(2)=columns
	 * TInfo2(2)=rows TInfo3(2)=font TInfo4(2)=aspect comments(1)
	 * flags(1) tinfos(20). Some writers drop the version field (record
	 * starts with the title right after the ID): detect it by checking
	 * the version bytes are ASCII digits. */
	/* Standard SAUCE records use version "00" (practically always):
	 * only that triggers the versioned layout. Titles starting with
	 * digits (e.g. "1096 Ascii Colly", "01/97 AnsiCluster") must not
	 * be misdetected as a version. */
	int v = (r[7] == '0' && r[8] == '0') ? 2 : 0;
	sauce_field(r, 7 + v, 35, out->title);
	sauce_field(r, 42 + v, 20, out->author);
	sauce_field(r, 62 + v, 20, out->group);
	sauce_field(r, 82 + v, 8, out->date);
	out->file_size = (uint32_t)r[90 + v] | ((uint32_t)r[91 + v] << 8) |
	                 ((uint32_t)r[92 + v] << 16) | ((uint32_t)r[93 + v] << 24);
	out->data_type = r[94 + v];
	out->file_type = r[95 + v];
	out->columns = (uint16_t)(r[96 + v] | (r[97 + v] << 8));
	/* sanity: a broken/odd record can put garbage here (e.g. spaces =
	 * 0x2020 = 8224, or a stray small value like 21 for a real 80-col
	 * art); treat it as absent so the caller falls back. BBS art is
	 * 40-200 columns, anything below 40 is not a real art width. */
	if (out->columns < 40 || out->columns > 200)
		out->columns = 0;
	out->rows = (uint16_t)(r[98 + v] | (r[99 + v] << 8));
	if (out->rows < 1 || out->rows > 1024)
		out->rows = 0;
	out->font = (uint16_t)(r[100 + v] | (r[101 + v] << 8));
	out->aspect = (uint16_t)(r[102 + v] | (r[103 + v] << 8));
	out->flags = r[105 + v];
	/* an all-blank record (broken writer) is not trustworthy: the
	 * "columns" byte can be garbage (e.g. 78 for a real 80-col art)
	 * and the iCE flag may be garbage too; fall back so rendering
	 * stays consistent with libansilove (which usually misses these
	 * records entirely). */
	if (out->title[0] == '\0') {
		out->columns = 0;
		out->flags = 0;
	}
	return true;
	sauce_field(r, 44, 20, out->author);
	sauce_field(r, 64, 20, out->group);
	sauce_field(r, 84, 8, out->date);
	out->file_size = (uint32_t)r[92] | ((uint32_t)r[93] << 8) |
	                 ((uint32_t)r[94] << 16) | ((uint32_t)r[95] << 24);
	out->data_type = r[96];
	out->file_type = r[97];
	out->columns = (uint16_t)(r[98] | (r[99] << 8));
	out->rows = (uint16_t)(r[100] | (r[101] << 8));
	out->font = (uint16_t)(r[102] | (r[103] << 8));
	out->aspect = (uint16_t)(r[104] | (r[105] << 8));
	out->flags = r[107];
	return true;
}
