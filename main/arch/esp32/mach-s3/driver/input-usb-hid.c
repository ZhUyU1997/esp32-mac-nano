/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "hal/usb_serial_jtag_ll.h"
#include "usb/usb_host.h"
#include "errno.h"
#include "driver/gpio.h"

#include "usb/hid_host.h"
#include "hid_report_parser.h"
#include "hid_usage.h"
#include "input.h"

/* GPIO Pin number for quit from example logic */
#define APP_QUIT_PIN GPIO_NUM_0

static const char *TAG = "usb-hid";

QueueHandle_t app_event_queue = NULL;

/**
 * @brief APP event group
 *
 * Application logic can be different. There is a one among other ways to distinguish the
 * event by application event group.
 * In this example we have two event groups:
 * APP_EVENT            - General event, which is APP_QUIT_PIN press event (Generally, it is IO0).
 * APP_EVENT_HID_HOST   - HID Host Driver event, such as device connection/disconnection or input report.
 */
typedef enum { APP_EVENT = 0, APP_EVENT_HID_HOST } app_event_group_t;

/**
 * @brief APP event queue
 *
 * This event is used for delivering the HID Host event from callback to a task.
 */
typedef struct {
	app_event_group_t event_group;
	/* HID Host - Device related info */
	struct {
		hid_host_device_handle_t handle;
		hid_host_driver_event_t event;
		void *arg;
	} hid_host_device;
} app_event_queue_t;

/**
 * @brief HID Protocol string names
 */
static const char *hid_proto_name_str[] = {"NONE", "KEYBOARD", "MOUSE"};

/**
 * @brief USB HID Host Keyboard Interface report callback handler
 *
 * Iterates report->field[] and dispatches by usage:
 *   Modifiers (usage 0xe0-0xe7) → modifier byte
 *   Key slots  (usage 0-5)      → keys[6]
 *
 * @param[in] hdev         Parsed HID device
 * @param[in] report       Report matching data[0] (or report_id_hash[0])
 * @param[in] payload      Report data after Report ID byte
 * @param[in] payload_len  Length of payload in bytes
 */
static void hid_host_keyboard_report_callback(struct hid_device *hdev,
                                              const struct hid_report *report,
                                              const uint8_t *payload, size_t payload_len)
{
	uint8_t mod = 0;
	uint8_t keys[6] = {0};

	for (unsigned i = 0; i < report->maxfield; i++) {
		struct hid_field *f = report->field[i];
		if (f->flags & HID_MAIN_ITEM_CONSTANT) continue;
		if (f->usage_page != 0x07) continue;

		unsigned u = f->usage_id;
		if (u >= 0xe0 && u <= 0xe7) {
			if (hid_field_extract(f, payload, payload_len, 0))
				mod |= (1u << (u - 0xe0));
		} else if (u < 6) {
			keys[u] = (uint8_t)hid_field_extract(f, payload, payload_len, 0);
		}
	}

	input_report_keyboard(mod, keys);
}

/**
 * @brief USB HID Host Mouse Interface report callback handler
 *
 * Iterates report->field[] and dispatches by usage:
 *   Buttons (usage 1-2) → input_report_mouse_button()
 *   X/Y              → input_post_mouse_move_rel()
 *
 * @param[in] hdev         Parsed HID device
 * @param[in] report       Report matching data[0] (or report_id_hash[0])
 * @param[in] payload      Report data after Report ID byte
 * @param[in] payload_len  Length of payload in bytes
 */
static void hid_host_mouse_report_callback(struct hid_device *hdev,
                                           const struct hid_report *report,
                                           const uint8_t *payload, size_t payload_len)
{
	unsigned btn_state = 0;
	int dx = 0, dy = 0;

	for (unsigned i = 0; i < report->maxfield; i++) {
		struct hid_field *f = report->field[i];
		if (f->flags & HID_MAIN_ITEM_CONSTANT) continue;
		if (f->usage_page == HID_UP_BUTTON) {
			if (f->usage_id >= HID_BTN_1 && f->usage_id <= HID_BTN_3)
				if (hid_field_extract(f, payload, payload_len, 0))
					btn_state |= (1u << (f->usage_id - 1));
		} else if (f->usage_page == HID_UP_GENDESK) {
			if (f->usage_id == HID_GD_X)
				dx = (int)hid_field_extract(f, payload, payload_len, 0);
			else if (f->usage_id == HID_GD_Y)
				dy = (int)hid_field_extract(f, payload, payload_len, 0);
		}
	}

	input_report_mouse_button(INPUT_MOUSE_BTN_LEFT,  (btn_state & 1u) ? 1u : 0u);
	input_report_mouse_button(INPUT_MOUSE_BTN_RIGHT, (btn_state & 2u) ? 1u : 0u);
	input_report_mouse_button(INPUT_MOUSE_BTN_MIDDLE,(btn_state & 4u) ? 1u : 0u);
	if (dx != 0 || dy != 0)
		input_post_mouse_move_rel(dx, dy);
}

/**
 * hid_input_report — lookup + dispatch driven by Application collection.
 */
static int hid_input_report(struct hid_device *hdev, hid_report_type_t type,
                            const uint8_t *data, size_t data_len)
{
	struct hid_report_enum *report_enum = hdev->report_enum[type - 1];
	struct hid_report *report;

	if (!report_enum) return -1;
	if (report_enum->numbered && data_len == 0) return -1;

	/* Find report by data[0] (report ID).
	 * report_id_hash[] only has HID_MAX_IDS (16) entries; the parser silently
	 * drops report IDs >= 16 at parse time, but some devices (e.g. cheap mice
	 * with private protocol frames) still send such reports at runtime.
	 * Indexing past the array reads heap garbage -> crash on report->application
	 * below, so drop these reports here. */
	if (report_enum->numbered) {
		if (data[0] >= HID_MAX_IDS) {
			ESP_LOGW(TAG, "unsupported report ID 0x%02x dropped (len=%u)",
			         data[0], (unsigned)data_len);
			return -1;
		}
		report = report_enum->report_id_hash[data[0]];
	} else {
		report = report_enum->report_id_hash[0];
	}

	if (!report) return -1;

	const uint8_t *payload = report_enum->numbered ? (data + 1) : data;
	size_t payload_len = report_enum->numbered ? (data_len - 1) : data_len;

	unsigned app = report->application;

	if (app == (((unsigned)HID_UP_GENDESK << 16) | HID_GD_KEYBOARD))
		hid_host_keyboard_report_callback(hdev, report, payload, payload_len);
	else if (app == (((unsigned)HID_UP_GENDESK << 16) | HID_GD_MOUSE))
		hid_host_mouse_report_callback(hdev, report, payload, payload_len);

	return 0;
}

/* ---- ID-based hid_device lookup (callback_arg stores an int) ---- */
#define HID_DEV_MAX 4

typedef struct {
	struct hid_device *hdev;
	bool used;
} hid_dev_slot_t;

static hid_dev_slot_t hid_dev_table[HID_DEV_MAX];

static int hid_dev_alloc(void)
{
	for (int i = 0; i < HID_DEV_MAX; i++) {
		if (!hid_dev_table[i].used) {
			hid_dev_table[i].used = true;
			return i;
		}
	}
	ESP_LOGE(TAG, "hid_dev_alloc: table full");
	return -1;
}

static void hid_dev_free(int id)
{
	if ((unsigned)id >= HID_DEV_MAX) return;
	if (!hid_dev_table[id].used) {
		ESP_LOGW(TAG, "hid_dev_free(%d): already freed", id);
		return;
	}
	if (hid_dev_table[id].hdev) {
		hid_destroy_device(hid_dev_table[id].hdev);
	}
	hid_dev_table[id].hdev = NULL;
	hid_dev_table[id].used = false;
}

static bool hid_dev_bind(int id, struct hid_device *hdev)
{
	if ((unsigned)id >= HID_DEV_MAX || !hid_dev_table[id].used) {
		ESP_LOGE(TAG, "hid_dev_bind(%d): slot not allocated", id);
		return false;
	}
	hid_dev_table[id].hdev = hdev;
	return true;
}

static struct hid_device *hid_dev_get(int id)
{
	if ((unsigned)id >= HID_DEV_MAX) {
		ESP_LOGE(TAG, "hid_dev_get: invalid id %d", id);
		return NULL;
	}
	if (!hid_dev_table[id].used) {
		ESP_LOGW(TAG, "hid_dev_get(%d): slot not used", id);
		return NULL;
	}
	return hid_dev_table[id].hdev;
}

/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle, const hid_host_interface_event_t event, void *arg)
{
	uint8_t data[64] = {0};
	size_t data_length = 0;
	hid_host_dev_params_t dev_params;
	esp_err_t err = hid_host_device_get_params(hid_device_handle, &dev_params);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "HID interface get_params failed: %s", esp_err_to_name(err));
		return;
	}

	switch (event) {
	case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
		err = hid_host_device_get_raw_input_report_data(hid_device_handle, data, 64, &data_length);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "HID get input report failed: %s", esp_err_to_name(err));
			break;
		}

		{
			struct hid_device *hdev = hid_dev_get((int)(uintptr_t)arg);
			if (!hdev) break;
			hid_input_report(hdev, HID_REPORT_TYPE_INPUT, data, data_length);
		}

		break;
	case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "HID Device DISCONNECTED: sub_class=0x%02x proto=0x%02x",
		         dev_params.sub_class, dev_params.proto);
		err = hid_host_device_close(hid_device_handle);
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "HID Device close failed: %s", esp_err_to_name(err));
		}
		hid_dev_free((int)(uintptr_t)arg);
		break;
	case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
		ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR", hid_proto_name_str[dev_params.proto]);
		break;
	default:
		ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event", hid_proto_name_str[dev_params.proto]);
		break;
	}
}

/**
 * @brief USB HID Host Device event
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host Device event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_device_event(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg)
{
	hid_host_dev_params_t dev_params;
	esp_err_t err = hid_host_device_get_params(hid_device_handle, &dev_params);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "HID Device get_params failed: %s", esp_err_to_name(err));
		return;
	}

	switch (event) {
	case HID_HOST_DRIVER_EVENT_CONNECTED:
		ESP_LOGI(TAG, "HID Device CONNECTED: sub_class=0x%02x proto=0x%02x (%s)",
		         dev_params.sub_class, dev_params.proto,
		         hid_proto_name_str[dev_params.proto]);

		/* Only handle keyboard and mouse — skip other HID devices to
		 * avoid wasting limited HCD channels. */
		if (dev_params.proto != HID_PROTOCOL_KEYBOARD &&
		    dev_params.proto != HID_PROTOCOL_MOUSE) {
			ESP_LOGI(TAG, "HID Device skipped: proto=%d not keyboard/mouse", dev_params.proto);
			break;
		}

		int id = hid_dev_alloc();
		if (id < 0) {
			ESP_LOGE(TAG, "HID Device alloc failed: table full");
			break;
		}

		const hid_host_device_config_t dev_config = {
		        .callback = hid_host_interface_callback,
		        .callback_arg = (void *)(uintptr_t)id,
		};

		err = hid_host_device_open(hid_device_handle, &dev_config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "HID Device open failed: %s", esp_err_to_name(err));
			hid_dev_free(id);
			break;
		}

		/* Allocate locally; table[id] is sentinel, DISCONNECT will not touch it. */
		struct hid_device *hdev = hid_allocate_device();
		bool parsed = false;
		{
			size_t desc_len = 0;
			uint8_t *desc = hid_host_get_report_descriptor(hid_device_handle, &desc_len);
			if (desc && desc_len) {
				parsed = (hid_parse_report(hdev, desc, desc_len) == 0);
#if 0
				printf("HID Report Descriptor (%u bytes):\n", (unsigned)desc_len);
				for (size_t i = 0; i < desc_len; i++)
					printf("%02x%c", desc[i], ((i & 0x0f) == 0x0f) ? '\n' : ' ');
				if (desc_len & 0x0f) printf("\n");
				hid_dump_device(hdev);
#endif
			}
		}
		if (!parsed) {
			ESP_LOGE(TAG, "HID Report Descriptor parse failed");
			hid_destroy_device(hdev);
			hid_dev_free(id);
			hid_host_device_close(hid_device_handle);
			break;
		}
		if (!hid_dev_bind(id, hdev)) {
			hid_destroy_device(hdev);
			hid_dev_free(id);
			hid_host_device_close(hid_device_handle);
			break;
		}

		if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class &&
		    HID_PROTOCOL_KEYBOARD == dev_params.proto) {
			err = hid_class_request_set_idle(hid_device_handle, 0, 0);
			if (err != ESP_OK) {
				ESP_LOGW(TAG, "HID set idle failed: %s", esp_err_to_name(err));
			}
		}
		err = hid_host_device_start(hid_device_handle);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "HID Device start failed: %s", esp_err_to_name(err));
			hid_host_device_close(hid_device_handle);
			hid_dev_free(id);
			break;
		}
		break;
	default:
		break;
	}
}

/**
 * @brief Start USB Host install and handle common USB host library events while app pin not low
 *
 * @param[in] arg  Not used
 */
/* USB Host and USB-Serial-JTAG share the internal USB FSLS PHY. Host init
 * maps it to the USB Wrap (RTC domain); esp_restart() is a CPU-only reset
 * that keeps the RTC domain, so the mapping would leak into the next boot
 * and leave USJ without a PHY (device invisible). Registered after Host
 * install: give the PHY back to USJ before any restart. */
static void usb_phy_restore_on_shutdown(void);

static void usb_lib_task(void *arg)
{
	const usb_host_config_t host_config = {
	        .skip_phy_setup = false,
	        .intr_flags = 0,
	};

	ESP_ERROR_CHECK(usb_host_install(&host_config));
	/* USB Host and USB-Serial-JTAG share the internal USB FSLS PHY; Host
	 * init maps it to the USB Wrap in the RTC domain. esp_restart() is a
	 * CPU-only reset and does NOT clear the RTC domain, so the mapping
	 * would survive into the next boot (e.g. Flash Mode) and leave USJ
	 * without a PHY. Return the PHY to USJ before any restart — the
	 * owner of the resource cleans up after itself. */
	esp_register_shutdown_handler(usb_phy_restore_on_shutdown);
	xTaskNotifyGive(arg);

	while (true) {
		uint32_t event_flags;
		usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
		// In this example, there is only one client registered
		// So, once we deregister the client, this call must succeed with ESP_OK
		if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
			ESP_ERROR_CHECK(usb_host_device_free_all());
			break;
		}
	}

	ESP_LOGI(TAG, "USB shutdown");
	// Clean up USB Host
	vTaskDelay(10); // Short delay to allow clients clean-up
	ESP_ERROR_CHECK(usb_host_uninstall());
	vTaskDelete(NULL);
}

/**
 * @brief BOOT button pressed callback
 *
 * Signal application to exit the HID Host task
 *
 * @param[in] arg Unused
 */
static void gpio_isr_cb(void *arg)
{
	BaseType_t xTaskWoken = pdFALSE;
	const app_event_queue_t evt_queue = {
	        .event_group = APP_EVENT,
	};

	if (app_event_queue) {
		xQueueSendFromISR(app_event_queue, &evt_queue, &xTaskWoken);
	}

	if (xTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}

/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg)
{
	const app_event_queue_t evt_queue = {.event_group = APP_EVENT_HID_HOST,
	                                     // HID Host Device related info
	                                     .hid_host_device.handle = hid_device_handle,
	                                     .hid_host_device.event = event,
	                                     .hid_host_device.arg = arg};

	if (app_event_queue) {
		xQueueSend(app_event_queue, &evt_queue, 0);
	}
}

/* USB Host and USB-Serial-JTAG share the internal USB FSLS PHY. Host init
 * maps it to the USB Wrap (RTC domain); esp_restart() is a CPU-only reset
 * that keeps the RTC domain, so the mapping would leak into the next boot
 * and leave USJ without a PHY (device invisible). Registered after Host
 * install: give the PHY back to USJ before any restart. */
static void usb_phy_restore_on_shutdown(void)
{
	usb_serial_jtag_ll_phy_enable_external(false); /* internal PHY -> USJ */
	usb_serial_jtag_ll_phy_enable_pad(true);       /* D+/D- pads on */
}

void usb_hid_main(void)
{
	BaseType_t task_created;
	app_event_queue_t evt_queue;
	ESP_LOGI(TAG, "HID Host example");

	/* Diagnostic: memory state before the USB Host library installs.
	 * usb_host_install() needs DMA-capable internal RAM; Wi-Fi (initialized
	 * earlier in app_main) may have consumed it all. */
	{
		int dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
		int dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
		int dma_max = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
		int psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
		ESP_LOGI(TAG, "before usb_lib_task: dram=%d dma=%d/%d ps=%d",
		         dram, dma, dma_max, psram);
	}

	// Init BOOT button: Pressing the button simulates app request to exit
	// It will disconnect the USB device and uninstall the HID driver and USB Host Lib
	const gpio_config_t input_pin = {
	        .pin_bit_mask = BIT64(APP_QUIT_PIN),
	        .mode = GPIO_MODE_INPUT,
	        .pull_up_en = GPIO_PULLUP_ENABLE,
	        .intr_type = GPIO_INTR_NEGEDGE,
	};
	ESP_ERROR_CHECK(gpio_config(&input_pin));
	// GPIO ISR service can only be installed once.
	// Some modules/tests may have installed it already; ignore the "already installed" state.
	esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
		ESP_ERROR_CHECK(err);
	}
	ESP_ERROR_CHECK(gpio_isr_handler_add(APP_QUIT_PIN, gpio_isr_cb, NULL));

	/*
    * Create usb_lib_task to:
    * - initialize USB Host library
    * - Handle USB Host events while APP pin in in HIGH state
    */
	task_created = xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, xTaskGetCurrentTaskHandle(), 2, NULL, 0);
	assert(task_created == pdTRUE);

	// Wait for notification from usb_lib_task to proceed
	ulTaskNotifyTake(false, 1000);

	/*
    * HID host driver configuration
    * - create background task for handling low level event inside the HID driver
    * - provide the device callback to get new HID Device connection event
    */
	const hid_host_driver_config_t hid_host_driver_config = {.create_background_task = true,
	                                                         .task_priority = 7,
	                                                         .stack_size = 4096,
	                                                         .core_id = 0,
	                                                         .callback = hid_host_device_callback,
	                                                         .callback_arg = NULL};

	ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

	// Create queue
	app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

	ESP_LOGI(TAG, "Waiting for HID Device to be connected");

	while (1) {
		// Wait queue
		if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
			if (APP_EVENT == evt_queue.event_group) {
				// User pressed button
				usb_host_lib_info_t lib_info;
				ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
				if (lib_info.num_devices == 0) {
					// End while cycle
					break;
				} else {
					ESP_LOGW(TAG, "To shutdown example, remove all USB devices and press button again.");
					// Keep polling
				}
			}

			if (APP_EVENT_HID_HOST == evt_queue.event_group) {
				hid_host_device_event(evt_queue.hid_host_device.handle, evt_queue.hid_host_device.event, evt_queue.hid_host_device.arg);
			}
		}
	}

	ESP_LOGI(TAG, "HID Driver uninstall");
	ESP_ERROR_CHECK(hid_host_uninstall());
	gpio_isr_handler_remove(APP_QUIT_PIN);
	xQueueReset(app_event_queue);
	vQueueDelete(app_event_queue);
}
