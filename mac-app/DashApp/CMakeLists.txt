cmake_minimum_required(VERSION 3.9)
project(Dash)

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE must be set to Retro68 toolchain")
endif()

add_application(Dash
    main.c
    Resources.r
    TYPE "APPL"
    CREATOR "DASH"
)
