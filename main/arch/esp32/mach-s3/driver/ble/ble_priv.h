/*
 * BLE HID Host — private shared declarations
 */
#ifndef BLE_PRIV_H
#define BLE_PRIV_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Target device names (NULL-terminated array)                        */
/* ------------------------------------------------------------------ */
extern const char *TARGET_DEVICE_NAMES[];

/* ------------------------------------------------------------------ */
/* Per-device state slot                                              */
/* ------------------------------------------------------------------ */
#define MAX_BLE_DEVICES 4

/* ------------------------------------------------------------------ */
/* Report types + per-report state                                    */
/* ------------------------------------------------------------------ */
#define MAX_BLE_REPORTS 8

typedef enum {
	BLE_REPORT_BOOT_MOUSE = 0,
	BLE_REPORT_BOOT_KBD,
} ble_report_type_t;

typedef struct {
	uint16_t handle;
	ble_report_type_t type;
} ble_report_t;

typedef struct {
	bool in_use;
	uint16_t conn_id;
	uint8_t bda[6];
	uint16_t hid_start;
	uint16_t hid_end;
	ble_report_t reports[MAX_BLE_REPORTS];
	int nreports;
} ble_dev_t;

extern ble_dev_t s_devs[MAX_BLE_DEVICES];
extern esp_gatt_if_t s_gattc_if;
extern bool s_restart_scan;
extern esp_ble_scan_params_t s_scan_params;

/* ------------------------------------------------------------------ */
/* BLE address helpers                                                */
/* ------------------------------------------------------------------ */
const char *addr_str(const uint8_t *bda);
bool is_bonded_addr(const uint8_t *bda);

/* ------------------------------------------------------------------ */
/* Slot management                                                    */
/* ------------------------------------------------------------------ */
ble_dev_t *dev_alloc(void);
ble_dev_t *dev_by_conn(uint16_t conn_id);
void dev_free(ble_dev_t *d);

/* ------------------------------------------------------------------ */
/* Connection                                                         */
/* ------------------------------------------------------------------ */
void connect_to(const uint8_t *bda, uint8_t addr_type);

/* ------------------------------------------------------------------ */
/* Service enumeration + subscription (ble_gattc.c)                   */
/* ------------------------------------------------------------------ */
void subscribe_to_report(uint16_t conn_id, uint8_t *bda,
	uint16_t char_handle, uint16_t cccd_handle);
void try_set_boot_protocol(uint16_t conn_id, uint16_t start_handle,
	uint16_t end_handle);
void enumerate_hid_service(ble_dev_t *dev, uint16_t conn_id,
	uint16_t start_handle, uint16_t end_handle);
ble_report_t *report_register(ble_dev_t *dev, uint16_t handle,
	ble_report_type_t type);

/* ------------------------------------------------------------------ */
/* Report parsing (ble_report.c)                                      */
/* ------------------------------------------------------------------ */
void input_ble_handle_notify(ble_dev_t *dev, uint16_t handle,
	const uint8_t *value, uint16_t value_len);

/* ------------------------------------------------------------------ */
/* Callbacks — defined in their respective .c files                   */
/* ------------------------------------------------------------------ */
void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
	esp_ble_gattc_cb_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* BLE_PRIV_H */
