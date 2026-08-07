# Root Makefile — ESP32 firmware + Retro68 Mac applications
#
# Targets:
#   make mac-all        构建所有 Mac 68k 应用
#   make mac-CounterApp 构建指定 Mac 应用
#   make mac-clean      清理 Mac 应用构建产物
#   make flash          烧录 ESP32 固件 (需配置串口)
#   make upgrade        生成 SD 卡升级文件 (firmware.bin → upgrade.bin)
#   make upgrade-full   生成 SD 卡升级文件 (含 hd.img → upgrade.bin)
#   make sdcard          打包 SD 卡镜像 + 升级文件到 sdcard-$(VERSION).zip
#   make version         显示当前版本号

MAC_APPS_DIR = mac-app
MAC_APPS    = DashApp

# 版本号 — 用于打包产物标识 (sdcard-<VERSION>.zip), 用 make version 查看
VERSION = 1.0.0

.PHONY: mac-all mac-clean $(addprefix mac-,$(notdir $(wildcard $(MAC_APPS_DIR)/*App)))

mac-all:
	$(MAKE) -C $(MAC_APPS_DIR)

mac-clean:
	$(MAKE) -C $(MAC_APPS_DIR) clean

# 每个 mac-<AppName> 目标委托给子 Makefile
define MAC_APP_RULE
mac-$(notdir $(1)):
	$(MAKE) -C $(MAC_APPS_DIR) $$(notdir $(1))
endef

$(foreach app,$(wildcard $(MAC_APPS_DIR)/*App),$(eval $(call MAC_APP_RULE,$(app))))

# ESP32 固件烧录 (委托给 idf.py)
flash:
	idf.py flash

# 生成 SD 卡升级文件 (仅固件)
upgrade:
	python3 tools/make_upgrade.py \
		--firmware build/esp32-mac-nano.bin \
		--output upgrade.bin

# 打包网页烧写固件包 (manifest.json + 各分区 bin → webflash-<VERSION>.zip)
# 烧写器: web/dist-flash/flash.html (JSZip 解包 + SHA256 校验 + WebSerial 烧写)
webflash:
	python3 tools/make_webflash.py \
		--version $(VERSION) \
		--output webflash-$(VERSION).zip
	@echo "Web flash package: webflash-$(VERSION).zip"

# 生成 SD 卡升级文件 (固件 + 硬盘镜像)
upgrade-full:
	python3 tools/make_upgrade.py \
		--firmware build/esp32-mac-nano.bin \
		--hd macintosh/hd.img \
		--output upgrade.bin

# 安装 Mac 应用到硬盘镜像
# hd_v1.img = 预装应用/游戏的镜像, hd_v0.img = 纯净镜像
# hd.img 由 hd_v1.img 复制生成, 不纳入版本管理
install: $(addprefix mac-,$(MAC_APPS))
	cp macintosh/hd_v1.img macintosh/hd.img
	@for app in $(MAC_APPS); do \
		dsk=$$(ls mac-app/$$app/build/*.dsk 2>/dev/null | head -1); \
		[ -n "$$dsk" ] || { echo "  SKIP $$app (no .dsk found)"; continue; }; \
		echo "Installing $$app from $$dsk..."; \
		scripts/hfs-sync.sh "$$dsk" macintosh/hd.img; \
	done
	@echo "Install done."

# 打包 SD 卡镜像 (hd.img, disk/ 下镜像) + 升级文件 (upgrade.bin)
# 产物带版本号: sdcard-$(VERSION).zip, 所有文件放 zip 根目录
# (zip 内 upgrade.bin 保持固定名, 固件依赖)
sdcard: upgrade-full
	rm -f sdcard-$(VERSION).zip
	cd macintosh && zip -j ../sdcard-$(VERSION).zip hd.img disk/hd0.img disk/hd1.img
	zip sdcard-$(VERSION).zip upgrade.bin
	@echo "SD card image: sdcard-$(VERSION).zip"

# 显示当前版本号
version:
	@echo $(VERSION)

.PHONY: flash upgrade upgrade-full install sdcard version
