/* ESP32 architecture mutex — FreeRTOS-based */
#ifndef ASM_MUTEX_H
#define ASM_MUTEX_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef SemaphoreHandle_t asm_mutex_t;

#define ASM_MUTEX_INITIALIZER NULL

static inline void asm_mutex_init(asm_mutex_t *m)
{
	*m = xSemaphoreCreateMutex();
}

static inline void asm_mutex_lock(asm_mutex_t *m)
{
	xSemaphoreTake(*m, portMAX_DELAY);
}

static inline void asm_mutex_unlock(asm_mutex_t *m)
{
	xSemaphoreGive(*m);
}

#endif
