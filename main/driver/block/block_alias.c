#include <string.h>

#include "block/block.h"
#include "driver.h"
#include "device.h"
#include "dt.h"

class(block_alias_t, block_t)
{
	block_t *target;
};

class_impl(block_alias_t, block_t){};

static block_alias_t *block_alias_priv(block_t *blk)
{
	return dynamic_cast(block_alias_t)(blk);
}

static uint64_t block_alias_capacity(block_t *blk)
{
	block_alias_t *a = block_alias_priv(blk);
	if (a == NULL || a->target == NULL || a->target->capacity == NULL) {
		return 0;
	}
	return a->target->capacity(a->target);
}

static uint64_t block_alias_read(block_t *blk, uint8_t *dst, uint64_t offset, uint64_t count)
{
	block_alias_t *a = block_alias_priv(blk);
	if (a == NULL || a->target == NULL || a->target->read == NULL) {
		return 0;
	}
	return a->target->read(a->target, dst, offset, count);
}

static uint64_t block_alias_write(block_t *blk, const uint8_t *src, uint64_t offset, uint64_t count)
{
	block_alias_t *a = block_alias_priv(blk);
	if (a == NULL || a->target == NULL || a->target->write == NULL) {
		return 0;
	}
	return a->target->write(a->target, src, offset, count);
}

static void block_alias_sync(block_t *blk)
{
	block_alias_t *a = block_alias_priv(blk);
	if (a == NULL || a->target == NULL || a->target->sync == NULL) {
		return;
	}
	a->target->sync(a->target);
}

static device_t *probe_block_alias(driver_t *drv, dtnode_t *n)
{
	const char *target = dt_read_string(n, "target", NULL);
	const char *fallback = dt_read_string(n, "fallback", NULL);

	block_t *target_blk = NULL;
	if (target != NULL) {
		target_blk = block_lookup(target);
	}
	if (target_blk == NULL && fallback != NULL) {
		target_blk = block_lookup(fallback);
	}
	if (target_blk == NULL) {
		return NULL;
	}

	block_alias_t *obj = new (block_alias_t);
	if (obj == NULL) {
		return NULL;
	}
	obj->target = target_blk;

	block_t *blk = dynamic_cast(block_t)(obj);
	if (blk == NULL) {
		delete (obj);
		return NULL;
	}
	blk->capacity = block_alias_capacity;
	blk->read = block_alias_read;
	blk->write = block_alias_write;
	blk->sync = block_alias_sync;

	device_t *dev = register_block(blk, drv, n);
	if (dev == NULL) {
		delete (obj);
		return NULL;
	}
	return dev;
}

impl(block_alias, driver_t){
        .name = "block-alias",
        .probe = probe_block_alias,
};
