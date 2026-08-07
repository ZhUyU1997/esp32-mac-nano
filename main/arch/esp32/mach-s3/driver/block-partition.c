/*
 * main/platform/io/hd.c - ESP32 flash partition backing store for emulated SCSI disk
 *
 * Reads come from the partition; written blocks are copied into a sparse RAM
 * overlay (session-only, not persisted). Reads merge overlay with flash.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "block/block.h"
#include "driver.h"
#include "device.h"
#include "dt.h"
#include "esp_partition.h"
#include <esp_heap_caps.h>

class(block_partition_t, block_t)
{
	const esp_partition_t *part;
	uint64_t size;
	uint8_t **overlay;
};

class_impl(block_partition_t, block_t){};

destructor(block_partition_t)
{
	if (this->overlay != NULL) {
		unsigned long nblocks = (unsigned long)(this->size / 512);
		for (unsigned long i = 0; i < nblocks; i++) {
			free(this->overlay[i]);
		}
		free(this->overlay);
		this->overlay = NULL;
	}
}

#define block_partition_priv(disk) dynamic_cast(block_partition_t)(disk)

static uint8_t *alloc_block(void)
{
	uint8_t *b = heap_caps_malloc(512, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	if (b == NULL) {
		b = malloc(512);
	}
	return b;
}

static uint64_t block_partition_capacity(block_t *disk)
{
	block_partition_t *p = block_partition_priv(disk);

	if (p == NULL) {
		return 0;
	}
	return p->size;
}

static uint64_t block_partition_read(block_t *disk, uint8_t *dst, uint64_t offset, uint64_t count)
{
	block_partition_t *p = block_partition_priv(disk);

	if (p == NULL || p->overlay == NULL) {
		return 0;
	}
	if ((offset % 512) != 0 || (count % 512) != 0) {
		return 0;
	}
	if (offset + count > p->size) {
		return 0;
	}

	unsigned long lba = (unsigned long)(offset / 512);
	unsigned long cnt = (unsigned long)(count / 512);

	unsigned long i = 0;
	while (i < cnt) {
		unsigned long blk = lba + i;
		if (p->overlay[blk] != NULL) {
			memcpy(dst + i * 512, p->overlay[blk], 512);
			i++;
			continue;
		}
		unsigned long run = 0;
		while (i + run < cnt && p->overlay[lba + i + run] == NULL) {
			run++;
		}
		esp_err_t err = esp_partition_read(p->part, (lba + i) * 512, dst + i * 512, run * 512);
		if (err != ESP_OK) {
			return 0;
		}
		i += run;
	}
	return count;
}

static uint64_t block_partition_write(block_t *disk, const uint8_t *src, uint64_t offset, uint64_t count)
{
	block_partition_t *p = block_partition_priv(disk);

	if (p == NULL || p->overlay == NULL) {
		return 0;
	}
	if ((offset % 512) != 0 || (count % 512) != 0) {
		return 0;
	}
	if (offset + count > p->size) {
		return 0;
	}

	unsigned long lba = (unsigned long)(offset / 512);
	unsigned long cnt = (unsigned long)(count / 512);

	for (unsigned long k = 0; k < cnt; k++) {
		unsigned long blk = lba + k;
		if (p->overlay[blk] == NULL) {
			p->overlay[blk] = alloc_block();
			if (p->overlay[blk] == NULL) {
				return 0;
			}
		}
		memcpy(p->overlay[blk], src + k * 512, 512);
	}
	return count;
}

static void block_partition_sync(block_t *disk)
{
	(void)disk;
}

static int block_partition_finalize(block_t *disk)
{
	block_partition_t *p = block_partition_priv(disk);

	unsigned long nblocks = (unsigned long)(p->size / 512);
	p->overlay = calloc(nblocks, sizeof(uint8_t *));
	if (p->overlay == NULL) {
		printf("Failed to alloc overlay table\n");
		return 1;
	}
	return 0;
}

block_t *block_create_from_partition(uint64_t type_u64, uint64_t subtype_u64)
{
	block_partition_t *obj = new (block_partition_t);
	if (obj == NULL) {
		return NULL;
	}
	block_t *b = dynamic_cast(block_t)(obj);
	if (b == NULL) {
		delete (obj);
		return NULL;
	}
	b->capacity = block_partition_capacity;
	b->read = block_partition_read;
	b->write = block_partition_write;
	b->sync = block_partition_sync;

	if (type_u64 == 0) {
		type_u64 = 0x40;
	}
	if (subtype_u64 == 0) {
		subtype_u64 = 0x2;
	}
	if (type_u64 > 0xff || subtype_u64 > 0xff) {
		delete (obj);
		return NULL;
	}
	const esp_partition_t *part = esp_partition_find_first((esp_partition_type_t)type_u64, (esp_partition_subtype_t)subtype_u64, NULL);
	if (part == 0) {
		printf("Couldn't find partition type=0x%llx subtype=0x%llx\n", (unsigned long long)type_u64, (unsigned long long)subtype_u64);
		delete (obj);
		return NULL;
	}
	obj->part = part;
	obj->size = (uint64_t)part->size;

	if (block_partition_finalize(b) != 0) {
		delete (obj);
		return NULL;
	}
	printf("Using block partition (%llu bytes)\n", (unsigned long long)obj->size);
	return b;
}

static device_t *block_partition_probe(driver_t *drv, dtnode_t *n)
{
	uint64_t type = dt_read_u64(n, "type", 0x40);
	uint64_t subtype = dt_read_u64(n, "subtype", 0x2);

	block_t *b = block_create_from_partition(type, subtype);
	if (b == NULL) {
		return NULL;
	}
	device_t *dev = register_block(b, drv, n);
	if (dev == NULL) {
		delete (b);
		return NULL;
	}
	return dev;
}

impl(block_partition, driver_t){
        .name = "block-partition",
        .probe = block_partition_probe,
};
