#include "block/block.h"
#include "common/log.h"

class_impl(block_t, device_t){};

device_t *register_block(block_t *blk, struct driver_t *drv, const struct dtnode_t *n)
{
	if (blk == NULL) {
		return NULL;
	}
	if (blk->capacity == NULL || blk->read == NULL || blk->write == NULL || blk->sync == NULL) {
		return NULL;
	}
	device_t *dev = dynamic_cast(device_t)(blk);
	if (dev == NULL) {
		return NULL;
	}
	if (!device_setup_from_dtnode(dev, drv, n)) {
		return NULL;
	}
	if (!device_register(dev)) {
		return NULL;
	}
	return dev;
}

void unregister_block(block_t *blk)
{
	if (blk == NULL) {
		return;
	}
	device_t *dev = dynamic_cast(device_t)(blk);
	if (dev == NULL) {
		return;
	}
	(void)unregister_device(dev);
}

uint64_t block_capacity(block_t *blk)
{
	if (blk == NULL || blk->capacity == NULL) {
		return 0;
	}
	return blk->capacity(blk);
}

uint64_t block_read(block_t *blk, uint8_t *dst, uint64_t offset, uint64_t count)
{
	if (blk == NULL || blk->read == NULL || dst == NULL) {
		return 0;
	}
	return blk->read(blk, dst, offset, count);
}

uint64_t block_write(block_t *blk, const uint8_t *src, uint64_t offset, uint64_t count)
{
	if (blk == NULL || blk->write == NULL || src == NULL) {
		return 0;
	}
	return blk->write(blk, src, offset, count);
}

void block_sync(block_t *blk)
{
	if (blk == NULL || blk->sync == NULL) {
		return;
	}
	blk->sync(blk);
}

block_t *block_lookup(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		LOGE("invalid name");
		return NULL;
	}
	device_t *d = search_device(name);
	if (d == NULL) {
		LOGE("not found: %s", name);
		return NULL;
	}
	block_t *b = dynamic_cast(block_t)(d);
	if (b == NULL) {
		LOGE("cast failed: %s", name);
		return NULL;
	}
	return b;
}
