#ifndef DEVICE_H
#define DEVICE_H

#include "common/class.h"

struct driver_t;
struct dtnode_t;

class(device_t, object_t)
{
	struct list_head head;
	char *name;
	struct driver_t *driver;
};

extern struct list_head g_dev;

#define FOR_EACH_DEVICE(pos) list_for_each_entry(pos, &g_dev, head)

int device_setup(device_t *dev, const char *name, struct driver_t *driver);
int device_setup_from_dtnode(device_t *dev, struct driver_t *driver, const struct dtnode_t *n);
int device_register(device_t *dev);
int unregister_device(device_t *dev);
device_t *register_device(const char *name, struct driver_t *driver);
device_t *search_device(const char *name);

char *device_strdup(const char *src);
char *alloc_device_name(const char *name, int id);
void free_device_name(char *name);

#endif
