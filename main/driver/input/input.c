#include "input/input.h"

#include "common/log.h"

class_impl(inputdev_t, device_t){};

device_t *register_inputdev(inputdev_t *in, struct driver_t *drv, const struct dtnode_t *n)
{
	if (in == NULL) {
		return NULL;
	}
	device_t *dev = dynamic_cast(device_t)(in);
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

void unregister_inputdev(inputdev_t *in)
{
	if (in == NULL) {
		return;
	}
	device_t *dev = dynamic_cast(device_t)(in);
	if (dev == NULL) {
		return;
	}
	(void)unregister_device(dev);
}

inputdev_t *inputdev_lookup(const char *name)
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
	inputdev_t *in = dynamic_cast(inputdev_t)(d);
	if (in == NULL) {
		LOGE("cast failed: %s", name);
		return NULL;
	}
	return in;
}
