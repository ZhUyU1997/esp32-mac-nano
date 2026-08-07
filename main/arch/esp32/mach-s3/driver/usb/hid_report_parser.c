/*
 * HID Report Descriptor parser — modelled on Linux's hid-core.c / hid_parser_main().
 *
 * MODIFY WITH CARE: any parser change MUST be verified by running the
 * standalone test suite at test_hid_parser.c:
 *
 *   gcc -std=gnu11 -I. hid_report_parser.c test_hid_parser.c && ./a.out
 */
#include "hid_report_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	HID_MAIN_INPUT = 0x8, HID_MAIN_OUTPUT = 0x9, HID_MAIN_FEATURE = 0xb,
	HID_MAIN_COLLECTION = 0xa, HID_MAIN_END_COLLECTION = 0xc,
	HID_GLOBAL_USAGE_PAGE = 0x0, HID_GLOBAL_LOGICAL_MINIMUM = 0x1,
	HID_GLOBAL_LOGICAL_MAXIMUM = 0x2, HID_GLOBAL_REPORT_SIZE = 0x7,
	HID_GLOBAL_REPORT_ID = 0x8, HID_GLOBAL_REPORT_COUNT = 0x9,
	HID_LOCAL_USAGE = 0x0, HID_LOCAL_USAGE_MINIMUM = 0x1,
	HID_LOCAL_USAGE_MAXIMUM = 0x2,
};

typedef struct {
	int32_t usage_page, logical_minimum, logical_maximum;
	int32_t report_size, report_count, report_id;
} hid_global_state_t;

typedef struct {
	int32_t usage_min, usage_max, usage_page;
	uint16_t local_usages[16];
	uint8_t local_usage_count;
} hid_local_state_t;

typedef struct {
	struct hid_device *hdev;
	hid_global_state_t global;
	hid_local_state_t local;
	unsigned collection_stack[HID_MAX_COLLECTIONS];
	unsigned collection_stack_ptr;
} hid_parser_t;

static int32_t sign_extend(int32_t val, int bytes)
{
	switch (bytes) {
	case 1: if (val & 0x80) val |= ~0xff; break;
	case 2: if (val & 0x8000) val |= ~0xffff; break;
	}
	return val;
}

static int32_t read_item_data(const uint8_t *desc, size_t desc_len, size_t *pos, int size_code)
{
	static const uint8_t sz[] = {0, 1, 2, 4};
	int len = sz[size_code & 3];
	int32_t val = 0;
	for (int i = 0; i < len; i++) {
		if (*pos >= desc_len) return 0;
		val |= ((int32_t)desc[(*pos)++]) << (i * 8);
	}
	return val;
}

/* ---- Collection stack ---- */

static unsigned hid_lookup_collection(hid_parser_t *p, unsigned type)
{
	for (int n = (int)p->collection_stack_ptr - 1; n >= 0; n--) {
		unsigned idx = p->collection_stack[n];
		if (p->hdev->collection[idx].type == type)
			return p->hdev->collection[idx].usage;
	}
	return 0;
}

static int open_collection(hid_parser_t *p, unsigned type)
{
	struct hid_device *hdev = p->hdev;
	if (hdev->maxcollection >= HID_MAX_COLLECTIONS) return -1;
	if (p->collection_stack_ptr >= HID_MAX_COLLECTIONS) return -1;

	unsigned usage = 0;
	if (p->local.local_usage_count > 0)
		usage = ((unsigned)p->local.usage_page << 16) | p->local.local_usages[0];

	unsigned idx = hdev->maxcollection++;
	hdev->collection[idx].type = type;
	hdev->collection[idx].usage = usage;
	hdev->collection[idx].parent_idx = p->collection_stack_ptr > 0
		? (int)p->collection_stack[p->collection_stack_ptr - 1] : -1;
	p->collection_stack[p->collection_stack_ptr++] = idx;
	return 0;
}

static void close_collection(hid_parser_t *p)
{
	if (p->collection_stack_ptr > 0)
		p->collection_stack_ptr--;
}

/* ---- Report / Field helpers ---- */

static struct hid_report_enum *enum_of(struct hid_device *hdev, unsigned report_type)
{
	if (report_type < 1 || report_type > 3) return NULL;
	struct hid_report_enum *re = hdev->report_enum[report_type - 1];
	if (!re) {
		re = calloc(1, sizeof(*re));
		hdev->report_enum[report_type - 1] = re;
	}
	return re;
}

static struct hid_report *register_report(hid_parser_t *p, unsigned report_type,
                                          unsigned report_id, unsigned application)
{
	struct hid_report_enum *re = enum_of(p->hdev, report_type);
	if (!re || report_id >= HID_MAX_IDS) return NULL;
	if (re->report_id_hash[report_id]) {
		/* Update application if first field didn't set it */
		if (!re->report_id_hash[report_id]->application)
			re->report_id_hash[report_id]->application = application;
		return re->report_id_hash[report_id];
	}

	struct hid_report *r = calloc(1, sizeof(*r));
	if (!r) return NULL;
	r->id = report_id;
	r->type = report_type;
	r->application = application;
	re->report_id_hash[report_id] = r;
	if (report_id > 0) re->numbered = true;
	return r;
}

static struct hid_field *register_field(struct hid_report *report)
{
	if (report->maxfield >= HID_MAX_FIELDS) return NULL;
	struct hid_field *f = calloc(1, sizeof(*f));
	if (!f) return NULL;
	report->field[report->maxfield++] = f;
	return f;
}

static int add_field(hid_parser_t *p, unsigned report_type, unsigned flags)
{
	unsigned rid = (unsigned)(p->global.report_id < 0 ? 0 : p->global.report_id);
	unsigned app = hid_lookup_collection(p, HID_COLLECTION_APPLICATION);
	struct hid_report *report = register_report(p, report_type, rid, app);
	if (!report) return -1;

	unsigned count = (unsigned)p->global.report_count;
	unsigned count_per_field = 1;

	if (p->local.local_usage_count > 0 &&
	    count % p->local.local_usage_count == 0)
		count_per_field = count / p->local.local_usage_count;

	unsigned offset = report->size;
	report->size += (unsigned)(p->global.report_size * count);

	for (unsigned k = 0; k < count; k += count_per_field) {
		struct hid_field *f = register_field(report);
		if (!f) return 0;

		f->usage_page = (uint16_t)p->global.usage_page;
		f->usage_id = (p->local.local_usage_count > 0)
			? p->local.local_usages[k / count_per_field]
			: (uint16_t)((unsigned)p->local.usage_min + k / count_per_field);
		f->report_offset = offset;
		f->report_size = (unsigned)p->global.report_size;
		f->report_count = count_per_field;
		f->logical_minimum = p->global.logical_minimum;
		f->logical_maximum = p->global.logical_maximum;
		f->flags = flags;
		offset += (unsigned)(p->global.report_size * count_per_field);
	}
	return 0;
}

static void free_report_enum(struct hid_report_enum *re)
{
	if (!re) return;
	for (unsigned i = 0; i < HID_MAX_IDS; i++) {
		struct hid_report *r = re->report_id_hash[i];
		if (!r) continue;
		for (unsigned j = 0; j < r->maxfield; j++)
			free(r->field[j]);
		free(r);
	}
	free(re);
}

/* ---- Public API ---- */

struct hid_device *hid_allocate_device(void)
{
	return calloc(1, sizeof(struct hid_device));
}

int hid_parse_report(struct hid_device *hdev, const uint8_t *desc, size_t desc_len)
{
	if (!hdev || !desc || !desc_len) return -1;
	for (int t = 0; t < 3; t++) {
		free_report_enum(hdev->report_enum[t]);
		hdev->report_enum[t] = NULL;
	}
	hdev->maxcollection = 0;

	hid_parser_t p = {.hdev = hdev, .global.report_id = -1};
	size_t pos = 0;

	while (pos < desc_len) {
		uint8_t prefix = desc[pos++];
		if (pos > desc_len) break;
		int bSize = prefix & 0x03, bType = (prefix >> 2) & 0x03, bTag = (prefix >> 4) & 0x0f;
		if (bSize == 2 && bTag == 0xf) {
			if (pos + 2 > desc_len) break;
			pos += desc[pos] + 2;
			if (pos > desc_len) break;
			continue;
		}
		int32_t val = read_item_data(desc, desc_len, &pos, bSize);

		if (bType == 0) {
			if (bTag == HID_MAIN_COLLECTION) {
				open_collection(&p, (unsigned)val);
				p.local = (hid_local_state_t){0};
			} else if (bTag == HID_MAIN_END_COLLECTION) {
				close_collection(&p);
				p.local = (hid_local_state_t){0};
			} else if (bTag == HID_MAIN_INPUT || bTag == HID_MAIN_OUTPUT || bTag == HID_MAIN_FEATURE) {
				if (p.global.report_size <= 0 || p.global.report_count <= 0) continue;
				unsigned rtype = (bTag == HID_MAIN_INPUT) ? HID_REPORT_TYPE_INPUT
				               : (bTag == HID_MAIN_OUTPUT) ? HID_REPORT_TYPE_OUTPUT
				               : HID_REPORT_TYPE_FEATURE;
				add_field(&p, rtype, (unsigned)val);
				p.local = (hid_local_state_t){0};
			}
		} else if (bType == 1) {
			switch (bTag) {
			case HID_GLOBAL_USAGE_PAGE:      p.global.usage_page = val; break;
			case HID_GLOBAL_LOGICAL_MINIMUM: p.global.logical_minimum = sign_extend(val, bSize); break;
			case HID_GLOBAL_LOGICAL_MAXIMUM: p.global.logical_maximum = sign_extend(val, bSize); break;
			case HID_GLOBAL_REPORT_SIZE:     p.global.report_size = val; break;
			case HID_GLOBAL_REPORT_COUNT:    p.global.report_count = val; break;
			case HID_GLOBAL_REPORT_ID:       p.global.report_id = val; break;
			}
		} else if (bType == 2) {
			switch (bTag) {
			case HID_LOCAL_USAGE:
				if (p.local.local_usage_count < 16)
					p.local.local_usages[p.local.local_usage_count++] = (uint16_t)val;
				p.local.usage_page = p.global.usage_page;
				break;
			case HID_LOCAL_USAGE_MINIMUM: p.local.usage_min = val; p.local.usage_page = p.global.usage_page; break;
			case HID_LOCAL_USAGE_MAXIMUM: p.local.usage_max = val; break;
			}
		}
	}
	return 0;
}

struct hid_device *hid_parse_device(const uint8_t *desc, size_t desc_len)
{
	struct hid_device *hdev = hid_allocate_device();
	if (!hdev) return NULL;
	if (hid_parse_report(hdev, desc, desc_len) != 0) {
		hid_destroy_device(hdev);
		return NULL;
	}
	return hdev;
}

void hid_destroy_device(struct hid_device *hdev)
{
	if (!hdev) return;
	for (int t = 0; t < 3; t++)
		free_report_enum(hdev->report_enum[t]);
	free(hdev);
}

struct hid_field *hid_find_field(const struct hid_device *hdev,
                                 unsigned report_type,
                                 uint16_t usage_page, uint16_t usage_id)
{
	if (!hdev || report_type < 1 || report_type > 3) return NULL;
	const struct hid_report_enum *re = hdev->report_enum[report_type - 1];
	if (!re) return NULL;
	for (unsigned i = 0; i < HID_MAX_IDS; i++) {
		struct hid_report *r = re->report_id_hash[i];
		if (!r) continue;
		for (unsigned j = 0; j < r->maxfield; j++) {
			struct hid_field *f = r->field[j];
			if (f->usage_page == usage_page && f->usage_id == usage_id)
				return f;
		}
	}
	return NULL;
}

int32_t hid_field_extract(const struct hid_field *field,
                          const uint8_t *data, size_t data_len, unsigned index)
{
	if (!field || !data) return 0;
	unsigned bit_off = field->report_offset + index * field->report_size;
	unsigned bit_size = field->report_size;
	if (bit_size == 0 || bit_size > 32) return 0;
	if ((bit_off + bit_size + 7) / 8 > data_len) return 0;
	uint32_t raw = 0;
	for (unsigned i = 0; i < bit_size; i++)
		if ((data[(bit_off + i) / 8] >> ((bit_off + i) % 8)) & 1)
			raw |= (1u << i);
	uint32_t sign_bit = 1u << (bit_size - 1);
	if (field->logical_minimum < 0 && (raw & sign_bit))
		raw |= ~((1u << bit_size) - 1);
	return (int32_t)raw;
}

static const char *collection_name(unsigned usage)
{
	switch (usage) {
	case 0x00010001: return "Pointer";
	case 0x00010002: return "Mouse";
	case 0x00010004: return "Joystick";
	case 0x00010005: return "Gamepad";
	case 0x00010006: return "Keyboard";
	case 0x00010080: return "System Control";
	case 0x000c0001: return "Consumer Control";
	default: return NULL;
	}
}

void hid_dump_device(const struct hid_device *hdev)
{
	if (!hdev) return;

	/* Print collection hierarchy */
	if (hdev->maxcollection) {
		printf("HID Collections:\n");
		for (unsigned i = 0; i < hdev->maxcollection; i++) {
			const struct hid_collection *c = &hdev->collection[i];
			const char *name = collection_name(c->usage);
			printf("  [%u] type=%s usage=0x%06x",
			       i,
			       c->type == HID_COLLECTION_APPLICATION ? "App" :
			       c->type == HID_COLLECTION_PHYSICAL ? "Phys" : "Log",
			       c->usage);
			if (name) printf(" (%s)", name);
			printf(" parent=%d\n", c->parent_idx);
		}
	}

	static const char *type_names[] = {"INPUT", "OUTPUT", "FEATURE"};
	for (int t = 0; t < 3; t++) {
		const struct hid_report_enum *re = hdev->report_enum[t];
		if (!re) continue;
		printf("HID %s: %s\n", type_names[t],
		       re->numbered ? "numbered reports" : "no report IDs");
		for (unsigned i = 0; i < HID_MAX_IDS; i++) {
			struct hid_report *r = re->report_id_hash[i];
			if (!r) continue;
			const char *aname = collection_name(r->application);
			printf("  Report ID %u (%u bytes, %u fields)",
			       r->id, hid_report_len(r), r->maxfield);
			if (aname) printf(" App=%s", aname);
			printf("\n");
			for (unsigned j = 0; j < r->maxfield; j++) {
				struct hid_field *f = r->field[j];
				printf("    [%u] page=0x%04x usage=0x%04x "
				       "off=%u size=%u count=%u min=%d max=%d flags=0x%03x\n",
				       j, f->usage_page, f->usage_id,
				       f->report_offset, f->report_size, f->report_count,
				       f->logical_minimum, f->logical_maximum, f->flags);
			}
		}
	}
}
