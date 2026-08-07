#include "sound.h"

#include "common/log.h"

class_impl(sound_t, device_t){};

device_t *register_sound(sound_t *snd, struct driver_t *drv, const struct dtnode_t *n)
{
	if (snd == NULL || snd->write == NULL || snd->output_task == NULL) {
		return NULL;
	}
	device_t *dev = dynamic_cast(device_t)(snd);
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

void unregister_sound(sound_t *snd)
{
	if (snd == NULL) {
		return;
	}
	device_t *dev = dynamic_cast(device_t)(snd);
	if (dev == NULL) {
		return;
	}
	(void)unregister_device(dev);
}

sound_t *sound_lookup(const char *name)
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
	sound_t *snd = dynamic_cast(sound_t)(d);
	if (snd == NULL) {
		LOGE("cast failed: %s", name);
		return NULL;
	}
	return snd;
}
