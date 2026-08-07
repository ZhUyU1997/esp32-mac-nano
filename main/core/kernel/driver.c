#include <string.h>

#include "driver.h"
#include "iface.h"
#include "device.h"

static void driver_nop(device_t *dev)
{
	(void)dev;
}

static struct list_head *driver_list(void)
{
	return iface_get_list(driver_t_iface_name);
}

int register_driver(driver_t *drv)
{
	if (drv == NULL || drv->name == NULL || drv->probe == NULL) {
		return -1;
	}

	if (drv->remove == NULL) {
		drv->remove = driver_nop;
	}
	if (drv->suspend == NULL) {
		drv->suspend = driver_nop;
	}
	if (drv->resume == NULL) {
		drv->resume = driver_nop;
	}

	drv->ifname = driver_t_iface_name;
	return iface_register((iface_base_t *)drv);
}

int unregister_driver(driver_t *drv)
{
	if (drv == NULL) {
		return -1;
	}
	return iface_unregister((iface_base_t *)drv);
}

driver_t *search_driver(const char *name, size_t len)
{
	iface_base_t *b = iface_search(driver_t_iface_name, name, len);
	return (driver_t *)b;
}

struct list_head *get_driver_list(void)
{
	return driver_list();
}

void remove_device(device_t *dev)
{
	if (dev == NULL) {
		return;
	}
	if (dev->driver != NULL && dev->driver->remove != NULL) {
		dev->driver->remove(dev);
	}
}
