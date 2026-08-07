/*
 * BLE HID — report parsing (mouse + keyboard)
 */
#include "esp_log.h"
#include "input.h"
#include "ble_priv.h"

static const char *TAG = "ble_hid";

/* ------------------------------------------------------------------ */
/* Report lookup — linear scan by GATT handle                         */
/* ------------------------------------------------------------------ */
static ble_report_t *report_lookup(ble_dev_t *dev, uint16_t handle)
{
	for (int i = 0; i < dev->nreports; i++) {
		if (dev->reports[i].handle == handle)
			return &dev->reports[i];
	}
	return NULL;
}

/* ------------------------------------------------------------------ */
/* Boot Mouse handler (4-byte fixed format)                            */
/* ------------------------------------------------------------------ */
static void handle_boot_mouse(
	const uint8_t *value, uint16_t value_len)
{
	uint8_t btn = value[0];
	int8_t dx = (int8_t)value[1];
	int8_t dy = (int8_t)value[2];

	input_report_mouse_button(INPUT_MOUSE_BTN_LEFT,  btn & 0x01);
	input_report_mouse_button(INPUT_MOUSE_BTN_RIGHT, btn & 0x02);
	input_report_mouse_button(INPUT_MOUSE_BTN_MIDDLE,btn & 0x04);
	if (dx != 0 || dy != 0)
		input_post_mouse_move_rel(dx, dy);
}

/* ------------------------------------------------------------------ */
/* Boot Keyboard handler (8-byte fixed format)                         */
/* ------------------------------------------------------------------ */
static void handle_boot_kbd(
	const uint8_t *value, uint16_t value_len)
{
	if (value_len < 8) return;
	input_report_keyboard(value[0], &value[2]);
}

/* ------------------------------------------------------------------ */
/* NOTIFY dispatcher — called from GATTC callback                     */
/* ------------------------------------------------------------------ */
void input_ble_handle_notify(ble_dev_t *dev, uint16_t handle,
	const uint8_t *value, uint16_t value_len)
{
	ble_report_t *rpt = report_lookup(dev, handle);
	if (!rpt) {
		ESP_LOGW(TAG, "unhandled notify handle=0x%04x", (int)handle);
		return;
	}

	switch (rpt->type) {
	case BLE_REPORT_BOOT_MOUSE:
		handle_boot_mouse(value, value_len);
		break;
	case BLE_REPORT_BOOT_KBD:
		handle_boot_kbd(value, value_len);
		break;
	}
}
