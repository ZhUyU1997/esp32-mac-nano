#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <stdint.h>

#include "device.h"

struct dtnode_t;
struct driver_t;

class(framebuffer_t, device_t)
{
	int width;
	int height;
	int pwidth;
	int pheight;

	void (*setbl)(struct framebuffer_t * fb, int brightness);
	int (*getbl)(struct framebuffer_t * fb);

	void *(*getfb)(struct framebuffer_t * fb);
	int (*wait_vsync)(struct framebuffer_t * fb, uint32_t timeout_ms);
	int (*restart)(struct framebuffer_t * fb);

	void *priv;
};

device_t *register_framebuffer(framebuffer_t *fb, struct driver_t *drv, const struct dtnode_t *n);
void unregister_framebuffer(framebuffer_t *fb);
framebuffer_t *framebuffer_lookup(const char *name);

static inline void framebuffer_set_backlight(framebuffer_t *fb, int brightness)
{
	if (fb == NULL || fb->setbl == NULL) {
		return;
	}
	fb->setbl(fb, brightness);
}

static inline int framebuffer_get_backlight(framebuffer_t *fb)
{
	if (fb == NULL || fb->getbl == NULL) {
		return -1;
	}
	return fb->getbl(fb);
}

static inline void *framebuffer_get_framebuffer(framebuffer_t *fb)
{
	if (fb == NULL || fb->getfb == NULL) {
		return NULL;
	}
	return fb->getfb(fb);
}

static inline int framebuffer_wait_vsync(framebuffer_t *fb, uint32_t timeout_ms)
{
	if (fb == NULL || fb->wait_vsync == NULL) {
		return 0;
	}
	return fb->wait_vsync(fb, timeout_ms);
}

static inline int framebuffer_restart(framebuffer_t *fb)
{
	if (fb == NULL || fb->restart == NULL) {
		return 0;
	}
	return fb->restart(fb);
}

#endif
