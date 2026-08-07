#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

#include "device.h"

struct dtnode_t;
struct driver_t;

class(sound_t, device_t)
{
	int (*write)(struct sound_t * snd, const uint16_t *buf, unsigned cnt);
	void (*close)(struct sound_t * snd);
	void (*output_task)(struct sound_t * snd);
	int snd_hz;
};

device_t *register_sound(sound_t *snd, struct driver_t *drv, const struct dtnode_t *n);
void unregister_sound(sound_t *snd);
sound_t *sound_lookup(const char *name);

static inline int sound_write(sound_t *snd, const uint16_t *buf, unsigned cnt)
{
	if (snd == NULL || snd->write == NULL) {
		return -1;
	}
	return snd->write(snd, buf, cnt);
}

static inline void sound_close(sound_t *snd)
{
	if (snd != NULL && snd->close != NULL) {
		snd->close(snd);
	}
}

#endif
