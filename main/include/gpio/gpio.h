#ifndef GPIO_GPIO_H
#define GPIO_GPIO_H

#include <stddef.h>
#include <stdbool.h>

#include "device.h"

enum gpio_pull_t {
	GPIO_PULL_NONE = 0,
	GPIO_PULL_UP,
	GPIO_PULL_DOWN,
};

enum gpio_drv_t {
	GPIO_DRV_WEAK = 0,
	GPIO_DRV_WEAKER,
	GPIO_DRV_STRONG,
	GPIO_DRV_STRONGER,
};

enum gpio_rate_t {
	GPIO_RATE_SLOW = 0,
	GPIO_RATE_FAST,
};

enum gpio_direction_t {
	GPIO_DIRECTION_INPUT = 0,
	GPIO_DIRECTION_OUTPUT,
};

enum gpio_irq_trigger_t {
	GPIO_IRQ_NONE = 0,
	GPIO_IRQ_RISING,
	GPIO_IRQ_FALLING,
	GPIO_IRQ_BOTH,
	GPIO_IRQ_LEVEL_HIGH,
	GPIO_IRQ_LEVEL_LOW,
};

struct dtnode_t;
struct driver_t;

class(gpiochip_t, device_t)
{
	int base;
	int ngpio;

	void (*set_cfg)(struct gpiochip_t * chip, int offset, int cfg);
	int (*get_cfg)(struct gpiochip_t * chip, int offset);

	void (*set_pull)(struct gpiochip_t * chip, int offset, enum gpio_pull_t pull);
	enum gpio_pull_t (*get_pull)(struct gpiochip_t * chip, int offset);

	void (*set_drv)(struct gpiochip_t * chip, int offset, enum gpio_drv_t drv);
	enum gpio_drv_t (*get_drv)(struct gpiochip_t * chip, int offset);

	void (*set_rate)(struct gpiochip_t * chip, int offset, enum gpio_rate_t rate);
	enum gpio_rate_t (*get_rate)(struct gpiochip_t * chip, int offset);

	void (*set_dir)(struct gpiochip_t * chip, int offset, enum gpio_direction_t dir);
	enum gpio_direction_t (*get_dir)(struct gpiochip_t * chip, int offset);

	void (*set_value)(struct gpiochip_t * chip, int offset, int value);
	int (*get_value)(struct gpiochip_t * chip, int offset);

	int (*to_irq)(struct gpiochip_t * chip, int offset);

	int (*irq_add)(struct gpiochip_t * chip, int offset, enum gpio_irq_trigger_t trigger, void (*handler)(void *), void *arg);
	int (*irq_remove)(struct gpiochip_t * chip, int offset);
};

device_t *register_gpiochip(gpiochip_t *chip, struct driver_t *drv, const struct dtnode_t *n);
void unregister_gpiochip(gpiochip_t *chip);
gpiochip_t *gpiochip_lookup(const char *name);
gpiochip_t *search_gpiochip(int gpio);

bool gpiochip_is_valid(int gpio);

void gpiochip_set_cfg(int gpio, int cfg);
int gpiochip_get_cfg(int gpio);

void gpiochip_set_pull(int gpio, enum gpio_pull_t pull);
enum gpio_pull_t gpiochip_get_pull(int gpio);

void gpiochip_set_drv(int gpio, enum gpio_drv_t drv);
enum gpio_drv_t gpiochip_get_drv(int gpio);

void gpiochip_set_rate(int gpio, enum gpio_rate_t rate);
enum gpio_rate_t gpiochip_get_rate(int gpio);

void gpiochip_set_direction(int gpio, enum gpio_direction_t dir);
enum gpio_direction_t gpiochip_get_direction(int gpio);

void gpiochip_set_value(int gpio, int value);
int gpiochip_get_value(int gpio);

void gpiochip_direction_output(int gpio, int value);
int gpiochip_direction_input(int gpio);

int gpiochip_to_irq(int gpio);

int gpiochip_irq_add(int gpio, enum gpio_irq_trigger_t trigger, void (*handler)(void *), void *arg);
int gpiochip_irq_remove(int gpio);

#endif
