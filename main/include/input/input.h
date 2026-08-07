#ifndef INPUT_INPUTDEV_H
#define INPUT_INPUTDEV_H

#include "device.h"

struct driver_t;
struct dtnode_t;

class(inputdev_t, device_t)
{
	int (*ioctl)(struct inputdev_t * in, const char *cmd, void *arg);
};

device_t *register_inputdev(inputdev_t *in, struct driver_t *drv, const struct dtnode_t *n);
void unregister_inputdev(inputdev_t *in);
inputdev_t *inputdev_lookup(const char *name);

static inline int inputdev_ioctl(inputdev_t *in, const char *cmd, void *arg)
{
	if (in != NULL && in->ioctl != NULL) {
		return in->ioctl(in, cmd, arg);
	}
	return -1;
}

#endif
