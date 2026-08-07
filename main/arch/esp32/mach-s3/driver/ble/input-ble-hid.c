/*
 * BLE HID Host (direct GATTC, no ESP-HIDH)
 *
 * Entry point + GAP callback + device slot management.
 * GATTC callback → ble_gattc.c, report parsing → ble_report.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "input.h"
#include "ble_priv.h"

static const char *TAG = "ble_hid";

const char *TARGET_DEVICE_NAMES[] = {"LIFT", "MX MCHNCL M", NULL};

static int target_device_count(void)
{
	int n = 0;
	while (TARGET_DEVICE_NAMES[n]) n++;
	return n;
}

/* ------------------------------------------------------------------ */
/* Format BLE address into static buffer                              */
/* ------------------------------------------------------------------ */
const char *addr_str(const uint8_t *bda)
{
	static char bufs[4][18];
	static int idx = 0;
	int cur = idx;
	idx = (idx + 1) & 3;
	sprintf(bufs[cur], "%02x:%02x:%02x:%02x:%02x:%02x",
		bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
	return bufs[cur];
}

/* ------------------------------------------------------------------ */
/* Global state                                                       */
/* ------------------------------------------------------------------ */
ble_dev_t s_devs[MAX_BLE_DEVICES];
esp_gatt_if_t s_gattc_if = ESP_GATT_IF_NONE;
bool s_restart_scan = false;

/* ------------------------------------------------------------------ */
/* Slot management                                                    */
/* ------------------------------------------------------------------ */
ble_dev_t *dev_alloc(void)
{
	for (int i = 0; i < MAX_BLE_DEVICES; i++) {
		if (!s_devs[i].in_use) {
			memset(&s_devs[i], 0, sizeof(s_devs[i]));
			s_devs[i].in_use = true;
			return &s_devs[i];
		}
	}
	return NULL;
}

ble_dev_t *dev_by_conn(uint16_t conn_id)
{
	for (int i = 0; i < MAX_BLE_DEVICES; i++) {
		if (s_devs[i].in_use && s_devs[i].conn_id == conn_id)
			return &s_devs[i];
	}
	return NULL;
}

void dev_free(ble_dev_t *d) { d->in_use = false; }

/* ------------------------------------------------------------------ */
/* Bond check                                                         */
/* ------------------------------------------------------------------ */
bool is_bonded_addr(const uint8_t *bda)
{
	int bn = esp_ble_get_bond_device_num();
	if (bn <= 0) return false;
	esp_ble_bond_dev_t *bd = malloc(bn * sizeof(esp_ble_bond_dev_t));
	if (!bd) return false;
	esp_ble_get_bond_device_list(&bn, bd);
	bool found = false;
	for (int i = 0; i < bn; i++) {
		if (memcmp(bd[i].bd_addr, bda, 6) == 0) {
			found = true;
			break;
		}
	}
	free(bd);
	return found;
}

/* ------------------------------------------------------------------ */
/* Scanning params                                                    */
/* ------------------------------------------------------------------ */
esp_ble_scan_params_t s_scan_params = {
	.own_addr_type = BLE_ADDR_TYPE_PUBLIC,
	.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
	.scan_type = BLE_SCAN_TYPE_ACTIVE,
	.scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE,
	.scan_interval = 800,
	.scan_window = 320,
};

static const esp_ble_conn_params_t s_conn_params = {
	.scan_interval = 480,
	.scan_window = 160,
	.interval_min = 12,
	.interval_max = 12,
	.latency = 0,
	.supervision_timeout = ESP_BLE_GAP_SUPERVISION_TIMEOUT_MS(6000),
	.min_ce_len = 0,
	.max_ce_len = 0,
};

/* ------------------------------------------------------------------ */
/* Initiate connection                                                */
/* ------------------------------------------------------------------ */
void connect_to(const uint8_t *bda, uint8_t addr_type)
{
	esp_ble_gatt_creat_conn_params_t cp = {0};
	memcpy(cp.remote_bda, bda, 6);
	cp.remote_addr_type = addr_type;
	cp.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	cp.is_direct = true;
	cp.is_aux = false;
	cp.phy_mask = ESP_BLE_PHY_1M_PREF_MASK;
	cp.phy_1m_conn_params = &s_conn_params;
	cp.phy_2m_conn_params = NULL;
	cp.phy_coded_conn_params = NULL;
	esp_ble_gattc_enh_open(s_gattc_if, &cp);
}

/* ------------------------------------------------------------------ */
/* GAP callback — scan + security                                     */
/* ------------------------------------------------------------------ */
void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	switch ((int)event) {
	case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
		ESP_LOGI(TAG, "scan params set, status %d", param->scan_param_cmpl.status);
		esp_ble_gap_start_scanning(0);
		break;
	}
	case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
		ESP_LOGI(TAG, "scan started, status %d", param->scan_start_cmpl.status);
		break;
	}
	case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
		ESP_LOGI(TAG, "scan stopped");
		if (s_restart_scan) {
			s_restart_scan = false;
			esp_ble_gap_start_scanning(0);
		}
		break;
	}
	case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		esp_ble_gap_cb_param_t *p = param;
		if (p->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT)
			break;
		uint8_t adv_name_len = 0;
		uint8_t *adv_name = esp_ble_resolve_adv_data_by_type(
			p->scan_rst.ble_adv,
			p->scan_rst.adv_data_len + p->scan_rst.scan_rsp_len,
			ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

		bool matched = false;

		if (adv_name && adv_name_len > 0) {
			char name[64];
			size_t copy_len = adv_name_len < (int)sizeof(name) - 1
				? adv_name_len : sizeof(name) - 1;
			memcpy(name, adv_name, copy_len);
			name[copy_len] = '\0';
			ESP_LOGI(TAG, "SCAN: %s type=%d NAME: %s",
				addr_str(p->scan_rst.bda), p->scan_rst.ble_addr_type, name);
			for (int i = 0; TARGET_DEVICE_NAMES[i] != NULL; i++) {
				if (strcmp(name, TARGET_DEVICE_NAMES[i]) == 0) {
					matched = true;
					break;
				}
			}
			if (!matched) matched = is_bonded_addr(p->scan_rst.bda);
		} else {
			ESP_LOGI(TAG, "SCAN: %s type=%d (no name)",
				addr_str(p->scan_rst.bda), p->scan_rst.ble_addr_type);
			if (is_bonded_addr(p->scan_rst.bda))
				matched = true;
		}
		if (matched) {
			bool already = false;
			for (int i = 0; i < MAX_BLE_DEVICES; i++) {
				if (s_devs[i].in_use && memcmp(s_devs[i].bda, p->scan_rst.bda, 6) == 0) {
					already = true;
					break;
				}
			}
			if (!already) {
				connect_to(p->scan_rst.bda, p->scan_rst.ble_addr_type);
				/* Stop when enough devices connected */
				int need = target_device_count(); if (need == 0) need = MAX_BLE_DEVICES; int used = 0;
				for (int i = 0; i < MAX_BLE_DEVICES; i++)
					if (s_devs[i].in_use) used++;
				if (used >= need)
					esp_ble_gap_stop_scanning();
			}
		}
		break;
	}
	case ESP_GAP_BLE_SEC_REQ_EVT: {
		ESP_LOGI(TAG, "SEC_REQ from %s — accepting",
			addr_str(param->ble_security.ble_req.bd_addr));
		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;
	}
	case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: {
		ESP_LOGI(TAG, "PASSKEY_NOTIF: %06"PRIu32,
			param->ble_security.key_notif.passkey);
		break;
	}
	case ESP_GAP_BLE_NC_REQ_EVT: {
		ESP_LOGI(TAG, "NC_REQ %s passkey: %06"PRIu32" — auto-confirm",
			addr_str(param->ble_security.key_notif.bd_addr),
			param->ble_security.key_notif.passkey);
		esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
		break;
	}
	case ESP_GAP_BLE_AUTH_CMPL_EVT: {
		ESP_LOGI(TAG, "AUTH %s %s",
			addr_str(param->ble_security.auth_cmpl.bd_addr),
			param->ble_security.auth_cmpl.success ? "SUCCESS" : "FAILED");
		if (!param->ble_security.auth_cmpl.success) {
			ESP_LOGE(TAG, "AUTH FAILED reason 0x%x",
				param->ble_security.auth_cmpl.fail_reason);
		}
		break;
	}
	default:
		break;
	}
}

/* ------------------------------------------------------------------ */
/* Public entry point                                                 */
/* ------------------------------------------------------------------ */
void ble_hid_host_init(void)
{
	esp_err_t ret;

	ESP_LOGI(TAG, "BLE HID Host init");

	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
	if (ret != ESP_OK)
		ESP_LOGW(TAG, "mem_release classic bt: %s", esp_err_to_name(ret));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller init failed: %s", esp_err_to_name(ret));
		return;
	}
	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller enable failed: %s", esp_err_to_name(ret));
		return;
	}

	ret = esp_bluedroid_init();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "bluedroid init failed: %s", esp_err_to_name(ret));
		return;
	}
	ret = esp_bluedroid_enable();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "bluedroid enable failed: %s", esp_err_to_name(ret));
		return;
	}

	esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
	esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
	uint8_t key_size = 16;
	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

	ret = esp_ble_gap_register_callback(esp_gap_cb);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "gap register failed: %s", esp_err_to_name(ret));
		return;
	}
	ret = esp_ble_gattc_register_callback(esp_gattc_cb);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "gattc register failed: %s", esp_err_to_name(ret));
		return;
	}
	ret = esp_ble_gattc_app_register(0);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "gattc app register failed: %s", esp_err_to_name(ret));
		return;
	}
	esp_ble_gatt_set_local_mtu(200);

	ESP_LOGI(TAG, "BLE init done, scanning…");
}
