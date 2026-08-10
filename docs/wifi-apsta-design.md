# APSTA 配网 + 局域网遥控 — 设计文档

> 状态：设计已定稿。核心行为——**配网纯模式**（长按 F12 → APSTA，STA 不发起连接，手机一律切热点配网）；断线静默无限重连、不自动开热点；配网 5min 超时。已确认裁剪：忘记网络、跳过配网（T8）、手动重配失败回滚（T20）。已实现：成功页 **20s 倒计时 + [复制并关闭配网热点] 按钮**（复制 IP + POST /api/wifi/done）；热点 SSID = **MacNano配网热点**（面板 ASCII 截断显示 MacNano…）；成功页不展示 macnano.local。实现：P0–P4 已完成（见进度表）；剩余 P6 回归。现状基线：`web-control.c` 已从仅 SoftAP 重构为状态机（APSTA/STA-only）。

## 设计目标

1. **配网统一由暂停菜单触发**：短按暂停键看引导 → 长按 1.5s 进入配网。首次上电 / 换网络 / 换路由器**同一入口、同一流程**（用户操作一遍即记住）；首次上电无凭据**不开 WiFi**（等长按；AP_ONLY 状态/代码保留，无入口）
2. 配网成功 → 设备上家庭局域网，手机在自家 WiFi 下直接遥控（不用切热点）
3. **断网静默自愈**：断线后无限后台重连，**不自动开热点**；配网统一由长按触发（D10）；提供固定名字 `http://macnano.local`（ESP-MDNS），IP 不是唯一入口
4. 兼容现有行为：`web_control_enable()/disable()` 接口、3-way 左右键 WiFi 开关（F4=off / F5=on）、LVGL 面板 switch、AP_ONLY 60s 自动关（代码保留）

## 已核实的关键事实（设计依据）

| 事实 | 来源 |
|------|------|
| AP-only 模式**不支持扫描**（`esp_wifi_set_scan_parameters` 返回 `ESP_ERR_NOT_SUPPORTED`）→ 配网出 SSID 列表必须 APSTA | esp_wifi.h v5.5.4 |
| `esp_wifi_connect()` 只尝试一次，官方建议应用层实现重连逻辑 | esp_wifi.h `esp_wifi_connect` attention 4 |
| APSTA 时 **AP 通道自动跟随 STA 通道**（STA 连上后 AP 跳频，手机短暂掉线自动回连） | esp_wifi.h `esp_wifi_set_config` attention 3 |
| 扫描与连接互斥：连接中调用 `esp_wifi_scan_start` 返回 `ESP_ERR_WIFI_STATE` | esp_wifi.h |
| WPA3-SAE 已开、Enterprise 已开（sdkconfig） | sdkconfig `CONFIG_ESP_WIFI_ENABLE_WPA3_SAE` / `CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT` |
| ESP32-S3 无 5GHz → 5G-only 网络扫描不到、连不上 | SOC_WIFI_SUPPORT_5G |
| WiFi buffer 配置：static_rx=4 / dynamic_rx=16 / static_tx=8 / AMPDU 关 | sdkconfig |
| `esp_wifi_set_config` 会把 STA/AP 配置存 WiFi NVS；本项目以自有 NVS（`mini_mac` 命名空间）为凭据唯一事实源，启动时显式 set_config | esp_wifi.h attention 4 / settings_persist.c |

## 核心设计决策

| # | 决策 | 状态 | 理由 |
|---|---|---|---|
| D1 | 配网成功 → **切 STA-only**（AP 关） | ✅ 已确认 | 内部 RAM 已有压力（截屏拷 PSRAM）；热点不常驻（安全、无干扰）；手机无需切网即可遥控 |
| D2 | PROVISIONING / CONNECTING 用 **APSTA** | ✅ | 扫描需要 STA 接口；用户提交凭据后手机还在热点上，能看到结果/错误 |
| D3 | 断线后 **静默无限后台重连**（STA-only，1s→60s 退避封顶），**不自动开热点**；网络恢复自动回 CONNECTED；上电有凭据连接失败也走本逻辑（T19） | ✅ | 简化：无"何时开热点"的自动决策，配网统一由用户长按触发（D10）；设备静默自愈，用户无感 |
| D4 | 凭据**提交时落盘（T5）**；`wifi_prov` 标志作为"未验证凭据"守卫（GOT_IP 才清）→ 重启时凭据未验证必进配网模式，杜绝静默困死 | ✅ | 与"验证成功后才写"安全性等价，且掉电窗口更小（无 GOT_IP→写 NVS 间隙）；T20 回滚已砍（P3 裁剪），"验证后写"失去支撑场景；开放网络空密码可存取（见进度表） |
| D5 | 密码错误判定：连接期收到 `4WAY_HANDSHAKE_TIMEOUT` / `HANDSHAKE_TIMEOUT` / `AUTH_FAIL` → **立即报错停止重试** | ✅ | 不等重试耗尽，秒级反馈 |
| D6 | auto-off 按状态差异化：AP_ONLY 保留 60s；**PROVISIONING 5min 无操作超时**（T15，用户不想配就超时关闭，重新长按）；CONNECTING / CONNECTED / RECONNECTING 不自动关 | ✅ | 已连时关 WiFi 等于杀遥控；配网超时防热点无限挂着，用户重配 = 再长按 |
| D7 | 配网成功有 **20s AP 宽限期**：成功页显示倒计时 + [复制并关闭配网热点] 按钮（POST /api/wifi/done 立即结束） | ✅ | 用户有充足时间在热点上看结果/复制地址；到点或按钮点击后自动关 AP 降 STA-only |
| D8 | 手动重配 = **长按 F12 → APSTA**（AP+STA 双接口，**STA 不发起连接**——配网纯模式）；超时并入统一 **5min**（T15：无操作 → 关热点回原状态，用户再长按重配） | ✅ | 配网纯模式：STA 接口保留（扫描可用）但不连接，手机一律切热点配网，与首启同流程（用户一遍记住）；有凭据仅用于 T15 超时恢复；超时防热点无限挂 |
| D9 | **新增 ESP-MDNS**：`http://macnano.local` 同时服务热点与局域网 | ✅ | IP 会丢、名字不会丢：成功页、断网兜底、换网后都能用同一地址找回设备；`esp_mdns` 为 IDF 现成组件，开销极小（定时组播报文）。局限：个别 Android ROM/浏览器对 `.local` 支持差 → UI 永远同时展示 IP 兜底 |
| D10 | **统一配网入口 = 暂停键长按 1.5s 或面板 [配网]/[重新配网] 按钮**（短按 = 暂停菜单）：任意状态长按/点按钮 → 开热点进配网；首次上电无凭据 → 不开 WiFi，等长按 | ✅ 已确认 | 首启/换网/换路由同一入口同一流程，用户一遍记住；暂停菜单中 Mac 鼠标可点击面板按钮（与长按同路径）；3-way 开关产生不了 UP/DOWN/ENTER/点击——左右键仅作 WiFi 开关（不触发配网）；长按检测在 key-gpio-polled 加 press-duration（原"不做长按"的决策前提已不成立） |

## 状态机

```
                    ┌──────────────────────────────────────────────────────────────┐
                    │                                                              │
                    ▼                                                              │
  boot ──无凭据──► OFF ──长按F12──► PROVISIONING ──提交凭据──► CONNECTING ──GOT_IP──► CONNECTED(宽限20s→STA-only)
    │                                   ▲    │                  │   ▲                    │
    │有凭据                              │    └──提交失败(首次)───┘   │                    │
    ▼                                   │                          │                    │
 CONNECTING ◄────────────────────────────┘                          │                    │
    │                                                               │                    │
    │                                                               │                    │
    │                                                               │                    │
    └──GOT_IP──► CONNECTED ──断线──► RECONNECTING ◄──重连成功────────┘                    │
                     │                    │                                              │
                     │ 用户: 长按F12 配网  │ 手动关                                        │
                     ▼                    ▼                                              │
                PROVISIONING           OFF ──手动开──► 按凭据走 boot 分支                  │

注：AP_ONLY 状态/auto-off 代码保留（无入口，需要时再加），当前无凭据不进 AP_ONLY。

注：长按 F12 或面板配网按钮（任意状态）→ 开热点进配网（T14）；短按 = 暂停菜单（不变）。

注：CONNECTING 的失败分流——
- 首次配网提交失败（NVS 无旧凭据）→ PROVISIONING(err)，表单显示具体原因（T6/T7）；
- 手动重配提交失败（NVS 有旧凭据）→ 报错，仍显示 PROVISIONING(err)（T20 已砍：无回滚，新凭据覆盖旧凭据）；
- boot 有凭据连接失败 → **不走表单**，进 RECONNECTING 静默无限重连，面板显示"重连中…"（T19）。
```

```c
typedef enum {
    WIFI_STATE_OFF = 0,         /* 关闭（现有 web_control_disable） */
    WIFI_STATE_PROVISIONING,    /* 配网中：APSTA，AP 常驻，5min 无操作超时（T15） */
    WIFI_STATE_CONNECTING,      /* 凭据提交/启动后连接中：APSTA */
    WIFI_STATE_CONNECTED,       /* 已连：STA-only（GOT_IP 后 60s 固定宽限关 AP） */
    WIFI_STATE_RECONNECTING,    /* 断线 / boot 连接失败重连：STA-only 无限退避重试，不自动开热点 */
    WIFI_STATE_AP_ONLY,         /* 保留：纯热点遥控 + 60s auto-off（无入口） */
} wifi_state_t;
```

## 状态转换表

| # | 事件 | 前置 | 动作 | 后置 |
|---|---|---|---|---|
| T1 | 上电，无凭据 | OFF | **不开 WiFi**（AP_ONLY 状态/代码保留无入口）；面板显示 Off；配网由长按 F12 或面板按钮（T14） | OFF |
| T2 | 上电/手动开，有凭据 | OFF | 起 APSTA，`esp_wifi_set_config(STA)` + connect | CONNECTING |
| T3 | `IP_EVENT_STA_GOT_IP` | CONNECTING | 凭据落盘(NVS)；页面推成功页（SSID + 局域网 IP 大字可复制 + 20s 倒计时 + [复制并关闭配网热点] 复制 IP）；20s 宽限计时 | CONNECTED(宽限) |
| T4 | 宽限 20s 到 / 用户点 [复制并关闭配网热点]（T21） | CONNECTED(宽限) | `esp_wifi_set_mode(STA)`（AP 关）；停 DNS | CONNECTED |
| T5 | 提交凭据 | PROVISIONING | 暂存 pending 凭据，STA connect；**若 AP 已关（T15 后）先重开 AP（set_mode(APSTA)）** | CONNECTING |
| T6 | 首次配网提交失败：`4WAY/AUTH_FAIL` | CONNECTING | 判"密码错误"，**停止重试**，保留 AP | PROVISIONING(err=AUTH) |
| T7 | 首次配网提交失败：`NO_AP_FOUND`/超时 | CONNECTING | 判"找不到网络/信号弱"，保留 AP | PROVISIONING(err=NOAP) |
| T9 | 用户手动关（3-way 左键 / 面板开关 / API） | 任意 | stop wifi + httpd（现有 disable 全流程） | OFF |
| T10 | `STA_DISCONNECTED`（已连后） | CONNECTED | 进入重连：STA-only 指数退避重连(1s→60s 封顶)，**静默重试，不自动开热点** | RECONNECTING |
| T11 | 重连成功 GOT_IP | RECONNECTING | —（凭据已在 NVS，无需落盘） | CONNECTED |
| T14 | **长按 F12 1.5s**（任意状态） | OFF / CONNECTED / RECONNECTING / AP_ONLY | 开热点（APSTA）+ httpd + DNS；**STA 接口保留但不发起连接**（配网纯模式，扫描可用）；RECONNECTING 时先停重连 | PROVISIONING |
| T15 | PROVISIONING **5min 无操作超时**（计时从进入配网起，仅 POST 状态变更重置；页面倒计时） | PROVISIONING | 关热点 + DNS；**无凭据 → OFF，有凭据 → CONNECTED**（httpd 保留）；用户重新配网 = 再长按 | OFF / CONNECTED |
| T17 | 60s 无活动 auto-off | AP_ONLY | 关整个栈 | OFF |
| T18 | 活动 touch | AP_ONLY | 重置计时 | AP_ONLY |
| T19 | boot 有凭据连接失败（任意 reason） | CONNECTING | 不弹配网表单；进 RECONNECTING **无限退避重连**（面板"重连中…"），不自动开热点 | RECONNECTING |
| T21 | 用户点 [复制并关闭配网热点]（POST /api/wifi/done） | CONNECTED(宽限) | 复制**局域网 IP**；立即 `esp_wifi_set_mode(STA)`（AP 关）；停 DNS | CONNECTED |

## 事件处理细节

```
esp_event 注册一次（全局）：
  WIFI_EVENT_STA_START      → 若 CONNECTING/RECONNECTING → esp_wifi_connect()
  WIFI_EVENT_STA_CONNECTED  → 不切换状态（等 GOT_IP），记 rssi
  IP_EVENT_STA_GOT_IP       → T3/T11 成功路径
  WIFI_EVENT_STA_DISCONNECTED → reason 分流：
      初始连接(pending/启动连接)：4WAY/HANDSHAKE_TIMEOUT/AUTH_FAIL → 密码错误（T6）
                                   NO_AP_FOUND → 找不到网络（T7）
                                   DHCP 无 IP → 超时 → 路由器异常（T7）
                                   —— 按来源分流：首次配网 → PROVISIONING(err)（T6/T7）；
                                      手动重配 → 报错（T20 已砍，无回滚）；
                                      boot 有凭据 → RECONNECTING 重试（T19）
      已连后：→ RECONNECTING（T10，静默无限重连，不自动开热点）
  扫描：WIFI_EVENT_SCAN_DONE → 缓存结果供 /api/wifi/scan
      （配网中 STA 接口存在即可扫描（off-channel）；仅"正在连接中"才返回 ESP_ERR_WIFI_STATE——纯模式下 STA 不连，扫描随时可用）
  探针分流（/generate_204、/hotspot-detect.html）：PROVISIONING / CONNECTING → 302 到 "/"
      （触发 iOS/Android 门户弹窗，把"连上热点"的用户引导到配网页）；
      AP_ONLY / 遥控态 → 维持 204/Success（遥控时不被"无互联网"弹窗骚扰）

长按检测（key-gpio-polled 扩展，F12）：
  debounce 后计时：<500ms 松开 = 暂停菜单（现有）；≥1.5s 按住 = T14（开热点进配网）；
  500ms~1.5s 按住中 = 不触发（面板引导行显示"继续按住进入配网…"）
```

重连时序（D3 简化）：

```
CONNECTED ──断线──► RECONNECTING ──重连成功──► CONNECTED
                       │
                       └─ STA-only 无限退避重连（1s→60s 封顶），静默进行，不自动开热点
                       └─ 用户要干预 → 长按 F12 进配网（T14）
```

已知行为：断网后设备**静默无限重连**，不自动开热点（D3 简化，配网统一由长按触发）；用户要干预配网 → 长按 F12。手机侧无需任何设置（不再依赖"保存过热点自动回连"的发现机制）。

## AP 生命周期实现细节（设备如何开 AP）

### 初始化（一次性，web_control_enable 内部）

| 步骤 | 调用 | 现状 → 改动 |
|---|---|---|
| 1 | `esp_netif_create_default_wifi_ap()` + **`esp_netif_create_default_wifi_sta()`** | 现在只建 AP netif → **补 STA netif**（netif 创建一次，跨 enable/disable 复用） |
| 2 | `esp_wifi_init(WIFI_INIT_CONFIG_DEFAULT())` | 现有 |
| 3 | `esp_wifi_set_mode()` 初始分流 | 无凭据 → **不启动 WiFi**（T1，等长按 F12）；有凭据 → `WIFI_MODE_APSTA`（T2，随后 set_config(STA) + connect） |
| 4 | `esp_wifi_set_config(WIFI_IF_AP, &ap)` | **channel 改 0**（APSTA 下 AP 自动跟随 STA 通道，esp_wifi.h set_config attention 3；channel=0 时由驱动选默认，AP-only 时固定 1）。**SSID = `MacNano配网热点`（UTF-8，19B）**；密码/WPA2_PSK/max_connection=4 不变 |
| 5 | `esp_wifi_set_config(WIFI_IF_STA, &sta)` | 新增；**每次进入 CONNECTING/RECONNECTING 都显式设置**（自有 NVS 是凭据唯一事实源，防 WiFi NVS 残留旧值） |
| 6 | `esp_wifi_start()` | 现有；触发 STA_START / AP_START 事件 → 事件处理决定是否 connect |
| 7 | httpd + DNS 启动 | 现有 |

要点：`esp_wifi_set_config` 要求目标接口**已存在于当前 mode**（esp_wifi.h attention 1）→ 必须先 set_mode 再 set_config(STA)；AP-only 模式下对 STA 接口 set_config 会失败——长按进配网（T14）时先 `set_mode(APSTA)`（STA 接口随模式出现，供扫描用；纯模式不 set_config(STA)）。

### 运行时开/关 AP（状态转移对应的 WiFi 动作）

| 转移 | set_mode | STA 动作 | DNS | auto-off |
|---|---|---|---|---|
| T1 boot 无凭据 | 不启动 | — | — | —（等长按 F12 / 面板按钮） |
| T2 boot 有凭据 | APSTA | set_config(STA) + connect | 开 | 无 |
| T3 GOT_IP（宽限） | APSTA（不变） | 已连 | 开 | 无 |
| T4 宽限 20s 结束 / T21 按钮 | **STA**（AP 关） | 保持 | 停 | 无 |
| T5 提交凭据 | APSTA（不变） | 切 pending 凭据 connect | 开 | 无 |
| T10/T11 断线重连/重连成功 | STA（不变） | 无限退避重连 → 恢复 | 停 | 无 |
| T14 长按 F12 | **APSTA** | STA 接口保留、不 connect | 开 | 无（配网 5min 超时 T15） |
| T9 手动关 | 全停（现有 disable 流程） | 停 | 停 | — |

实现约定：
- 所有模式切换收口到 `ap_set_mode(wifi_mode_t)`：调 `esp_wifi_set_mode()`。**IDF 头文件对运行时 set_mode 是否 stop/start 已启用接口无明示——实现第 2 步时用串口日志验证 AP_START/AP_STOP/STA_STOP 事件序列**；若新接口未自动启动，回退方案：`esp_wifi_stop()` → set_mode → `esp_wifi_start()`（STA_START 事件已覆盖 connect 重发，见事件处理细节）
- DNS 与 AP 同生共死：CONNECTED（STA-only）停用，避免劫持局域网 DNS
- 每次状态切换打印：`heap_caps_get_free_size(MALLOC_CAP_INTERNAL)` + 当前 mode + DNS 状态（同时验证 RAM 预算与模式切换行为）

### 通道行为

- 配网纯模式 STA 不连：AP 始终用自己的通道（channel=0 退化默认 1），不跟随 STA——手机连热点期间连接稳定，无跳频（B6 不再适用）
- 5GHz-only 环境：AP 只能是 2.4GHz（S3 无 5G），列表为空 → 表单空列表提示（A3）

### 错误路径

- 任一 `esp_wifi_*` 返回错误 → 按当前状态回退 + `err` 入 /api/status：APSTA 起不来（C3）→ 降级 AP-only（无列表，仅手动输入）并提示；STA connect 失败 → T6/T7/T19 分流（见状态转换表）
- 手动关（T9）：httpd 停 → DNS 停 → `esp_wifi_stop()` → `esp_wifi_deinit()`，netif 保留；下次 enable 复用

## 交互设计

### 网页端（web/src，新增配网视图）

**视图路由**：单页 preact 应用，按 `GET /api/status` 的 `state` 字段切换视图；WS 0x09 推送状态变更时即时切换（WS 断开退化为 1s 轮询）。刚配网成功用前端标记区分：`wifiProvisioned`（提交时置位）→ 成功页，否则遥控页。

| state | 视图 | 说明 |
|---|---|---|
| PROVISIONING | 配网表单页 | 主流程入口 |
| CONNECTING | 连接中页 | 提交凭据后的中间态 |
| CONNECTED + wifiProvisioned | 成功页 | 刚配网成功（前端提交时置位；宽限 20s 内手机在热点上可见） |
| CONNECTED | 遥控页 | 现有 UI + IP/macnano.local（可复制；**无配网按钮**——主动配网统一由面板长按） |
| AP_ONLY | 遥控页 | 现有 UI（保留状态，当前无入口） |

**配网表单页**（竖屏手机优先，触控目标 ≥44px）：
- 顶部：标题"WiFi 配网" + 访问提示 `http://macnano.local`（灰字小号，可长按复制）
- **手动重配与首启同一流程**（配网纯模式：STA 断开、手机一律切热点；差异仅在有旧凭据——T15 超时恢复 CONNECTED）
- 倒计时小字（仅手动重配）："热点将在 3:00 后关闭"
- SSID 列表（`/api/wifi/scan`，RSSI 降序）：行 = 信号格 + SSID + 徽标（🔒 WPA2/WPA3 / 2.4G；Enterprise 整行禁用标"不支持"）；点行选中填入下方
- 折叠区：[手动输入 SSID]（隐藏 SSID 场景）+ [扫描隐藏网络] 复选框（scan show_hidden=true）
- 密码框（👁 明文切换）+ [连接] 主按钮（无 SSID 时禁用）
- 状态位：扫描中 spinner / 空列表提示条"未发现 2.4GHz 网络：请确认路由器开启了 2.4GHz 频段" + 自动展开手动输入 / 提交中全屏覆盖"连接中…" / 错误红条（密码错误 / 找不到网络 / 路由器异常）+ [重试]
- 提示条：仅支持 2.4GHz 个人网络（WPA/WPA2/WPA3-Personal）

**连接中页**：spinner + 三步进度（已连接热点 → 正在连接 WiFi → 获取 IP）；结果由 WS 推送/轮询带回，无需手动刷新。
- 提交成功 → 设备 GOT_IP 后 AP 保持 20s 宽限（手机仍在热点上，页面直接看到成功页/错误行）；用户切回路由器 WiFi 后 WS 短暂断开，store.js 1s 间隔自动重连恢复——**无需额外掉线提示**（纯模式设备不切网，切换发生在手机侧）

（RECONNECTING 无网页视图：断网时设备 STA 无 IP、热点不自动开，网页不可达；用户干预 = 长按 F12 进配网）

**成功页**（宽限期，简版 + 主动关闭）：
- 已连 SSID 展示
- 局域网 IP（等宽大字，点击复制，✓ 确认反馈）——IP 单播跨频段/跨系统都可用，不依赖 mDNS
- **主按钮 [复制并关闭配网热点]**：复制**局域网 IP**（✓ 反馈）→ POST /api/wifi/done → 立即关热点，按钮变绿色提示"热点已关闭，请连接 <SSID> 后访问设备"；**真实环境（非 localhost dev）额外尝试 `window.close()` 主动关闭网页**（浏览器限制：非脚本打开的窗口可能被拒——尝试无害）
- **20s 倒计时**（与设备宽限同步）："配网热点将在 Xs 后自动关闭"，到点设备自动关 AP（手机断连、WS 断）
- 文案：请连接家庭 WiFi 后，用下面的 IP 地址访问设备（成功页不再展示 macnano.local）
- 复制兼容：http（insecure context）下 `navigator.clipboard` 不可用 → `copyText()` 回退 `execCommand`（textarea）

**遥控页**（CONNECTED / AP_ONLY）：
- 现有 touchpad/keyboard/menu/floppy 全部不动
- 顶部状态条追加：局域网 IP + `macnano.local`（可点复制）；**不提供配网按钮**——主动配网统一由设备面板长按 F12 触发（统一入口原则）

### 设备端 LVGL 面板（wifi_panel.c）—— 配网中枢（5 步交互）

暂停菜单中的 WiFi 面板承担"引导 → 触发 → 配网信息 → 结果"全部角色：配网由**长按 F12 或面板按钮**触发：

| 步骤 | 用户操作 | 面板显示 | 设备行为 |
|---|---|---|---|
| 1 | F12 短按 | 暂停菜单（现有） | `ui_pause_enter` |
| 2 | 看引导 / 点按钮 | 状态行（未配网/已连接/重连中…）+ 引导行"**Hold rear button 1.5s to provision**" + 按钮（未配网=[配网] / 已连=[重新配网]）；**OFF 无信息行** | 无（每秒刷新状态） |
| 3 | F12 长按 1.5s 或点按钮 | switch → **ON**，状态行切"配网中"：**Hotspot: MacNano…（面板 ASCII 截断；手机显示 MacNano配网热点）/ Key: mac-nano / IP: 192.168.4.1** + 引导行"Connect your phone to this hotspot"（连上后自动弹配网页）；按钮隐藏 | 开热点 + httpd + DNS（T14） |
| 4 | 手机完成配网 | 面板保持"配网中"+ 热点信息（用户可能需要再看 Key） | 提交 → 连接 → GOT_IP → 宽限 → 关热点 |
| 5 | 回暂停菜单查看 | 状态行"已连接"：**WiFi: MyHome-2.4G / Key: — / IP: 192.168.1.42 / mDNS: macnano.local** + [重新配网] 按钮 | 面板从 `web_control_state()` 读 STA IP + mDNS 名 |

布局（640×480 暂停菜单，面板约 420×230）。OFF（未配网）只显示标题/状态/引导行 + 按钮，**信息行隐藏**；配网中显示热点信息（无引导行）；已连接显示家网信息 + 引导行：

```
OFF（未配网）：
┌──────────────────────────────────────────────┐
│ Network                              [开关]  │  标题行 + switch（状态指示，OFF）
│ 未配网                                      │  状态行（大字）
│              ┌────────┐                     │
│              │  配网   │                     │  按钮：点击进配网
│              └────────┘                     │
│ Hold rear button 1.5s to provision        │  引导行
└──────────────────────────────────────────────┘

配网中（引导行提示连热点，连上后门户自动弹配网页）：
┌──────────────────────────────────────────────┐
│ Network                              [开关]  │  标题行 + switch（状态指示，ON）
│ Provisioning                                 │  状态行（大字）
│ Hotspot: MacNano…                          │  热点 SSID（ASCII 截断）
│ Key:  mac-nano                              │  热点密码
│ IP:   192.168.4.1                           │  热点 IP
│ Connect your phone to this hotspot          │  引导行
└──────────────────────────────────────────────┘

已连接：
┌──────────────────────────────────────────────┐
│ Network                              [开关]  │  标题行 + switch（状态指示，ON）
│ 已连接                                      │  状态行（大字）
│ WiFi: MyHome-2.4G                          │  家网 SSID
│ Key:  —                                     │  已连="—"
│ IP:   192.168.1.42                          │  局域网 IP
│ mDNS: macnano.local                         │  已连时附 mDNS 名
│              ┌──────────────┐               │
│              │   重新配网    │               │  按钮：配网中隐藏
│              └──────────────┘               │
│ Hold rear button 1.5s to provision        │  引导行
└──────────────────────────────────────────────┘
```

状态显示表（状态行 / 信息行 / 引导行 / switch / 按钮）：

| 状态 | 状态行 | 信息行（WiFi/Key/IP） | 引导行 | switch | 按钮 |
|---|---|---|---|---|---|
| OFF | 未配网 | 隐藏 | Hold rear button 1.5s to provision | OFF | 配网 |
| PROVISIONING | 配网中 | 热点 SSID（面板 Hotspot: 标签，MacNano… 截断）/ mac-nano / 192.168.4.1 | Connect your phone to this hotspot（连上后自动弹配网页） | ON | 隐藏 |
| CONNECTING | 连接中… | 家网 SSID / — | Hold rear button to provision | ON | 重新配网 |
| CONNECTED | 已连接 | 家网 SSID / — / 局域网 IP / macnano.local | Hold rear button 1.5s to provision | ON | 重新配网 |
| RECONNECTING | 重连中… | 家网 SSID / — | Hold rear button to provision | ON | 重新配网 |

（AP_ONLY 无入口，面板不显示；状态/代码保留）

> 面板文案为英文（字体仅 ASCII `--range 32-127`，与暂停菜单一致）：状态行 `Off / Provisioning / Connecting... / Connected / Reconnecting... / Hotspot`；按钮 `Provision`（OFF）/ `Re-provision`（其他）；引导行 `Hold rear button 1.5s to provision` / `Connect your phone to this hotspot`（配网中）/ `Hold rear button to provision`（连接中/重连中）。

交互变更（P4）：
- **3-way 左右键 = WiFi 开关**：F4（←）= off / F5（→）= on（`web_control_enable/disable`，lvgl.c 保留绑定）；仅开关 WiFi，**不影响配网/联网状态**——不触发配网（配网唯一入口 = 长按 F12 / 面板按钮），不改变已配网连接；背光调节在 mac_hid_bridge（MAC 模式）
- **配网入口 = 长按 F12 或面板按钮**（[配网]/[重新配网] 点击 = `web_control_reprovision()`，与长按同路径）；switch 变为**状态指示**（长按/按钮后 ON；配网超时/关闭后 OFF）
- **手动关 WiFi**：3-way 左键（F4）/ 面板开关 → OFF；重新开启按凭据直连（F5）或长按/按钮进配网（无凭据）
- 引导行按状态切换：未配网/已连 → "Hold rear button 1.5s to provision"（长按后部按钮 1.5s 进配网）；**配网中 → "Connect your phone to this hotspot"**（请用手机连接此热点，连上后自动弹配网页，无需手动输地址）；连接中/重连中 → "Hold rear button to provision"
- 长按触发：key-gpio-polled 检测 F12 ≥1.5s → 调 `web_control_reprovision()`（T14）
- refresh()：1s 定时读 `web_control_state()` + `web_control_sta_info()` 刷新状态行/信息行（现为静态常量 WIFI_SSID/WIFI_PASS/WIFI_IP，全部改为实时）

### HTTP API（web-control.c 新增）

| 端点 | 方法 | 语义 |
|---|---|---|
| `/api/status` | GET | 当前实现 `{floppy, state}`；#2 追加 `sta:{ssid,ip}`（成功页显示 STA IP）。设计版 `ap/err/grace/mdns` 字段未实现 |
| `/api/wifi/scan` | GET | `{aps:[{ssid,rssi,channel,auth}]}` 按信号排序（触发非阻塞扫描；连接中返回"稍后重试"） |
| `/api/wifi/config` | POST | `{ssid,pass}` → T5，200 表示已接受（结果走状态轮询） |
| `/api/wifi/done` | POST | 成功页 [复制并关闭配网热点]（复制 IP + 立即结束宽限）→ T21 |
| WS | 0x09 | 设备主动推送状态变更（配网/重连/宽限倒计时刷新；WS 断开时前端退化为 1s 轮询） |

### 主动配网入口（统一）

**唯一主动入口 = 暂停键长按 1.5s**（D10）。用户一遍记住：短按看菜单 → 看引导 → 长按进配网 → 手机连热点完成。

| 入口 | 路径 | 从哪到哪 |
|---|---|---|
| **暂停键长按 1.5s（唯一主动入口）** | 任意状态 → 长按 F12 → 开热点 + httpd + DNS → 面板显示热点名/Key/IP → 手机连热点（门户弹窗）→ 表单 | 任意 → PROVISIONING |
| 断网自愈 | 自动（**静默无限重连，非配网入口**） | CONNECTED → RECONNECTING → CONNECTED |
| 上电连接失败 | 自动 | boot → CONNECTING → RECONNECTING（不弹表单，静默重连） |
| **面板 [配网]/[重新配网] 按钮** | 暂停菜单 → 点击按钮（Mac 鼠标） | OFF/CONNECTED/RECONNECTING → PROVISIONING（同长按路径） |
| 开关 off/on | 物理按键（3-way 左右键，WiFi 开关，非配网入口） | 任意 → OFF / OFF → 按凭据分流（无凭据不开 WiFi） |

决策说明：原"不引入物理开关长按 = 重新配网"改为"**暂停键长按 = 配网**"——理由：3-way 开关产生不了 UP/DOWN/ENTER/点击，面板按钮在无键鼠时不可达；统一为暂停键长按后，首启/换网/换路由同一入口同一流程，且删除按钮顺带解决不可达问题。

## Case 清单

### A. 首次启动 / 配网中

| # | Case | 行为 |
|---|---|---|
| A1 | 全新首启无凭据 | 不开 WiFi，面板显示 Off；长按 F12 → 配网（T14） |
| A2 | 密码错误 | 4WAY 超时 → **立即**报"密码错误"，不循环重试 |
| A3 | 5GHz-only 路由器 | 扫描不到（S3 无 5G），列表为空 → 表单直接显示"未发现 2.4GHz 网络"提示条；手动输入连不上 → 提示"仅支持 2.4GHz" |
| A4 | WPA3-Enterprise | UI 标注不支持（需 EAP client，本期不做） |
| A5 | 隐藏 SSID | 扫描开关 show_hidden 或手动输入 |
| A6 | 路由器关机/信号弱 | NO_AP_FOUND/超时 → "找不到网络"，留配网页 |
| A7 | DHCP 无响应 | GOT_IP 超时 → "路由器异常（DHCP 无响应）" |
| A8 | 手机提交后中途断热点 | 设备继续连接，结果入状态；宽限固定 60s（无挂起），手机重连热点后看到结果 |
| A9 | 配网页放着不操作 | **5min 超时关热点**（T15）：无凭据回 OFF，有凭据回 CONNECTED；重新长按再配 |
| A11 | 成功宽限期内拔电 | 凭据已落盘 → 下次启动直接 CONNECTING（幂等） |
| A12 | 上电时路由器瞬断/重启 | 有凭据但连不上 → 静默无限重连（面板"重连中…"），不自动开热点、不弹表单（T19） |

### B. 已配网运行

| # | Case | 行为 |
|---|---|---|
| B1 | 正常运行 | CONNECTED，局域网 IP 遥控 |
| B2 | 路由器重启/瞬断 | 设备静默重连自动恢复（无热点干扰）；恢复后经 http://macnano.local 重进页面 |
| B3 | 网络长时间不可用 | 静默无限重连，不自动开热点；用户干预 = 长按 F12 进配网（T14） |
| B4 | 换路由器 | 长按 F12 → 配网；断网中也可长按（RECONNECTING 先停重连再开热点，T14） |
| B5 | 换网络（覆盖旧凭据） | 长按 F12 → 配网 → 新凭据覆盖旧凭据（无"忘记网络"功能） |
| B6 | 配网中 AP 通道固定（STA 不连，无跳频） | 手机连热点期间连接稳定 |
| B7 | 已连态手动关 WiFi | OFF；F5 / 开关再开 → 按凭据直连；长按/按钮 → 配网 |
| B8 | 换网络 | 长按 F12 → STA 断开 → 手机切热点 → 配网（与首启同流程）→ 提交 → 成功页 → 切回路由器 WiFi 遥控 |

### C. 并发/资源/边界

| # | Case | 行为 |
|---|---|---|
| C1 | 多台设备连热点 | max_connection=4；httpd 7 sockets + 1s idle 断开（现有策略） |
| C2 | 两个页面同时提交 | 后者覆盖，幂等 |
| C3 | APSTA 内存不足 | `esp_wifi_start` 返回 NO_MEM → 降级 AP-only 配网（无列表仅手动输入）+ 提示 |
| C4 | NVS 损坏/擦除 | 凭据丢失 → 回首启流程（可接受） |
| C5 | 无外网但 WiFi 通 | 成功标准 = GOT_IP，不要求外网（遥控不需要） |
| C6 | STA-only 时 DNS 服务 | 停用（无 AP 无劫持）；httpd 保留监听局域网 |

## 代码改动映射

| 文件 | 改动 |
|---|---|
| `main/arch/esp32/mach-s3/driver/web/web-control.c` | 重构为状态机 + STA 事件处理 + 扫描 + AP 生命周期（set_mode 切换）+ auto-off 按状态 + HTTP API + **mDNS 初始化 + 探针分流 + boot 失败重试 + `web_control_reprovision()`（长按入口，配网纯模式：APSTA、STA 不 connect）** |
| `main/arch/esp32/mach-s3/driver/web/web-control.h` | `wifi_state_t` + `web_control_state()` 访问器 + P4 追加 `web_control_sta_info()`（实际 SSID + 局域网 IP） |
| `main/arch/esp32/mach-s3/driver/web/dns_server.c` | 不动（PROVISIONING/CONNECTING 用；CONNECTED/RECONNECTING 停用） |
| `main/arch/esp32/mach-s3/settings_persist.c/h` | +`wifi_ssid`/`wifi_pass`（复用现有 persist_set/get_str；空串合法——开放网络） |
| `main/arch/esp32/mach-s3/driver/key-gpio-polled.c` | +F12 长按检测（press-duration：<500ms 暂停 / ≥1.5s 配网 / 按住中显示"继续按住"） |
| `main/arch/esp32/mach-s3/ui/lvgl.c` | F4/F5 → WiFi 开关**保留**；仅删除 F12 长按与菜单退出的旧 ESC 路径冲突处理 |
| `main/arch/esp32/mach-s3/ui/wifi_panel.c` | 5 步交互面板：标题 Network + 状态行 + 引导行（**OFF 无信息行**）+ 配网中热点信息（SSID/Key/IP）+ CONNECTED 显示 IP/mDNS（keys 按状态动态行数：热点 3 行 / 已连 4 行 / 连接中·重连中 2 行，IP/mDNS 仅已连显示）+ switch 状态指示 + [配网]/[重新配网] 按钮；refresh() 1s 定时实时读状态 |
| `web/src/app.jsx` | 状态路由（配网表单/成功页/遥控页；无重连页）+ 遥控页顶部 IP/macnano.local |
| `main/arch/esp32/mach-s3/main.c:372` | **不改**：上电无条件 `web_control_enable()`——有凭据 → 直连；无凭据 → enable 内判断不开 WiFi（T1），配网由长按触发 |
| `main/CMakeLists.txt`（组件依赖） | 引入 `esp_mdns` |

## RAM 预算

- APSTA vs STA-only 增量：STA 接口 static RX 缓冲（每接口一份，当前 4×1600B）+ 第二 netif + lwIP 每接口开销 ≈ **10–20KB 内部 RAM**
- 缓解：AP 仅存在于 PROVISIONING / CONNECTING / 20s 宽限；CONNECTED / RECONNECTING 是 STA-only
- mDNS：开销可忽略（定时组播报文 + 小任务栈）
- 兜底：C3 降级路径
- 建议：状态切换时打印 `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)` 日志验证预算

## 实施计划（分阶段，每步可单独验证）

> 代码路径前缀：`main/arch/esp32/mach-s3/`（web-control / dns_server / key-gpio-polled / wifi_panel / lvgl / settings_persist）；`main.c`、`CMakeLists.txt` 在 `main/` 下。

验证约定：**自闭环** = `idf.py build` / `pnpm build`（agent 执行）；**非自闭环** = 烧录后操作 + 现象（用户执行）。每阶段出口条件达成后才进下一阶段。

### P0：基础设施（无行为变化）
- 范围：引入 `esp_mdns`（`macnano.local`）；`settings_persist` 加 `wifi_ssid`/`wifi_pass` 存取
- 改动：`CMakeLists.txt`（组件依赖）、`settings_persist.c/h`、`web-control.c`（mDNS init）
- 验证：自闭环 build；硬件——现状不变（AP-only），手机连热点访问 `http://macnano.local` 能打开遥控页
- 出口：mDNS 与凭据存取就绪

### P1：STA 直连 + 静默重连（核心状态机第一次成型）
- 范围：`wifi_state_t` + `web_control_state()`；STA netif；boot 有凭据 → CONNECTING → CONNECTED(STA-only)；断线 → RECONNECTING 静默无限重连 → 恢复；上电失败 → RECONNECTING（T19）；httpd 局域网监听
- 改动：`driver/web/web-control.c/h`
- 验证：自闭环 build；硬件（凭据先用临时手段注入，正式入口 P2 才有）——①上电自动连家网，局域网 `macnano.local` 打开遥控页；②关路由，设备静默重连（无热点出现），恢复后遥控页自动回来；③内存日志正常
- 出口：直连 + 自愈成立，遥控体验与现状等价

### P2：配网模式开启（长按 → 热点 → 表单）
- 范围：key-gpio F12 长按检测（<500ms 暂停 / ≥1.5s 配网 / 按住中提示）；`web_control_reprovision()`；PROVISIONING（开热点+DNS+httpd）、5min 超时（T15）、探针分流（302）；网页配网表单基础版（SSID 列表 + 密码 + 提交）；boot 无凭据 → 不开 WiFi（T1）
- 改动：`driver/key-gpio-polled.c`、`driver/web/web-control.c`、`web/src`
- 验证：自闭环 build + `pnpm build`；硬件——①无凭据上电不开热点、面板显示 Off；②长按 F12 → 面板显示热点名/Key/IP，手机连热点门户弹窗出表单；③配网成功 → 切 STA-only，局域网可遥控；④配网后不操作 5min → 热点自动关
- 出口：首启配网主流程通

### P3：配网体验完善（实际范围已裁剪，见下方"P3 实际范围"节）
- 范围：成功页（大字 IP/QR/macnano.local/倒计时/[我已记住]）、60s 宽限（含无客户端挂起）、错误分流（密码错/找不到网）、连接中页 + 切换网络掉线处理、手动重配失败回滚（T20）
- 改动：`web/src`、`driver/web/web-control.c`
- 验证：自闭环 build + `pnpm build`；硬件——①密码错误秒级报错并留表单；②成功页倒计时/[我已记住] 生效，宽限到点热点关；③提交后页面失联自动恢复（"设备正在切换网络"提示）
- 出口：配网全程有反馈、无死路

### P4：LVGL 面板 5 步交互（✅ 已完成，见进度表）
- 范围：状态行/引导行按状态实时切换（**OFF 无信息行，仅引导行 + [配网] 按钮**）、配网中显示热点名/Key/IP、CONNECTED 显示局域网 IP + macnano.local、[重新配网] 按钮（PROVISIONING 隐藏）、switch 状态指示（长按/按钮 → ON）、**3-way 左右键保留 WiFi 开关**（F4=off/F5=on，不影响配网/联网）、refresh() 1s 定时实时读状态
- 改动：`main/arch/esp32/mach-s3/ui/wifi_panel.c`、`main/arch/esp32/mach-s3/ui/lvgl.c`（删 F4/F5 绑定）、`main/arch/esp32/mach-s3/driver/web/web-control.h`（+`web_control_sta_info()`）
- 验证：自闭环 build；硬件——面板各状态（未配网/配网中/已连接/重连中）显示正确、按钮按状态显隐、switch 随长按 ON、3-way 开关 WiFi 正常（不影响配网/联网）、与设备实际状态一致
- 出口：设备端信息中枢成立

### P5：换网与边界（范围已收敛——[放弃配网] 已砍：T15 5min 超时已自动关热点回 CONNECTED，F4/F5 可立即退出，无需按钮）
- 无独立开发内容：手动重配（长按 → STA 断开 → 切热点 → 配网）与 5min 超时恢复均为 P2/P3 已实现行为
- 换网/超时/误触验证并入 P6 回归

### P6：回归与边界收尾
- 范围：长按误触时序（短按/长按/按住中边界）、内存日志对照（`heap_caps_get_free_size`）、双页面并发（C2）、多设备连热点（C1）、5min 超时与 20s 宽限组合
- 改动：按回归结果微调
- 验证：自闭环 build；硬件——P1–P4 全流程回归 + 边界项逐条过：长按时序 10 次无误触、内存增量 ≤20KB、两页并发提交幂等、首启长按配网全流程、门户弹窗（iOS + Android 各一台）、密码错误、配网 5min 超时（关热点回原状态）、断网静默重连自动收敛（无热点）、换网重配（手机切热点流程）、上电瞬断静默重连、macnano.local 热点/局域网双接口解析
- 出口：设计文档硬件验证清单全部打勾

---

## 实施进度（实际，截至当前）

| 阶段 | 状态 | 验证 | 说明 |
|---|---|---|---|
| P0 mDNS + 凭据 NVS | ✅ | 用户烧录 ✓ | macnano.local 热点模式可访问 |
| P1 状态机 + STA 直连 + 静默重连 | ✅ | 用户烧录 ✓ | 修复路由器重启不重连（IDF v5.5.4 无驱动自动重连 → 自管理退避 1s→60s） |
| P2a F12 长按（LVGL active-set） | ✅ | 用户烧录 ✓ | 短按暂停 / ≥1.5s 配网 |
| P2b reprovision + 5min 超时 | ✅ | 用户确认 ✓ | T14/T15 |
| P2c 配网 API + 前端表单 | ✅ | 用户烧录 ✓ | scan/config/探针 302 |
| 配网纯模式 | ✅ | 用户确认 ✓ | 长按后 STA 不连（即使有凭据）；重启保持配网状态（NVS wifi_prov 标志） |
| 失败细分 + 密码错误重弹 | ✅ | 用户烧录 ✓ | 0x09 帧第 3 字节 fail_reason（1=密码错 2=找不到网 3=其他） |
| 行内连接状态 + 背景闪动 | ✅ | 用户烧录 ✓ | 不切页面，wifi-cell 内显示 |
| 配网完成提示页 | ✅ | 用户烧录 ✓ | macnano.local 可复制（http:// 前缀）；显示已连 SSID |
| **accept(23) ENFILE 根因修复** | ✅ | 用户烧录 ✓ | **ws_close_cb 漏 close(fd)**（close_fn 语义：设置后 httpd 不自动 close）→ PCB 卡 CLOSE_WAIT → 池耗尽。对照 xiaozhi 真实产品 + httpd_sess_delete 源码定位 |
| 配网页布局（横屏 grid 挤压） | ✅ | playwright 453x312 ✓ | 遥控页包 `.remote-shell`，横屏 grid 移到 wrapper；配网页不受影响 |
| scan 首次不出现修复 | ✅ | build ✓（真机待验） | 前端 catch 把网络错误当空列表并停轮询 → 改为限次重试（5 次/10s） |
| 信号强度图标 | ✅ | build ✓（真机待验） | 经典填充弧（点+3弧），从内到外：1格=点，2格=点+内弧，3格=全亮 |
| **P4 面板 5 步交互** | ✅ | 用户烧录 ✓ | Network 面板：状态行/信息行/引导行/按钮/switch 按状态渲染（OFF 无信息行）；[配网]/[重新配网] 按钮 = `web_control_reprovision()`；1s 定时刷新（timer 单例 + DELETE 置空防重建崩溃）；3-way 保留 WiFi 开关（不影响配网/联网）；无 hold hint（引导行已说明长按）；面板英文（字体 ASCII only）；验证中修复：keys label 未设文本致“Text”乱码、按钮与引导行重叠（实际面板为窄列 ~238px，引导行换行）、引导行文案定稿 `Hold rear button 1.5s to provision` / `Connect your phone to this hotspot` / `Hold rear button to provision`、F4 WiFi off 死代码（else-if 被 ESC 分支吞）修复 |
| boot 无凭据行为 | ✅ | 用户确认 ✓ | **不开 WiFi**：上电无条件 enable，enable 内判断无凭据直接返回（面板 Off）；AP_ONLY 状态/auto-off 代码保留（无入口）；配网统一由长按 F12（T14）。设计描述已同步（T1、main.c 映射、A1、P2 验证） |
| 开放网络直连 + 空密码持久化 | ✅ | build ✓（真机待验） | auth=0 点击直接连接（无密码框）；`wifi_pass` 支持空串存取（NVS 空串合法，get 区分键缺失/空值）；D4 描述已同步（提交时落盘 + wifi_prov 守卫） |
| 成功页 STA IP（#2） | ✅ | build ✓（真机待验） | `/api/status` 加 `sta:{ssid,ip}`（SSID 做 JSON 转义）；SuccessView 局域网 IP 等宽大字 + 点击复制 + ✓ 确认；样式 `.wifi-ip`；固件嵌入新前端已验证 |
| 复制对号修复（insecure context） | ✅ | build ✓（真机待验） | 真机配网页是 http://IP（insecure context），`navigator.clipboard` 为 undefined → 复制无 ✓ 反馈。修复：`copyText()` 回退 `execCommand('copy')`（textarea 方案）；playwright 验证：置 `navigator.clipboard=undefined` 后 IP/macnano.local 两按钮均显示"已复制 ✓" |
| 成功页 20s 倒计时 + [复制并关闭配网热点] + SSID 改名 | ✅ | build + playwright ✓（真机待验） | grace 60s→20s（`end_grace()` 提取复用）；`/api/wifi/done` 端点（T21）；前端倒计时 + 主按钮（**复制 IP** ✓ + 关热点 + 绿色提示"热点已关闭"；成功页不再展示 macnano.local；真实环境尝试 window.close()）；SSID `MacNano配网热点`（UTF-8 19B），面板 `ascii_prefix()` ASCII 截断显示 MacNano…；**面板 CONNECTED 两列布局 keys/vals 行对齐：keys 加 mDNS: 标签行，vals 显示 macnano.local**；playwright 验证：按钮/倒计时/POST done/剪贴板内容 = IP 全通过 |
| review 修复（并发竞态 + 文案） | ✅ | build ✓（真机待验） | `take_ap_dns()`：临界区内原子取走 s_ap_on/s_dns 并置空，锁外执行 set_mode/stop_dns_server——消除 grace timer / done / 断线事件三方并发导致的 DNS double-free/UAF（`stop_dns_server` 非幂等且有 vTaskDelay，不能锁内调用）；`enter_reconnecting` 同模式收口；倒计时到 0 文案改"热点已自动关闭"；`copyText` execCommand 路径补 `ta.focus()`（iOS Safari） |

### P3 实际范围（用户确认裁剪后）
- ✅ 错误分流（密码错/找不到网）、行内连接状态、成功页（macnano.local + 复制）
- ❌ **不做**（后部分恢复）：60s 宽限交互（倒计时/[我已记住]/无客户端挂起）——用户砍掉后改为 **20s 宽限 + 倒计时 + [复制并关闭配网热点]**（见进度表）
- ❌ **不做**：T20 手动重配失败回滚——重配失败保持配对模式，新凭据覆盖旧凭据无所谓
- ✅ 切换网络掉线处理：**不需要**——配网纯模式下设备不切网（STA 不连），GOT_IP 后 AP 保持 20s 宽限结果可见（成功页/错误行），用户切网后 WS 自动重连自愈

### 后续阶段
- P4 LVGL 面板 5 步交互 ✅（已完成，见进度表）；剩余：P6 回归（P5 已并入）

---

## 已知问题（未解决）

### mDNS（macnano.local）在 STA 网络部分 Android 设备解析失败

**现象**：同一网络（XG_5G，双频合一，设备 ESP32-S3 只能连 2.4G，手机连 5G）下，**一台 Android 手机能解析 macnano.local，另一台不能**（DNS_PROBE_FINISHED_NXDOMAIN）；AP 模式（热点直连）两台都能。

**排查结论**（web research + 实测）：
- 网络/路由器/频段均排除（同网络同频段一个能连，证明组播可达、设备响应正常）
- **根因方向：Android 系统 DNS Resolver 模块版本差异**——AOSP 自 2021.11 起支持 `.local` mDNS 解析，但通过 Play Store 推送的 DNS Resolver 模块，**并非所有设备都更新到支持版本**（Samsung Knox 官方文档确认依赖模块版本；Stack Overflow 有 Android 8-11 能、12/12L/13 某些不能的案例）
- 双频路由器 2.4G/5G 跨频段组播不通是叠加因素（设备 2.4G ↔ 手机 5G），但手机 A 能访问证明组播部分可达
- mDNS TTL 120s 缓存：切换网络（AP→STA）后设备 IP 变化，手机缓存旧 IP 最多 2 分钟

**状态**：设备侧无法修复（手机系统问题）。

**待实施方案**（D9 已有设计，未实现）：
1. 配网成功页**显示设备 STA IP**——IP 单播跨频段/跨系统都可用，不依赖 mDNS（已确认实施：#2）
2. 用户侧：更新手机系统 / Google Play 服务（DNS 模块）；或路由器关双频合一使手机与设备同频段

**验证标准**：Android 老系统手机在 STA 网络用 IP 能访问设备，macnano.local 作为便捷名（支持时可用）。

依赖链：P0 → P1 → P2 → P3 → P4（与 P3 并行）→ P6。（P5 已并入 P6：无独立开发内容）
