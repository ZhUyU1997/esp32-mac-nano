# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# VERSION must match Makefile VERSION (packaging) — embedded into the
# app image via esp_app_get_description()->version.
project(esp32-mac-nano VERSION 1.0.0)

# esptool_py_flash_to_partition(flash "rom" "${CMAKE_CURRENT_SOURCE_DIR}/macintosh/rom.bin")
# esptool_py_flash_to_partition(flash "hd" "${CMAKE_CURRENT_SOURCE_DIR}/macintosh/hd.img")

