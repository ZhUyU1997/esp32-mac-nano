# BLE HID 鼠标支持

ESP32-S3 蓝牙 HID 鼠标支持，使用 **Bluedroid + 直接 GATTC**（不依赖 ESP-HIDH）。

## 设计概要

| 项目 | 内容 |
|------|------|
| 栈 | Bluedroid（GATT Client） |
| 扫描 | BLE 4.2 legacy scan，主动扫描 |
| 匹配策略 | 扫描→名字匹配→连接；无名字设备按 bond list / 最后连接地址匹配 |
| 鼠标协议 | Boot Mouse Input Report（USB HID Boot Protocol） |
| 配对 | SC + MITM + Bond（密钥存 NVS） |
| 断连恢复 | 停扫 + 重开扫描（清空控制器重复过滤），bond key 自动复用 |

## 为什么不直接用 ESP-HIDH

ESP-HIDH 是 Espressif 提供的 HID Host 封装层，但在当前项目中不使用：

- **Notification 限制**：ESP-HIDH 内部固定最多订阅 8 个 notification，多设备/多 report 时会失败
- **连接管理复杂**：多设备支持时需要侵入修改 `esp_hid` 组件代码
- **调试困难**：事件回调经过多层转发，问题定位变慢
- **二进制兼容**：`esp_hid` 组件随 IDF 版本变动，锁定版本后项目更可控

直接 GATTC 的路径更短：`GAP/GATTC 回调 → input_post_mouse_*()`，完全可控。

## 连接流程

```
ble_hid_host_init()
  ├─ nvs_flash_init()
  ├─ esp_bt_controller_mem_release(CLASSIC_BT)   # S3 不支持 Classic
  ├─ esp_bt_controller_init()                     # BLE-only controller
  ├─ esp_bluedroid_init() / enable()
  ├─ esp_ble_gap_set_security_param()            # SC + MITM + BOND
  ├─ esp_ble_gap_register_callback(gap_cb)
  ├─ esp_ble_gattc_register_callback(gattc_cb)
  └─ esp_ble_gattc_app_register(0)

ESP_GATTC_REG_EVT (app register complete)
  └─ esp_ble_gap_set_scan_params()

ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT
  └─ esp_ble_gap_start_scanning()

ESP_GAP_BLE_SCAN_RESULT_EVT
  ├─ 有名字 → 对比 TARGET_DEVICE_NAME ("LIFT")
  │     └─ 匹配 → connect_to()
  └─ 无名字 → 对比 bond list / s_peer_bda
        └─ 匹配 → connect_to()

connect_to()
  └─ esp_ble_gattc_enh_open()   # 发起连接

ESP_GATTC_CONNECT_EVT
  ├─ 记录 s_conn_id, s_connected, s_peer_bda
  └─ esp_ble_set_encryption()   # 触发认证/配对

配对流程（自动）:
  ├─ PASSKEY_NOTIF / NC_REQ → 自动确认
  └─ AUTH_CMPL → 成功则开始服务发现

ESP_GATTC_DIS_SRVC_CMPL_EVT (服务发现完成)
  └─ esp_ble_gattc_search_service(HID_SERVICE_UUID = 0x1812)

ESP_GATTC_SEARCH_RES_EVT (找到 HID Service)
  └─ 记录 start_handle / end_handle

ESP_GATTC_SEARCH_CMPL_EVT (搜索完成)
  └─ enumerate_hid_service()
       ├─ 读取 Report Map（log only）
       ├─ try_set_boot_protocol() → 写入 Boot Protocol Mode
       ├─ 查找 Boot Mouse Input Report → 订阅 notification
       └─ 查找 Report characteristic (0x2A4D) → 逐个订阅

ESP_GATTC_NOTIFY_EVT (鼠标数据到达)
  └─ 解析 USB HID Boot Protocol: [btn][dx][dy][wheel]
       ├─ btn & 0x01 → input_post_mouse_down/up(LEFT)
       ├─ btn & 0x02 → input_post_mouse_down/up(RIGHT)
       └─ dx/dy → input_post_mouse_move_rel()
```

## 断连恢复

鼠标关机/超出范围重新连回的流程：

```
ESP_GATTC_DISCONNECT_EVT
  ├─ s_connected = false  # 清状态
  └─ esp_ble_gap_stop_scanning()  # 停扫触发重启
       │
ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT
  └─ esp_ble_gap_start_scanning()  # 重开扫描，控制器重复过滤已清空
       │
ESP_GAP_BLE_SCAN_RESULT_EVT  # 扫到鼠标
  └─ 名字/地址匹配 → connect_to()
       │
ESP_GATTC_CONNECT_EVT → esp_ble_set_encryption()
  └─ 自动复用 NVS 中存储的 LTK → 直接连上
```

关键机制：
- **Bond key** 存在 NVS，ESP32 重启后仍有效
- **扫描重复过滤 (`BLE_SCAN_DUPLICATE_ENABLE`)**：断连时停扫重开，清空控制器内部的地址过滤表
- **名字 + 地址双层匹配**：有名字时按名字连，无名字时按 bond list 连

## 配置文件

`sdkconfig.defaults` 中相关选项：

| 配置 | 值 | 说明 |
|------|----|------|
| `CONFIG_BT_ENABLED` | y | 启用蓝牙 |
| `CONFIG_BT_BLUEDROID_ENABLED` | y | 使用 Bluedroid 栈 |
| `CONFIG_BT_BLE_42_FEATURES_SUPPORTED` | y | BLE 4.2（HID 不需要 5.0） |
| `CONFIG_BT_BLE_50_FEATURES_SUPPORTED` | n | 节省内存 |
| `CONFIG_BT_GATTC_NOTIF_REG_MAX` | 24 | 最多 24 个 notification 注册 |
| `CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST` | y | BT 分配优先用 PSRAM |
| `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0` | n | 关闭 IDLE0 监视 |
| `CONFIG_PM_SLEEP_FUNC_IN_IRAM` | y | 睡眠功能在 IRAM |

## 已知限制

- **单设备匹配**：`TARGET_DEVICE_NAME` 硬编码为 "LIFT"，多设备需要扩展
- **仅鼠标**：未处理键盘 report，键盘通过 USB HID 实现
- **Boot Protocol only**：不支持 Report Protocol 的复合设备
- **ESP32-S3 无 Classic BT**：不支持传统蓝牙键鼠
