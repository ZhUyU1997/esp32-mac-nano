#include <stdlib.h>

#include "driver.h"
#include "dt.h"
#include "gpio/gpio.h"
#include "input/input.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "input.h"

typedef struct {
	int gpio;
	int gpiocfg;
	int active_low;
	input_keycode_t keycode;
	int state;        /* confirmed stable state (0=released, 1=pressed) */
	int pending;      /* current raw sample */
	int stable_cnt;   /* consecutive consistent samples */
} gpio_key_t;

/* Samples required to confirm a state change (3 x poll-interval = debounce). */
#define KEY_DEBOUNCE_SAMPLES 3

class(key_gpio_polled_t, inputdev_t)
{
	gpio_key_t *keys;
	int nkeys;
	int interval_ms;
	TaskHandle_t task;
};

class_impl(key_gpio_polled_t, inputdev_t){};

static void key_gpio_polled_task(void *arg)
{
	key_gpio_polled_t *p = (key_gpio_polled_t *)arg;
	if (p == NULL) {
		vTaskDelete(NULL);
		return;
	}

	for (;;) {
		for (int i = 0; i < p->nkeys; i++) {
			int val = gpiochip_get_value(p->keys[i].gpio);
			int raw = p->keys[i].active_low ? (val ? 0 : 1) : (val ? 1 : 0);
			if (raw != p->keys[i].pending) {
				/* level changed: restart confirmation count */
				p->keys[i].pending = raw;
				p->keys[i].stable_cnt = 1;
			} else if (raw != p->keys[i].state) {
				/* same sample repeatedly: confirm after debounce window */
				if (++p->keys[i].stable_cnt >= KEY_DEBOUNCE_SAMPLES) {
					p->keys[i].state = raw;
					input_post_key(p->keys[i].keycode, raw ? 1 : 0);
					p->keys[i].stable_cnt = 0;
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS((uint32_t)p->interval_ms));
	}
}

static int key_gpio_polled_ioctl(inputdev_t *in, const char *cmd, void *arg)
{
	(void)in;
	(void)cmd;
	(void)arg;
	return -1;
}

static device_t *key_gpio_polled_probe(driver_t *drv, dtnode_t *n)
{
	if (drv == NULL || n == NULL) {
		return NULL;
	}

	int nkeys = dt_read_array_length(n, "keys");
	if (nkeys <= 0) {
		return NULL;
	}

	key_gpio_polled_t *obj = new (key_gpio_polled_t);
	if (obj == NULL) {
		return NULL;
	}
	inputdev_t *in = dynamic_cast(inputdev_t)(obj);
	if (in == NULL) {
		delete (obj);
		return NULL;
	}

	gpio_key_t *keys = (gpio_key_t *)calloc((size_t)nkeys, sizeof(*keys));
	if (keys == NULL) {
		delete (obj);
		return NULL;
	}

	for (int i = 0; i < nkeys; i++) {
		dtnode_t o;
		if (dt_read_array_object(n, "keys", i, &o) == NULL) {
			free(keys);
			delete (obj);
			return NULL;
		}
		keys[i].gpio = dt_read_int(&o, "gpio", -1);
		keys[i].gpiocfg = dt_read_int(&o, "gpio-config", -1);
		keys[i].active_low = dt_read_bool(&o, "active-low", 0);
		keys[i].keycode = (input_keycode_t)dt_read_int(&o, "key-code", 0);
		if (keys[i].gpio < 0) {
			free(keys);
			delete (obj);
			return NULL;
		}

		gpiochip_set_direction(keys[i].gpio, GPIO_DIRECTION_INPUT);
		gpiochip_set_pull(keys[i].gpio, keys[i].active_low ? GPIO_PULL_UP : GPIO_PULL_DOWN);
		if (keys[i].gpiocfg >= 0) {
			gpiochip_set_cfg(keys[i].gpio, keys[i].gpiocfg);
		}
		keys[i].state = gpiochip_get_value(keys[i].gpio);
		keys[i].state = keys[i].active_low ? (keys[i].state ? 0 : 1) : (keys[i].state ? 1 : 0);
		keys[i].pending = keys[i].state;
		keys[i].stable_cnt = 0;
	}

	obj->keys = keys;
	obj->nkeys = nkeys;
	obj->interval_ms = dt_read_int(n, "poll-interval-ms", 20);
	if (obj->interval_ms <= 0) {
		obj->interval_ms = 20;
	}
	obj->task = NULL;

	in->ioctl = key_gpio_polled_ioctl;

	device_t *dev = register_inputdev(in, drv, n);
	if (dev == NULL) {
		free(keys);
		delete (obj);
		return NULL;
	}

	if (xTaskCreatePinnedToCore(&key_gpio_polled_task, "key_gpio_polled", 2048, obj, 4, &obj->task, 0) != pdPASS) {
		unregister_inputdev(in);
		free(keys);
		delete (obj);
		return NULL;
	}

	return dev;
}

static void key_gpio_polled_remove(device_t *dev)
{
	inputdev_t *in = dynamic_cast(inputdev_t)(dev);
	key_gpio_polled_t *obj = dynamic_cast(key_gpio_polled_t)(dev);
	if (in != NULL) {
		unregister_inputdev(in);
	}
	if (obj != NULL) {
		if (obj->task != NULL) {
			vTaskDelete(obj->task);
			obj->task = NULL;
		}
		free(obj->keys);
		obj->keys = NULL;
		obj->nkeys = 0;
		delete (obj);
	}
}

impl(key_gpio_polled, driver_t){
        .name = "key-gpio-polled",
        .probe = key_gpio_polled_probe,
        .remove = key_gpio_polled_remove,
};
