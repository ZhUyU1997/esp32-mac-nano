# Web 烧写方案 — 设计决策与软件待办清单

> 状态：核心链路已实现并真机验证；阶段 4（防砖）部分落地（rollback 使能 + 物理键强制 Flash Mode 已实现，ota_0 目标切换待做）。本文沉淀设计决策 + 软件域待办。

## 方案概述

ESP32-S3 无 BOOT/RESET 键，运行期 GPIO19/20 被 USB Host（键盘）占用。通过 **Flash Mode**（NVS 一次性标志 + 重启 + 跳过 USB Host）让 USB-Serial-JTAG 暴露，浏览器 esptool-js（WebSerial）免按键烧写：

```
pause menu "Flash Mode" → 写 NVS → esp_restart
  → Flash Mode 启动（跳过 USB Host，USJ 暴露，全屏提示页）
  → 电脑识别 ttyACM → 网页 flash.html 连接（USJ DTR/RTS 握手自动复位）
  → 固件包 webflash-<ver>.zip（manifest + SHA256 校验）→ 烧 4 分区 → hard reset
  → 自动重启回正常模式
```

限制：WebSerial 仅桌面 Chrome/Edge（Android 无原生支持、iOS 拒绝）。

## 已完成（真机验证）

- [x] Flash Mode 固件机制：NVS 一次性标志（进入即清）+ 跳过 `usb_hid_main` + 全屏提示页 + pause menu 按钮
- [x] PHY 归还：`input-usb-hid.c` 注册 `esp_register_shutdown_handler`，重启前把共享 internal PHY 还给 USJ（Flash Mode 零 USB 代码）
- [x] 网页烧写器 `web/flash.html`：连接 / 固件包 zip（JSZip + SHA256）/ 进度 / 日志（手动分区 DIY 已移除，可能回归）
- [x] `make webflash`：manifest.json + zip 固件包构建（`tools/make_webflash.py`）
- [x] 启动早期物理键强制 Flash Mode（`flash_mode.c`：app_main 第一行 GPIO 直读，GPIO15 三连采样去抖，零依赖）
- [x] `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`（`sdkconfig.defaults`）+ 启动即 `esp_ota_mark_app_valid_cancel_rollback()`（`main.c` → `upgrade_sdcard.c`）
- [x] P0：git commit 当前成果

## 关键技术决策（勿随意改动）

1. **Flash Mode = 一次性授权**：NVS 标志在进入的那次启动读取后立即清除 → 任何重启（烧完 hard_reset / 断电）都回正常模式，无残留。
2. **PHY 归属**：USJ 与 USB Host 共享 internal PHY（`RTCCNTL.usb_conf.sw_usb_phy_sel`，RTC 域）；`esp_restart`（RTC WDT 系统复位）不清 RTC 域 → Host 的 PHY 占用会残留。**修复在占用者**（input-usb-hid.c shutdown handler），不在 Flash Mode。
3. **烧写发生在 ROM 阶段**：esptool-js 握手（DTR/RTS）把芯片复位进 ROM 下载模式，之后与 ROM 固件通信——App 驱动无关，固件坏到校验失败也有 ROM 兜底。
4. **esptool-js 用法**：① 不手动 `port.open()`（`Transport.connect()` 自己 open）；② hard reset 官方写法 `transport.setRTS(true)` → `loader.after('hard_reset')`（esp-web-tools src/flash.ts 同款）。
5. **固件包 vs merged.bin**：merged.bin 按地址跨度填充 0xFF 膨胀；manifest+zip 存原始字节 + 地址 + SHA256，无膨胀。
6. **绝不启用** `CONFIG_SECURE_BOOT` / `CONFIG_SECURE_FLASH`（flash 加密会熔断 ROM 下载模式，无按键设备永久砖）；绝不烧 eFuse 的 DIS_USB_JTAG / DIS_DOWNLOAD_MODE；固件绝不配置 GPIO19/20 为非 USB 功能。

## 软件域待办清单

### P0 收尾
- [x] git commit 当前成果（工作区已干净，全部提交）

### P1 防砖定稿（阶段 4）
- [x] 开启 `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`（`sdkconfig.defaults`，已生效）
- [ ] 网页烧写目标切 ota_0（0x20000）+ 生成"指向 ota_0"的 otadata bin（factory 留 known-good；现 manifest 仍是 factory 0x10000 + 空白 otadata 0xFF）
- [ ] **ota_1 去留决策**：留 = SD 升级 A/B 轮换、两级回滚；去 = hd 扩 10M（需重建分区表 + hd 镜像）。现状：SD 升级已走 `esp_ota_get_next_update_partition`（A/B 轮换）+ 启动 `esp_ota_mark_app_valid_cancel_rollback` 确认

### P2 鲁棒性增强
- [ ] 烧写顺序：app 先、**otadata 最后**（现 `make_webflash.py` 按地址排序，otadata 0xd000 在 app 之前；切 ota_0 后必须先做）
- [ ] WDT 覆盖评估（现状 `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0/1=n`、`CONFIG_ESP_INT_WDT_CHECK_CPU1=n`、`CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y`）
- [ ] panic 自动进 Flash Mode（`esp_reset_reason()==ESP_RST_PANIC` + panic REBOOT）— 可选
- [x] 启动早期物理键强制 Flash Mode（GPIO 直读，app_main 第一行）— 已实现（GPIO15）
- [ ] 故障注入测试框架（Kconfig 隔离）— 可选

### P3 发布（阶段 5）
- [ ] GitHub Actions：tag → build → `make webflash` → Release + gh-pages
- [ ] flash.html 版本列表（Releases API 自动拉取）

### P4 可选扩展
- [ ] UF2 模式（TinyUSB MSC，手机拖文件烧写；能力弱于 WebSerial）
- [ ] 本文档配套：用户操作手册

## 建议顺序

```
P0 ✓ → P1 防砖定稿（剩 ota_0 目标 + 指向性 otadata + ota_1 决策）→ P2 顺序加固（otadata 后置）
→ WDT 评估 → P3 发布 → 文档 → 可选扩展
```

## 恢复层次（软件域内无砖）

```
日常更新        → 菜单 Flash Mode ✓
烧一半断电      → 半写分区损坏但 USB 可见 → 重烧恢复 ✓
固件完全坏      → ROM 下载模式兜底（三级校验链失败落点 USB 均可见）✓
无声卡死        → WDT 转 panic / 断电重启（偶发）/ GPIO0 pad（物理兜底）
真砖（软件外）  → 硬件损坏 / eFuse 禁用下载模式（绝不启用即无）
```
