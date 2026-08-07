#pragma once

#include <stdint.h>

#include "macplus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Sensor / RTC shared memory protocol at 0xF00000 (sim->shm_region).
 *
 * All multi-byte values are big-endian (68k native).
 * ESP32 (little-endian) converts on write/read.
 */
#define SENSOR_SHM_CMD_OFFSET    0x00
#define SENSOR_SHM_STATUS_OFFSET 0x01
#define SENSOR_SHM_TEMP_OFFSET   0x02   /* int16 BE: temp x 10 */
#define SENSOR_SHM_HUMI_OFFSET   0x04   /* int16 BE: humi x 10 */
#define SENSOR_SHM_GEN_OFFSET    0x06   /* uint32 BE: cache gen counter */
#define SENSOR_SHM_TIME_OFFSET   0x0A   /* 7 bytes BCD: s/m/h/d/M/Y/wday */

/* Commands */
#define SENSOR_CMD_NONE            0
#define SENSOR_CMD_GET_MEASUREMENT 1
#define SENSOR_CMD_GET_TIME        2
#define SENSOR_CMD_SET_TIME        3
#define SENSOR_CMD_GET_GEN         4

/* Status */
#define SENSOR_STATUS_IDLE  0
#define SENSOR_STATUS_BUSY  1
#define SENSOR_STATUS_DONE  2

/* Big-endian store helpers (like Linux kernel's put_unaligned_be*) */
#include <endian.h>
#include <string.h>
#define put_unaligned_be32(val, ptr) do { uint32_t _v = htobe32(val); memcpy((ptr), &_v, 4); } while(0)
#define put_unaligned_be16(val, ptr) do { uint16_t _v = htobe16(val); memcpy((ptr), &_v, 2); } while(0)

/* Hook values (used with mac_register_hook) */
#define SENSOR_HOOK_READ    23
#define SENSOR_HOOK_RTC     24

/**
 * @brief Register sensor hooks on the given macplus instance.
 *
 * Should be called after mac_new() but before macplus_boot().
 * Registers handlers for SENSOR_HOOK_READ (23) and SENSOR_HOOK_RTC (24).
 */
void sensor_shm_init(macplus_t *sim);

#ifdef __cplusplus
}
#endif
