/* sauce.c — SAUCE record parser (see sauce.h for the layout). */
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
	sauce_field(r, 7, 35, out->title);
	sauce_field(r, 42, 20, out->author);
	sauce_field(r, 62, 20, out->group);
	sauce_field(r, 82, 8, out->date);
	out->file_size = (uint32_t)r[90] | ((uint32_t)r[91] << 8) |
	                 ((uint32_t)r[92] << 16) | ((uint32_t)r[93] << 24);
	out->data_type = r[94];
	out->file_type = r[95];
	out->columns = (uint16_t)(r[96] | (r[97] << 8));
	out->rows = (uint16_t)(r[98] | (r[99] << 8));
	out->font = (uint16_t)(r[100] | (r[101] << 8));
	out->aspect = (uint16_t)(r[102] | (r[103] << 8));
	out->flags = r[108];
	return true;
}
