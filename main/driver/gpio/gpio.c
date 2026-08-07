#include "gpio/gpio.h"

#include <stdlib.h>
#include <string.h>

class_impl(gpiochip_t, device_t){};

gpiochip_t *search_gpiochip(int gpio)
{
	device_t *d;
	FOR_EACH_DEVICE(d) {
		gpiochip_t *chip = dynamic_cast(gpiochip_t)(d);
		if (chip == NULL) {
			continue;
		}
		if (gpio >= chip->base && gpio < (chip->base + chip->ngpio)) {
			return chip;
		}
	}
	return NULL;
}

gpiochip_t *gpiochip_lookup(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		return NULL;
	}
	device_t *d = search_device(name);
	if (d == NULL) {
		return NULL;
	}
	return dynamic_cast(gpiochip_t)(d);
}

device_t *register_gpiochip(gpiochip_t *chip, struct driver_t *drv, const struct dtnode_t *n)
{
	if (chip == NULL) {
		return NULL;
	}
	if (chip->base < 0 || chip->ngpio <= 0) {
		return NULL;
	}
	device_t *dev = dynamic_cast(device_t)(chip);
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

void unregister_gpiochip(gpiochip_t *chip)
{
	if (chip == NULL) {
		return;
	}
	device_t *dev = dynamic_cast(device_t)(chip);
	if (dev == NULL) {
		return;
	}
	(void)unregister_device(dev);
}

bool gpiochip_is_valid(int gpio)
{
	return search_gpiochip(gpio) != NULL;
}

void gpiochip_set_cfg(int gpio, int cfg)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->set_cfg != NULL) {
		chip->set_cfg(chip, gpio - chip->base, cfg);
	}
}

int gpiochip_get_cfg(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->get_cfg != NULL) {
		return chip->get_cfg(chip, gpio - chip->base);
	}
	return 0;
}

void gpiochip_set_pull(int gpio, enum gpio_pull_t pull)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->set_pull != NULL) {
		chip->set_pull(chip, gpio - chip->base, pull);
	}
}

enum gpio_pull_t gpiochip_get_pull(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->get_pull != NULL) {
		return chip->get_pull(chip, gpio - chip->base);
	}
	return GPIO_PULL_NONE;
}

void gpiochip_set_drv(int gpio, enum gpio_drv_t drv)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->set_drv != NULL) {
		chip->set_drv(chip, gpio - chip->base, drv);
	}
}

enum gpio_drv_t gpiochip_get_drv(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->get_drv != NULL) {
		return chip->get_drv(chip, gpio - chip->base);
	}
	return GPIO_DRV_WEAK;
}

void gpiochip_set_rate(int gpio, enum gpio_rate_t rate)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->set_rate != NULL) {
		chip->set_rate(chip, gpio - chip->base, rate);
	}
}

enum gpio_rate_t gpiochip_get_rate(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->get_rate != NULL) {
		return chip->get_rate(chip, gpio - chip->base);
	}
	return GPIO_RATE_SLOW;
}

void gpiochip_set_direction(int gpio, enum gpio_direction_t dir)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->set_dir != NULL) {
		chip->set_dir(chip, gpio - chip->base, dir);
	}
}

enum gpio_direction_t gpiochip_get_direction(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->get_dir != NULL) {
		return chip->get_dir(chip, gpio - chip->base);
	}
	return GPIO_DIRECTION_INPUT;
}

void gpiochip_set_value(int gpio, int value)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->set_value != NULL) {
		chip->set_value(chip, gpio - chip->base, value);
	}
}

int gpiochip_get_value(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->get_value != NULL) {
		return chip->get_value(chip, gpio - chip->base);
	}
	return 0;
}

void gpiochip_direction_output(int gpio, int value)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip == NULL) {
		return;
	}
	if (chip->set_dir != NULL) {
		chip->set_dir(chip, gpio - chip->base, GPIO_DIRECTION_OUTPUT);
	}
	if (chip->set_value != NULL) {
		chip->set_value(chip, gpio - chip->base, value);
	}
}

int gpiochip_direction_input(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip == NULL) {
		return 0;
	}
	if (chip->set_dir != NULL) {
		chip->set_dir(chip, gpio - chip->base, GPIO_DIRECTION_INPUT);
	}
	if (chip->get_value != NULL) {
		return chip->get_value(chip, gpio - chip->base);
	}
	return 0;
}

int gpiochip_to_irq(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->to_irq != NULL) {
		return chip->to_irq(chip, gpio - chip->base);
	}
	return -1;
}

int gpiochip_irq_add(int gpio, enum gpio_irq_trigger_t trigger, void (*handler)(void *), void *arg)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->irq_add != NULL) {
		return chip->irq_add(chip, gpio - chip->base, trigger, handler, arg);
	}
	return -1;
}

int gpiochip_irq_remove(int gpio)
{
	gpiochip_t *chip = search_gpiochip(gpio);
	if (chip != NULL && chip->irq_remove != NULL) {
		return chip->irq_remove(chip, gpio - chip->base);
	}
	return -1;
}
