idf_component_register(SRCS 
    "arch/esp32/mach-s3/main.c"
    "arch/esp32/mach-s3/blit_worker.c"
    "arch/esp32/mach-s3/task_stats.c"
    "arch/esp32/mach-s3/ui/lvgl.c"
    "arch/esp32/mach-s3/settings_persist.c"
    "arch/esp32/mach-s3/flash_mode.c"
    "arch/esp32/mach-s3/upgrade_sdcard.c"
    "arch/esp32/mach-s3/machine_backend.c"
    "arch/esp32/mach-s3/ui/settings_ui.c"
    "arch/esp32/mach-s3/ui/upgrade_ui.c"
    "arch/esp32/mach-s3/ui/flash_mode_ui.c"
    "arch/esp32/mach-s3/ui/lv_mem_psram.c"
    "arch/esp32/mach-s3/ui/wifi_panel.c"
    "arch/esp32/mach-s3/ui/font/mono_opposans_14.c"
    "arch/esp32/mach-s3/ui/font/mono_opposans_18.c"
    "arch/esp32/mach-s3/driver/ble/input-ble-hid.c"
    "arch/esp32/mach-s3/driver/ble/ble_gattc.c"
    "arch/esp32/mach-s3/driver/ble/ble_report.c"
    "tests/i2s-test.c"
    "arch/esp32/mach-s3/sensors.c"
    "arch/esp32/mach-s3/sensor_shm.c"
    "arch/esp32/mach-s3/driver/key-gpio-polled.c"
    "arch/esp32/mach-s3/driver/gpio-s3.c"
    "arch/esp32/mach-s3/driver/input-usb-hid.c"
    "arch/esp32/mach-s3/driver/web/web-control.c"
    "arch/esp32/mach-s3/driver/web/dns_server.c"
    "arch/esp32/mach-s3/driver/usb/hid_report_parser.c"
    "arch/esp32/mach-s3/driver/snd-i2s.c"
    "arch/esp32/mach-s3/driver/fb-st7701.c"
    "arch/esp32/mach-s3/driver/storage-sdspi-fatfs.c"
    "arch/esp32/mach-s3/driver/block-partition.c"
    "driver/block/block_alias.c"
    "driver/block/block-file.c"
    "driver/block/block.c"
    "driver/sound/sound.c"
    "driver/gpio/gpio.c"
    "arch/esp32/mach-s3/driver/i2c.c"
    "arch/esp32/mach-s3/driver/sensor-sht40.c"
    "arch/esp32/mach-s3/driver/rtc-ds3231.c"
    "third_party/jsmn/jsmn_impl.c"
    "core/kernel/driver.c"
    "core/kernel/iface.c"
    "core/kernel/class.c"
    "core/kernel/framebuffer.c"
    "core/kernel/input.c"
    "core/macplus/core/msg.c"
    "core/kernel/device.c"
    "core/dtree/dt.c"
    "core/dtree/dtree_json.c"
    "core/dtree/dtree_probe.c"
    "core/dtree/probe.c"
    "driver/input/input.c"
    "util/fps_counter.c"
    "core/macplus/core/rom.c"
    "core/macplus/core/macplus.c"
    "core/macplus/core/m68k_pc_sample.c"
    "core/macplus/core/mac_trap_log.c"
    "core/macplus/core/mac_traps_table.c"
    "core/macplus/core/interrupts.c"
    "core/macplus/core/clock.c"
    "core/macplus/core/memmap.c"
    "core/macplus/core/mac_sound.c"
    "core/macplus/devices/iwm.c"
    "core/macplus/devices/scsi.c"
    "core/macplus/devices/rtc.c"
    "core/macplus/devices/e8530.c"
    "core/macplus/devices/e6522.c"
    "core/macplus/devices/keyboard.c"
    "core/macplus/devices/mac_hid_bridge.c"
    "core/macplus/devices/sony.c"
    "core/macplus/cpu/musashi/m68kcpu.c"
    "core/macplus/cpu/musashi/m68kdasm.c"
    "core/macplus/cpu/musashi/m68kops_pre.c"
    "core/macplus/cpu/musashi/m68kopnz.c"
    "core/macplus/cpu/musashi/m68kopdm.c"
    "core/macplus/cpu/musashi/m68kopac.c"
    # ---- vterm: vendored libvterm + platform-free renderer ----
    "../libvterm/src/vterm.c"
    "../libvterm/src/state.c"
    "../libvterm/src/parser.c"
    "../libvterm/src/screen.c"
    "../libvterm/src/pen.c"
    "../libvterm/src/encoding.c"
    "../libvterm/src/unicode.c"
    "../libvterm/src/keyboard.c"
    "../libvterm/src/mouse.c"
    "arch/esp32/mach-s3/vterm/term_render.c"
    "arch/esp32/mach-s3/vterm/vterm_esp32.c"
    "arch/esp32/mach-s3/vterm/vterm_hid_map.c"
    "arch/esp32/mach-s3/vterm/vterm_telnet.c"
    # LDFRAGMENTS
    #     "musashi_iram.lf"
    INCLUDE_DIRS 
        "."
        "include"
        "arch/esp32/include"
        "arch/esp32/mach-s3/include"
        "arch/esp32/mach-s3/driver"
        "arch/esp32/mach-s3/driver/usb"
        "arch/esp32/mach-s3/driver/web"
        "third_party/jsmn"
        "driver"
        "core/macplus/include"
        "core/macplus/cpu/musashi"
        "../libvterm/include"
        "../libvterm/src"
        "../tools/vterm"
        "arch/esp32/mach-s3/vterm"
    EMBED_FILES
        "../macintosh/rom.bin"
        "../macintosh/pcex/pcex_mmio.rom"
        "arch/esp32/mach-s3/dtree/mach-s3.json"
        "../web/dist/index.html.gz"
        "../web/dist/provision.html.gz"
    WHOLE_ARCHIVE
        )

target_compile_definitions(${COMPONENT_TARGET} PRIVATE RAM_SIZE=4096)
target_compile_definitions(${COMPONENT_TARGET} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE DISP_WIDTH=640)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE DISP_HEIGHT=480)
# ROM patches (main/core/macplus/core/rom.c); all default off — set 1 to enable. Example fast boot:
#   SKIP_CHECKSUM_LOOP HLE_ERASE_SCRN HLE_BOOT_PART2_RAM SKIP_MBOOT_BEEP =1, SHORTEN_RAM_SELFTEST optional.
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ROM_PATCH_SKIP_CHECKSUM_LOOP=0)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ROM_PATCH_SHORTEN_RAM_SELFTEST=0)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ROM_PATCH_HLE_ERASE_SCRN=0)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ROM_PATCH_HLE_BOOT_PART2_RAM=0)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ROM_PATCH_SKIP_MBOOT_BEEP=0)
# 1 = center DS alert Rect immediates (Plus v3 ROM P19 @0x1358/0x135E, P_mDSHook @0x2AB8/0x2AC0) for DISP_WIDTH/HEIGHT
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ROM_PATCH_CENTER_DS_ALERT_RECT=0)
# 1 = instruction hook + m68k_pc_sample_at_pc (watch[] in m68k_pc_sample.c, e.g. 0x400A30); very slow
target_compile_definitions(${COMPONENT_TARGET} PRIVATE M68K_PC_SAMPLE_ENABLED=0)
# A-line trap names to serial (peek word at PC in m68k_instruction); very slow.
target_compile_definitions(${COMPONENT_TARGET} PRIVATE MAC_TRAP_LOG=0)


# target_compile_definitions(${COMPONENT_TARGET} PRIVATE DEBUG=1)



target_compile_options(${COMPONENT_TARGET} PRIVATE -Wno-error=format -Wno-error=unused-const-variable -Wno-error=format-overflow -Wno-error=array-bounds)

# vendored libvterm: silence warnings (false positives on the CSI state
# machine); the rest of main stays -Wall-clean
set_source_files_properties(
    ../libvterm/src/vterm.c
    ../libvterm/src/state.c
    ../libvterm/src/parser.c
    ../libvterm/src/screen.c
    ../libvterm/src/pen.c
    ../libvterm/src/encoding.c
    ../libvterm/src/unicode.c
    ../libvterm/src/keyboard.c
    ../libvterm/src/mouse.c
    PROPERTIES COMPILE_OPTIONS "-w")
