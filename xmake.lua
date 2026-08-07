set_project("esp32-mac-nano")
set_version("0.1.0")

add_rules("mode.debug", "mode.release")

add_requires("pkgconfig::sdl2")

target("mini-mac-sdl")
    set_kind("binary")
    set_languages("gnu11")
    add_packages("pkgconfig::sdl2")

    add_defines("RAM_SIZE=4096", "DISP_WIDTH=640", "DISP_HEIGHT=480")
    add_defines(
        "ROM_PATCH_SKIP_CHECKSUM_LOOP=0",
        "ROM_PATCH_SHORTEN_RAM_SELFTEST=0",
        "ROM_PATCH_HLE_ERASE_SCRN=0",
        "ROM_PATCH_HLE_BOOT_PART2_RAM=0",
        "ROM_PATCH_SKIP_MBOOT_BEEP=0",
        "ROM_PATCH_CENTER_DS_ALERT_RECT=1",
        "M68K_PC_SAMPLE_ENABLED=0",
        "MAC_TRAP_LOG=0"
    )

    add_includedirs(
        "main",
        "main/include",
        "main/arch/x64/include",
        "main/third_party/jsmn",
        "main/driver",
        "main/core/macplus/include",
        "main/core/macplus/cpu/musashi",
        "main/arch/x64/mach-sdl/include"
    )

    add_files(
        "main/third_party/jsmn/jsmn_impl.c",
        "main/core/kernel/*.c",
        "main/core/dtree/*.c",
        "main/driver/block/block.c",
        "main/driver/block/block_alias.c",
        "main/driver/block/block-file.c",
        "main/driver/sound/sound.c",
        "main/driver/gpio/gpio.c",
        "main/driver/input/input.c",
        "main/core/macplus/core/*.c",
        "main/core/macplus/devices/*.c",
        "main/core/macplus/cpu/musashi/m68kcpu.c",
        "main/core/macplus/cpu/musashi/m68kops_pre.c",
        "main/core/macplus/cpu/musashi/m68kopnz.c",
        "main/core/macplus/cpu/musashi/m68kopdm.c",
        "main/core/macplus/cpu/musashi/m68kopac.c",
        "main/arch/x64/mach-sdl/main.c",
        "main/arch/x64/mach-sdl/driver/*.c"
    )
