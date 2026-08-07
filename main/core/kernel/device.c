#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "device.h"
#include "dt.h"

class_impl(device_t, object_t){};

LIST_HEAD(g_dev);

constructor(device_t)
{
	init_list_head(&this->head);
}

destructor(device_t)
{
	free(this->name);
}

char *device_strdup(const char *src)
{
	if (src == NULL) {
		return NULL;
	}
	size_t n = strlen(src);
	char *s = malloc(n + 1);
	if (s == NULL) {
		return NULL;
	}
	memcpy(s, src, n);
	s[n] = '\0';
	return s;
}

char *alloc_device_name(const char *name, int id)
{
	const char *base = (name != NULL && name[0] != '\0') ? name : "dev";
	if (id < 0) {
		id = 0;
	}

	char buf[256];
	for (;;) {
		int n = snprintf(buf, sizeof(buf), "%s.%d", base, id++);
		if (n <= 0 || (size_t)n >= sizeof(buf)) {
			return NULL;
		}
		if (search_device(buf) == NULL) {
			break;
		}
	}
	return device_strdup(buf);
}

void free_device_name(char *name)
{
	free(name);
}

int device_setup(device_t *dev, const char *name, struct driver_t *driver)
{
	if (dev == NULL || name == NULL || name[0] == '\0') {
		return 0;
	}
	free(dev->name);
	dev->name = device_strdup(name);
	if (dev->name == NULL) {
		return 0;
	}
	dev->driver = driver;
	return 1;
}

int device_setup_from_dtnode(device_t *dev, struct driver_t *driver, const struct dtnode_t *n)
{
	if (dev == NULL || n == NULL) {
		return 0;
	}

	const char *name = dt_read_string(n, "name", NULL);
	char *new_name = (name != NULL && name[0] != '\0') ? device_strdup(name) : alloc_device_name(dt_read_name(n), dt_read_id(n));
	if (new_name == NULL) {
		free(new_name);
		return 0;
	}

	free(dev->name);
	dev->name = new_name;
	dev->driver = driver;
	return 1;
}

int device_register(device_t *dev)
{
	if (dev == NULL || dev->name == NULL || dev->name[0] == '\0') {
		return 0;
	}
	if (search_device(dev->name) != NULL) {
		return 0;
	}
	list_add_tail(&dev->head, &g_dev);
	return 1;
}

int unregister_device(device_t *dev)
{
	if (dev == NULL || dev->name == NULL || dev->name[0] == '\0') {
		return 0;
	}

	list_del(&dev->head);
	return 1;
}

device_t *register_device(const char *name, struct driver_t *driver)
{
	if (name == NULL || name[0] == '\0') {
		return NULL;
	}
	if (search_device(name) != NULL) {
		return NULL;
	}

	device_t *dev = new (device_t);
	if (dev == NULL) {
		return NULL;
	}
	if (!device_setup(dev, name, driver)) {
		delete (dev);
		return NULL;
	}
	if (!device_register(dev)) {
		delete (dev);
		return NULL;
	}
	return dev;
}

device_t *search_device(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		return NULL;
	}
	device_t *d;
	FOR_EACH_DEVICE(d) {
		if (d->name != NULL && strcmp(d->name, name) == 0) {
			return d;
		}
	}
	return NULL;
}
