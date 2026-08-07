#ifndef DRIVER_H
#define DRIVER_H

#include <stddef.h>

#include "iface.h"

struct device_t;
struct dtnode_t;

typedef struct driver_t {
	struct list_head list;
	const char *ifname;
	const char *name;
	struct device_t *(*probe)(struct driver_t *drv, struct dtnode_t *n);
	void (*remove)(struct device_t *dev);
	void (*suspend)(struct device_t *dev);
	void (*resume)(struct device_t *dev);
} driver_t;

#define driver_t_iface_name "driver"

int register_driver(driver_t *drv);
int unregister_driver(driver_t *drv);
driver_t *search_driver(const char *name, size_t len);
struct list_head *get_driver_list(void);
void remove_device(struct device_t *dev);

#endif
