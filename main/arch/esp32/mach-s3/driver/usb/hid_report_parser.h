/*
 * HID Report Descriptor parser — modelled on Linux's hid-core.
 *
 * Architecture:
 *   struct hid_device *hdev = hid_parse_device(desc, len);
 *   struct hid_field *f = hid_find_field(hdev, HID_REPORT_TYPE_INPUT, page, usage);
 *   int32_t val = hid_field_extract(f, data, len, index);
 *   hid_free_device(hdev);
 *
 * Internal parser (hid_item, state machine) is private to the .c file.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* hid_report_type_t — either from ESP-IDF or self-defined */
#if defined(ESP_PLATFORM)
#include "usb/hid.h"
#else
typedef enum {
	HID_REPORT_TYPE_INPUT   = 0x01,
	HID_REPORT_TYPE_OUTPUT  = 0x02,
	HID_REPORT_TYPE_FEATURE = 0x03,
} hid_report_type_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Constants ---- */
#define HID_MAX_IDS      16
#define HID_MAX_FIELDS   32
#define HID_MAX_COLLECTIONS 8

/* Collection types */
#define HID_COLLECTION_PHYSICAL    0
#define HID_COLLECTION_APPLICATION 1
#define HID_COLLECTION_LOGICAL     2

/* Main-item flags (HID 1.11, §6.2.2.4) */
#define HID_MAIN_ITEM_CONSTANT    0x001
#define HID_MAIN_ITEM_VARIABLE    0x002
#define HID_MAIN_ITEM_RELATIVE    0x004
#define HID_MAIN_ITEM_WRAP        0x008
#define HID_MAIN_ITEM_NONLINEAR   0x010
#define HID_MAIN_ITEM_NO_PREFERRED 0x020
#define HID_MAIN_ITEM_NULL_STATE  0x040
#define HID_MAIN_ITEM_VOLATILE    0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE 0x100

/* ---- Types ---- */

/* Note: hid_report_type_t (HID_REPORT_TYPE_INPUT/OUTPUT/FEATURE) is in usb/hid.h */

struct hid_field {
	uint16_t       usage_page;
	uint16_t       usage_id;
	unsigned       report_offset;  /* bit offset within report (excl. report ID) */
	unsigned       report_size;    /* bit width of each value */
	unsigned       report_count;   /* how many values in this field */
	int32_t        logical_minimum;
	int32_t        logical_maximum;
	unsigned       flags;          /* HID_MAIN_ITEM_* */
};

struct hid_report {
	unsigned            id;          /* report ID (0 if none) */
	unsigned            type;        /* hid_report_type_t */
	unsigned            application; /* toplevel Application collection usage */
	unsigned            size;        /* total bits (excl. report ID) */
	unsigned            maxfield;
	struct hid_field   *field[HID_MAX_FIELDS];
};

struct hid_report_enum {
	struct hid_report  *report_id_hash[HID_MAX_IDS];
	bool                numbered;    /* true if any report uses non-zero ID */
};

struct hid_collection {
	unsigned type;    /* PHYSICAL=0, APPLICATION=1, LOGICAL=2 */
	unsigned usage;   /* combined usage (page << 16) | id */
	int     parent_idx;
};

/* HID device descriptor holder.
 * report_enum[0] = INPUT, [1] = OUTPUT, [2] = FEATURE (per HID 1.11 §7.2.1). */
struct hid_device {
	struct hid_report_enum *report_enum[3];
	struct hid_collection   collection[HID_MAX_COLLECTIONS];
	unsigned                maxcollection;
};

/* ---- API ---- */

/* Allocate an empty hid_device. */
struct hid_device *hid_allocate_device(void);

/* Parse a raw HID Report Descriptor into an existing hid_device.
 * Returns 0 on success, -1 on error. */
int hid_parse_report(struct hid_device *hdev, const uint8_t *desc, size_t desc_len);

/* Allocate + parse in one call. Returns NULL on error. */
struct hid_device *hid_parse_device(const uint8_t *desc, size_t desc_len);

/* Free a hid_device and all its reports/fields. */
void hid_destroy_device(struct hid_device *hdev);

/* Find the first field matching (usage_page : usage_id) in reports of the
 * given type. */
struct hid_field *hid_find_field(const struct hid_device *hdev,
                                 unsigned report_type,
                                 uint16_t usage_page, uint16_t usage_id);

/* Extract a value from raw report data at the field's bit-offset + size.
 * 'index' selects which value (0 .. report_count-1). */
int32_t hid_field_extract(const struct hid_field *field,
                          const uint8_t *data, size_t data_len,
                          unsigned index);

/* Report length in bytes (including report ID byte if id > 0). */
static inline unsigned hid_report_len(const struct hid_report *r)
{
	return (r->size + 7) / 8 + (r->id > 0 ? 1 : 0);
}

/* Dump all reports and fields to stdout (debug). */
void hid_dump_device(const struct hid_device *hdev);

#ifdef __cplusplus
}
#endif
