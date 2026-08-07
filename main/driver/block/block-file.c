#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "block/block.h"
#include "driver.h"
#include "device.h"
#include "dt.h"

class(block_file_t, block_t)
{
	FILE *image_file;
	uint64_t size;
	int readonly;

	uint8_t **overlay;
	uint32_t overlay_block_count;
};

class_impl(block_file_t, block_t){};

destructor(block_file_t)
{
	if (this->image_file != NULL) {
		fclose(this->image_file);
		this->image_file = NULL;
	}
	if (this->overlay != NULL) {
		for (uint32_t i = 0; i < this->overlay_block_count; i++) {
			free(this->overlay[i]);
		}
		free(this->overlay);
		this->overlay = NULL;
		this->overlay_block_count = 0;
	}
}

static block_file_t *block_file_priv(block_t *blk)
{
	return dynamic_cast(block_file_t)(blk);
}

static uint8_t *alloc_block(void)
{
	return malloc(512);
}

static uint64_t block_file_capacity(block_t *blk)
{
	block_file_t *p = block_file_priv(blk);

	if (p == NULL || p->image_file == NULL) {
		return 0;
	}
	return p->size;
}

static uint64_t block_file_read(block_t *blk, uint8_t *dst, uint64_t offset, uint64_t count)
{
	block_file_t *p = block_file_priv(blk);

	if (p == NULL || p->image_file == NULL) {
		return 0;
	}
	if ((offset % 512) != 0 || (count % 512) != 0) {
		return 0;
	}
	if (offset + count > p->size) {
		return 0;
	}

	if (p->overlay == NULL) {
		if (fseek(p->image_file, (long)offset, SEEK_SET) != 0) {
			return 0;
		}
		size_t got = fread(dst, 1, (size_t)count, p->image_file);
		return (uint64_t)got;
	}

	uint32_t lba = (uint32_t)(offset / 512u);
	uint32_t cnt = (uint32_t)(count / 512u);

	uint32_t i = 0;
	while (i < cnt) {
		uint32_t blk_idx = lba + i;
		if (blk_idx >= p->overlay_block_count) {
			return 0;
		}
		if (p->overlay[blk_idx] != NULL) {
			memcpy(dst + (size_t)i * 512u, p->overlay[blk_idx], 512u);
			i++;
			continue;
		}
		uint32_t run = 0;
		while (i + run < cnt) {
			uint32_t b = lba + i + run;
			if (b >= p->overlay_block_count || p->overlay[b] != NULL) {
				break;
			}
			run++;
		}
		if (run == 0) {
			return 0;
		}
		uint64_t file_off = (uint64_t)(lba + i) * 512ULL;
		if (fseek(p->image_file, (long)file_off, SEEK_SET) != 0) {
			return 0;
		}
		size_t got = fread(dst + (size_t)i * 512u, 1, (size_t)run * 512u, p->image_file);
		if (got != (size_t)run * 512u) {
			return 0;
		}
		i += run;
	}
	return count;
}

static uint64_t block_file_write(block_t *blk, const uint8_t *src, uint64_t offset, uint64_t count)
{
	block_file_t *p = block_file_priv(blk);

	if (p == NULL || p->image_file == NULL) {
		return 0;
	}
	if (p->readonly) {
		return 0;
	}
	if ((offset % 512) != 0 || (count % 512) != 0) {
		return 0;
	}
	if (offset + count > p->size) {
		return 0;
	}

	if (p->overlay == NULL) {
		if (fseek(p->image_file, (long)offset, SEEK_SET) != 0) {
			return 0;
		}
		size_t wrote = fwrite(src, 1, (size_t)count, p->image_file);
		return (uint64_t)wrote;
	}

	uint32_t lba = (uint32_t)(offset / 512u);
	uint32_t cnt = (uint32_t)(count / 512u);
	if (lba + cnt > p->overlay_block_count) {
		return 0;
	}

	for (uint32_t k = 0; k < cnt; k++) {
		uint32_t blk_idx = lba + k;
		if (p->overlay[blk_idx] == NULL) {
			p->overlay[blk_idx] = alloc_block();
			if (p->overlay[blk_idx] == NULL) {
				return 0;
			}
		}
		memcpy(p->overlay[blk_idx], src + (size_t)k * 512u, 512u);
	}
	return count;
}

static void block_file_sync(block_t *blk)
{
	block_file_t *p = block_file_priv(blk);

	if (p == NULL || p->image_file == NULL) {
		return;
	}
	(void)fflush(p->image_file);
}

static block_t *block_create_from_file_impl(const char *path, int use_overlay)
{
	FILE *f = NULL;
	const char *img_path = path;
	long fsize;
	int readonly = 0;

	if (img_path == NULL || img_path[0] == '\0') {
		return NULL;
	}
	f = fopen(img_path, "r+b");
	if (f == NULL) {
		f = fopen(img_path, "rb");
		if (f == NULL) {
			printf("Block image open failed: %s (errno=%d)\n", img_path, errno);
			return NULL;
		}
		readonly = 1;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return NULL;
	}
	fsize = ftell(f);
	if (fsize <= 0 || (fsize % 512) != 0) {
		printf("Block image invalid size: %ld\n", fsize);
		fclose(f);
		return NULL;
	}
	rewind(f);

	block_file_t *obj = new (block_file_t);
	if (obj == NULL) {
		fclose(f);
		return NULL;
	}
	obj->image_file = f;
	obj->size = (uint64_t)fsize;
	obj->readonly = readonly;

	block_t *b = dynamic_cast(block_t)(obj);
	if (b == NULL) {
		delete (obj);
		return NULL;
	}
	b->capacity = block_file_capacity;
	b->read = block_file_read;
	b->write = block_file_write;
	b->sync = block_file_sync;

	if (use_overlay) {
		obj->overlay_block_count = (uint32_t)((uint64_t)fsize / 512ULL);
		obj->overlay = calloc((size_t)obj->overlay_block_count, sizeof(uint8_t *));
		if (obj->overlay == NULL) {
			delete (obj);
			return NULL;
		}
	}

	printf("Using block file: %s (%ld bytes, %s%s)\n", img_path, fsize, readonly ? "read-only" : "read-write", use_overlay ? ", overlay=ram" : "");
	return b;
}

block_t *block_create_from_file(const char *path)
{
	return block_create_from_file_impl(path, 0);
}

static device_t *block_file_probe(driver_t *drv, dtnode_t *n)
{
	(void)drv;
	const char *path = dt_read_string(n, "path", NULL);
	if (path == NULL) {
		return NULL;
	}

	int overlay = dt_read_bool(n, "overlay", 0);
	const char *overlay_s = dt_read_string(n, "overlay", NULL);
	if (!overlay && overlay_s != NULL && overlay_s[0] != '\0' && strcmp(overlay_s, "0") != 0 && strcmp(overlay_s, "false") != 0) {
		overlay = 1;
	}

	block_t *b = block_create_from_file_impl(path, overlay);
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

impl(block_file, driver_t){
        .name = "block-file",
        .probe = block_file_probe,
};
