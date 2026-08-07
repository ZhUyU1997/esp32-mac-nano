#include <stdlib.h>

#include "driver.h"
#include "dt.h"
#include "gpio/gpio.h"

#include "driver/gpio.h"
#include "esp_err.h"

class(gpio_s3_t, gpiochip_t)
{
	int *cfg;
	enum gpio_pull_t *pull;
	enum gpio_drv_t *drv;
	enum gpio_rate_t *rate;
	enum gpio_direction_t *dir;
};

class_impl(gpio_s3_t, gpiochip_t){};

destructor(gpio_s3_t)
{
	free(this->cfg);
	free(this->pull);
	free(this->drv);
	free(this->rate);
	free(this->dir);
}

#define gpio_s3_priv(chip) dynamic_cast(gpio_s3_t)(chip)

static bool gpio_s3_validate(gpiochip_t *chip, int offset, gpio_num_t *out_gpio)
{
	if (chip == NULL || out_gpio == NULL) {
		return false;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return false;
	}
	int gpio_i = chip->base + offset;
	if (gpio_i < 0 || gpio_i >= (int)GPIO_NUM_MAX) {
		return false;
	}
	*out_gpio = (gpio_num_t)gpio_i;
	return true;
}

static void gpio_s3_set_cfg(gpiochip_t *chip, int offset, int cfg)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->cfg == NULL) {
		return;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return;
	}
	p->cfg[offset] = cfg;
}

static int gpio_s3_get_cfg(gpiochip_t *chip, int offset)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->cfg == NULL) {
		return 0;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return 0;
	}
	return p->cfg[offset];
}

static void gpio_s3_set_pull(gpiochip_t *chip, int offset, enum gpio_pull_t pull)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->pull == NULL) {
		return;
	}
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return;
	}

	gpio_pull_mode_t mode = GPIO_FLOATING;
	switch (pull) {
	case GPIO_PULL_UP:
		mode = GPIO_PULLUP_ONLY;
		break;
	case GPIO_PULL_DOWN:
		mode = GPIO_PULLDOWN_ONLY;
		break;
	case GPIO_PULL_NONE:
	default:
		mode = GPIO_FLOATING;
		break;
	}
	(void)gpio_set_pull_mode(gpio, mode);
	p->pull[offset] = pull;
}

static enum gpio_pull_t gpio_s3_get_pull(gpiochip_t *chip, int offset)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->pull == NULL) {
		return GPIO_PULL_NONE;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return GPIO_PULL_NONE;
	}
	return p->pull[offset];
}

static void gpio_s3_set_drv(gpiochip_t *chip, int offset, enum gpio_drv_t drv)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->drv == NULL) {
		return;
	}
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return;
	}

	gpio_drive_cap_t cap = GPIO_DRIVE_CAP_0;
	switch (drv) {
	case GPIO_DRV_WEAKER:
		cap = GPIO_DRIVE_CAP_1;
		break;
	case GPIO_DRV_STRONG:
		cap = GPIO_DRIVE_CAP_2;
		break;
	case GPIO_DRV_STRONGER:
		cap = GPIO_DRIVE_CAP_3;
		break;
	case GPIO_DRV_WEAK:
	default:
		cap = GPIO_DRIVE_CAP_0;
		break;
	}
	(void)gpio_set_drive_capability(gpio, cap);
	p->drv[offset] = drv;
}

static enum gpio_drv_t gpio_s3_get_drv(gpiochip_t *chip, int offset)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->drv == NULL) {
		return GPIO_DRV_WEAK;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return GPIO_DRV_WEAK;
	}
	return p->drv[offset];
}

static void gpio_s3_set_rate(gpiochip_t *chip, int offset, enum gpio_rate_t rate)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->rate == NULL) {
		return;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return;
	}
	p->rate[offset] = rate;
}

static enum gpio_rate_t gpio_s3_get_rate(gpiochip_t *chip, int offset)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->rate == NULL) {
		return GPIO_RATE_SLOW;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return GPIO_RATE_SLOW;
	}
	return p->rate[offset];
}

static void gpio_s3_set_dir(gpiochip_t *chip, int offset, enum gpio_direction_t dir)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->dir == NULL) {
		return;
	}
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return;
	}

	gpio_mode_t mode = GPIO_MODE_INPUT;
	switch (dir) {
	case GPIO_DIRECTION_OUTPUT:
		mode = GPIO_MODE_OUTPUT;
		break;
	case GPIO_DIRECTION_INPUT:
	default:
		mode = GPIO_MODE_INPUT;
		break;
	}
	(void)gpio_set_direction(gpio, mode);
	p->dir[offset] = dir;
}

static enum gpio_direction_t gpio_s3_get_dir(gpiochip_t *chip, int offset)
{
	gpio_s3_t *p = gpio_s3_priv(chip);
	if (p == NULL || p->dir == NULL) {
		return GPIO_DIRECTION_INPUT;
	}
	if (offset < 0 || offset >= chip->ngpio) {
		return GPIO_DIRECTION_INPUT;
	}
	return p->dir[offset];
}

static void gpio_s3_set_value(gpiochip_t *chip, int offset, int value)
{
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return;
	}
	(void)gpio_set_level(gpio, value ? 1 : 0);
}

static int gpio_s3_get_value(gpiochip_t *chip, int offset)
{
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return 0;
	}
	return gpio_get_level(gpio) ? 1 : 0;
}

static int gpio_s3_to_irq(gpiochip_t *chip, int offset)
{
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return -1;
	}
	return (int)gpio;
}

static int gpio_s3_irq_add(gpiochip_t *chip, int offset, enum gpio_irq_trigger_t trigger, void (*handler)(void *), void *arg)
{
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return -1;
	}

	gpio_int_type_t intr = GPIO_INTR_DISABLE;
	switch (trigger) {
	case GPIO_IRQ_RISING:
		intr = GPIO_INTR_POSEDGE;
		break;
	case GPIO_IRQ_FALLING:
		intr = GPIO_INTR_NEGEDGE;
		break;
	case GPIO_IRQ_BOTH:
		intr = GPIO_INTR_ANYEDGE;
		break;
	case GPIO_IRQ_LEVEL_HIGH:
		intr = GPIO_INTR_HIGH_LEVEL;
		break;
	case GPIO_IRQ_LEVEL_LOW:
		intr = GPIO_INTR_LOW_LEVEL;
		break;
	case GPIO_IRQ_NONE:
	default:
		intr = GPIO_INTR_DISABLE;
		break;
	}

	esp_err_t err = gpio_set_intr_type(gpio, intr);
	if (err != ESP_OK) {
		return (int)err;
	}

	err = gpio_isr_handler_add(gpio, (gpio_isr_t)handler, arg);
	if (err == ESP_ERR_INVALID_STATE) {
		err = gpio_install_isr_service(0);
		if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
			return (int)err;
		}
		err = gpio_isr_handler_add(gpio, (gpio_isr_t)handler, arg);
	}
	return (int)err;
}

static int gpio_s3_irq_remove(gpiochip_t *chip, int offset)
{
	gpio_num_t gpio;
	if (!gpio_s3_validate(chip, offset, &gpio)) {
		return -1;
	}

	(void)gpio_set_intr_type(gpio, GPIO_INTR_DISABLE);
	esp_err_t err = gpio_isr_handler_remove(gpio);
	return (int)err;
}

static device_t *gpio_s3_probe(driver_t *drv, dtnode_t *n)
{
	if (drv == NULL || n == NULL) {
		return NULL;
	}

	int base = dt_read_int(n, "base", 0);
	int ngpio = dt_read_int(n, "ngpio", (int)GPIO_NUM_MAX);
	if (base < 0) {
		return NULL;
	}
	if (ngpio <= 0) {
		return NULL;
	}

	gpio_s3_t *obj = new (gpio_s3_t);
	if (obj == NULL) {
		return NULL;
	}
	gpiochip_t *chip = dynamic_cast(gpiochip_t)(obj);
	if (chip == NULL) {
		delete (obj);
		return NULL;
	}

	obj->cfg = (int *)calloc((size_t)ngpio, sizeof(*obj->cfg));
	obj->pull = (enum gpio_pull_t *)calloc((size_t)ngpio, sizeof(*obj->pull));
	obj->drv = (enum gpio_drv_t *)calloc((size_t)ngpio, sizeof(*obj->drv));
	obj->rate = (enum gpio_rate_t *)calloc((size_t)ngpio, sizeof(*obj->rate));
	obj->dir = (enum gpio_direction_t *)calloc((size_t)ngpio, sizeof(*obj->dir));
	if (obj->cfg == NULL || obj->pull == NULL || obj->drv == NULL || obj->rate == NULL || obj->dir == NULL) {
		delete (obj);
		return NULL;
	}

	for (int i = 0; i < ngpio; i++) {
		obj->pull[i] = GPIO_PULL_NONE;
		obj->drv[i] = GPIO_DRV_WEAK;
		obj->rate[i] = GPIO_RATE_SLOW;
		obj->dir[i] = GPIO_DIRECTION_INPUT;
	}

	chip->base = base;
	chip->ngpio = ngpio;
	chip->set_cfg = gpio_s3_set_cfg;
	chip->get_cfg = gpio_s3_get_cfg;
	chip->set_pull = gpio_s3_set_pull;
	chip->get_pull = gpio_s3_get_pull;
	chip->set_drv = gpio_s3_set_drv;
	chip->get_drv = gpio_s3_get_drv;
	chip->set_rate = gpio_s3_set_rate;
	chip->get_rate = gpio_s3_get_rate;
	chip->set_dir = gpio_s3_set_dir;
	chip->get_dir = gpio_s3_get_dir;
	chip->set_value = gpio_s3_set_value;
	chip->get_value = gpio_s3_get_value;
	chip->to_irq = gpio_s3_to_irq;
	chip->irq_add = gpio_s3_irq_add;
	chip->irq_remove = gpio_s3_irq_remove;

	device_t *dev = register_gpiochip(chip, drv, n);
	if (dev == NULL) {
		delete (obj);
		return NULL;
	}
	return dev;
}

static void gpio_s3_remove(device_t *dev)
{
	gpiochip_t *chip = dynamic_cast(gpiochip_t)(dev);
	if (chip != NULL) {
		unregister_gpiochip(chip);
	}
	gpio_s3_t *obj = dynamic_cast(gpio_s3_t)(dev);
	if (obj != NULL) {
		delete (obj);
	}
}

impl(gpio_s3, driver_t){
        .name = "gpio-s3",
        .probe = gpio_s3_probe,
        .remove = gpio_s3_remove,
};
