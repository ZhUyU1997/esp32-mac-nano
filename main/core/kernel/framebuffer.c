#include "framebuffer.h"

#include "common/log.h"

class_impl(framebuffer_t, device_t){};

device_t *register_framebuffer(framebuffer_t *fb, struct driver_t *drv, const struct dtnode_t *n)
{
	if (fb == NULL) {
		return NULL;
	}
	if (fb->width <= 0 || fb->height <= 0) {
		return NULL;
	}
	if (fb->getfb == NULL || fb->wait_vsync == NULL) {
		return NULL;
	}
	device_t *dev = dynamic_cast(device_t)(fb);
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

void unregister_framebuffer(framebuffer_t *fb)
{
	if (fb == NULL) {
		return;
	}
	device_t *dev = dynamic_cast(device_t)(fb);
	if (dev == NULL) {
		return;
	}
	(void)unregister_device(dev);
}

framebuffer_t *framebuffer_lookup(const char *name)
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
	framebuffer_t *fb = dynamic_cast(framebuffer_t)(d);
	if (fb == NULL) {
		LOGE("cast failed: %s", name);
		return NULL;
	}
	return fb;
}
