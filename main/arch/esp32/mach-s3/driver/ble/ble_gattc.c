/*
 * BLE HID — GATTC callback (connect, service discovery, subscription)
 */
#include <string.h>
#include "esp_log.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_gap_ble_api.h"
#include "ble_priv.h"

static const char *TAG = "ble_hid";

/* BLE UUIDs */
static const uint16_t HID_SERVICE_UUID       = 0x1812;
static const uint16_t BOOT_MOUSE_IN_REP_UUID = 0x2A33;
static const uint16_t CCCD_UUID              = 0x2902;
static const uint16_t PROTOCOL_MODE_UUID     = 0x2A4E;
static const uint16_t REPORT_MAP_UUID        = 0x2A4B;
static const uint16_t BOOT_KBD_IN_REP_UUID   = 0x2A22;

/* ------------------------------------------------------------------ */
/* Report registration                                                */
/* ------------------------------------------------------------------ */
ble_report_t *report_register(ble_dev_t *dev, uint16_t handle,
	ble_report_type_t type)
{
	if (dev->nreports >= MAX_BLE_REPORTS) {
		ESP_LOGE(TAG, "too many reports for device");
		return NULL;
	}
	ble_report_t *rpt = &dev->reports[dev->nreports++];
	memset(rpt, 0, sizeof(*rpt));
	rpt->handle = handle;
	rpt->type = type;
	return rpt;
}

/* ------------------------------------------------------------------ */
/* Subscribe to a characteristic: register for notify + write CCCD    */
/* ------------------------------------------------------------------ */
void subscribe_to_report(uint16_t conn_id, uint8_t *bda,
	uint16_t char_handle, uint16_t cccd_handle)
{
	esp_err_t ret;

	ret = esp_ble_gattc_register_for_notify(s_gattc_if, bda, char_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "register_for_notify failed: %d", ret);
		return;
	}

	uint8_t value[2] = {0x01, 0x00};
	ret = esp_ble_gattc_write_char_descr(s_gattc_if, conn_id, cccd_handle,
		sizeof(value), value, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "write CCCD failed: %d", ret);
	} else {
		ESP_LOGI(TAG, "subscribed to report char_handle=0x%02x cccd=0x%02x",
			char_handle, cccd_handle);
	}
}

/* ------------------------------------------------------------------ */
/* Set Protocol Mode to Boot Protocol (0x00)                          */
/* ------------------------------------------------------------------ */
void try_set_boot_protocol(uint16_t conn_id, uint16_t start_handle, uint16_t end_handle)
{
	esp_bt_uuid_t uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = PROTOCOL_MODE_UUID}};
	uint16_t count = 1;
	esp_gattc_char_elem_t result;

	esp_gatt_status_t st = esp_ble_gattc_get_char_by_uuid(s_gattc_if, conn_id,
		start_handle, end_handle, uuid, &result, &count);
	if (st == ESP_GATT_OK && count > 0) {
		uint8_t boot_mode = 0x00;
		esp_ble_gattc_write_char(s_gattc_if, conn_id, result.char_handle,
			sizeof(boot_mode), &boot_mode, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
		ESP_LOGI(TAG, "set Protocol Mode to Boot Protocol, handle=0x%02x", result.char_handle);
	}
}

/* ------------------------------------------------------------------ */
/* Enumerate HID service: subscribe to Boot reports */
/* ------------------------------------------------------------------ */
void enumerate_hid_service(ble_dev_t *dev, uint16_t conn_id,
	uint16_t start_handle, uint16_t end_handle)
{
	/* Read Report Map */
	{
		esp_bt_uuid_t uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = REPORT_MAP_UUID}};
		uint16_t count = 1;
		esp_gattc_char_elem_t result;
		esp_gatt_status_t st = esp_ble_gattc_get_char_by_uuid(s_gattc_if, conn_id,
			start_handle, end_handle, uuid, &result, &count);
		if (st == ESP_GATT_OK && count > 0) {
			ESP_LOGI(TAG, "found Report Map, handle=0x%02x, reading...", result.char_handle);
			esp_ble_gattc_read_char(s_gattc_if, conn_id, result.char_handle, ESP_GATT_AUTH_REQ_NONE);
		}
	}

	try_set_boot_protocol(conn_id, start_handle, end_handle);

	/* Boot Mouse Input Report */
	{
		esp_bt_uuid_t uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = BOOT_MOUSE_IN_REP_UUID}};
		uint16_t count = 1;
		esp_gattc_char_elem_t result;
		esp_gatt_status_t st = esp_ble_gattc_get_char_by_uuid(s_gattc_if, conn_id,
			start_handle, end_handle, uuid, &result, &count);
		if (st == ESP_GATT_OK && count > 0) {
			ESP_LOGI(TAG, "found Boot Mouse Input Report, handle=0x%02x", result.char_handle);
			report_register(dev, result.char_handle, BLE_REPORT_BOOT_MOUSE);
			uint16_t dcount = 1;
			esp_gattc_descr_elem_t descr;
			esp_bt_uuid_t cccd_u = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = CCCD_UUID}};
			st = esp_ble_gattc_get_descr_by_char_handle(s_gattc_if, conn_id,
				result.char_handle, cccd_u, &descr, &dcount);
			if (st == ESP_GATT_OK && dcount > 0) {
				subscribe_to_report(conn_id, dev->bda,
					result.char_handle, descr.handle);
			}
		}
	}

	/* Boot Keyboard Input Report */
	{
		esp_bt_uuid_t uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = BOOT_KBD_IN_REP_UUID}};
		uint16_t count = 1;
		esp_gattc_char_elem_t result;
		esp_gatt_status_t st = esp_ble_gattc_get_char_by_uuid(s_gattc_if, conn_id,
			start_handle, end_handle, uuid, &result, &count);
		if (st == ESP_GATT_OK && count > 0) {
			ESP_LOGI(TAG, "found Boot Keyboard Input Report, handle=0x%02x", result.char_handle);
			report_register(dev, result.char_handle, BLE_REPORT_BOOT_KBD);
			uint16_t dcount = 1;
			esp_gattc_descr_elem_t descr;
			esp_bt_uuid_t cccd_u = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = CCCD_UUID}};
			st = esp_ble_gattc_get_descr_by_char_handle(s_gattc_if, conn_id,
				result.char_handle, cccd_u, &descr, &dcount);
			if (st == ESP_GATT_OK && dcount > 0) {
				subscribe_to_report(conn_id, dev->bda,
					result.char_handle, descr.handle);
			}
		}
	}
}

/* ------------------------------------------------------------------ */
/* GATTC callback                                                     */
/* ------------------------------------------------------------------ */
void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
	esp_ble_gattc_cb_param_t *param)
{
	switch ((int)event) {
	case ESP_GATTC_REG_EVT: {
		ESP_LOGI(TAG, "GATTC register, status %d, app_id %d, gattc_if %d",
			param->reg.status, param->reg.app_id, gattc_if);
		if (param->reg.status == ESP_GATT_OK) {
			s_gattc_if = gattc_if;
			esp_ble_gap_set_scan_params(&s_scan_params);
		}
		break;
	}
	case ESP_GATTC_OPEN_EVT: {
		ESP_LOGI(TAG, "OPEN %s conn_id=%d status=%d",
			addr_str(param->open.remote_bda),
			param->open.conn_id, param->open.status);
		break;
	}
	case ESP_GATTC_CONNECT_EVT: {
		if (param->connect.link_role == 0) {
			ble_dev_t *dev = dev_alloc();
			if (dev == NULL) {
				ESP_LOGE(TAG, "no free device slot, dropping conn!");
				break;
			}
			dev->conn_id = param->connect.conn_id;
			memcpy(dev->bda, param->connect.remote_bda, 6);
			ESP_LOGI(TAG, "CONNECTED %s conn_id=%d slot=%d",
				addr_str(dev->bda), dev->conn_id,
				(int)(dev - s_devs));
			s_gattc_if = gattc_if;
			esp_err_t enc_ret = esp_ble_set_encryption(
				param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
			if (enc_ret != ESP_OK) {
				ESP_LOGE(TAG, "set_encryption failed: %s",
					esp_err_to_name(enc_ret));
			}
		}
		break;
	}
	case ESP_GATTC_DISCONNECT_EVT: {
		ble_dev_t *dev = dev_by_conn(param->disconnect.conn_id);
		ESP_LOGI(TAG, "DISCONNECTED conn_id=%d reason=0x%02x%s",
			param->disconnect.conn_id, param->disconnect.reason,
			dev ? "" : " (unknown dev)");
		if (dev) dev_free(dev);
		s_restart_scan = true;
		esp_ble_gap_stop_scanning();
		break;
	}
	case ESP_GATTC_CLOSE_EVT: {
		ESP_LOGI(TAG, "CLOSE conn_id=%d", param->close.conn_id);
		break;
	}
	case ESP_GATTC_DIS_SRVC_CMPL_EVT: {
		ble_dev_t *dev = dev_by_conn(param->dis_srvc_cmpl.conn_id);
		if (!dev) break;
		ESP_LOGI(TAG, "service discovery complete conn_id=%d",
			param->dis_srvc_cmpl.conn_id);
		esp_bt_uuid_t svc_uuid = {.len = ESP_UUID_LEN_16,
			.uuid = {.uuid16 = HID_SERVICE_UUID}};
		esp_ble_gattc_search_service(gattc_if,
			param->dis_srvc_cmpl.conn_id, &svc_uuid);
		break;
	}
	case ESP_GATTC_SEARCH_RES_EVT: {
		ble_dev_t *dev = dev_by_conn(param->search_res.conn_id);
		if (!dev) break;
		ESP_LOGI(TAG, "found service UUID=0x%04x, start=0x%02x, end=0x%02x conn_id=%d",
			param->search_res.srvc_id.uuid.uuid.uuid16,
			param->search_res.start_handle,
			param->search_res.end_handle,
			param->search_res.conn_id);
		dev->hid_start = param->search_res.start_handle;
		dev->hid_end = param->search_res.end_handle;
		break;
	}
	case ESP_GATTC_SEARCH_CMPL_EVT: {
		ble_dev_t *dev = dev_by_conn(param->search_cmpl.conn_id);
		if (!dev) break;
		ESP_LOGI(TAG, "search complete conn_id=%d status=%d",
			param->search_cmpl.conn_id, param->search_cmpl.status);
		if (param->search_cmpl.status == ESP_GATT_OK && dev->hid_start != 0) {
			enumerate_hid_service(dev, param->search_cmpl.conn_id,
				dev->hid_start, dev->hid_end);
		}
		break;
	}
	case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
		ESP_LOGI(TAG, "register_for_notify complete, status %d, handle=0x%02x",
			param->reg_for_notify.status, param->reg_for_notify.handle);
		break;
	}
	case ESP_GATTC_WRITE_CHAR_EVT: {
		ESP_LOGI(TAG, "WRITE_CHAR complete, status %d, handle=0x%02x",
			param->write.status, param->write.handle);
		break;
	}
	case ESP_GATTC_WRITE_DESCR_EVT: {
		ESP_LOGI(TAG, "WRITE_DESCR complete, status %d, handle=0x%02x",
			param->write.status, param->write.handle);
		break;
	}
	case ESP_GATTC_READ_CHAR_EVT: {
		ESP_LOGI(TAG, "READ_CHAR handle=0x%02x status=%d len=%d",
			param->read.handle, param->read.status, param->read.value_len);
		ESP_LOG_BUFFER_HEX(TAG, param->read.value, param->read.value_len);
		break;
	}
	case ESP_GATTC_NOTIFY_EVT: {
		ble_dev_t *dev = dev_by_conn(param->notify.conn_id);
		if (!dev) break;

		input_ble_handle_notify(dev, param->notify.handle,
			param->notify.value, param->notify.value_len);
		break;
	}
	default:
		break;
	}
}
