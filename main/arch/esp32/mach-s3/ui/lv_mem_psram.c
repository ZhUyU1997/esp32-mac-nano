/*
 * LVGL memory backend: allocate all LVGL objects/styles from PSRAM.
 *
 * Internal SRAM is scarce on this board (emulator 128KB block + Wi-Fi DMA +
 * USB HCD all need it), while LVGL menus are low-frequency UI that does not
 * need fast internal RAM. Selected via CONFIG_LV_USE_CUSTOM_MALLOC.
 */
#include "lvgl.h"
#include "esp_heap_caps.h"

void lv_mem_init(void)
{
	/* Nothing to init */
}

void lv_mem_deinit(void)
{
	/* Nothing to deinit */
}

lv_mem_pool_t lv_mem_add_pool(void *mem, size_t bytes)
{
	LV_UNUSED(mem);
	LV_UNUSED(bytes);
	return NULL; /* pools not supported */
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
	LV_UNUSED(pool);
}

void *lv_malloc_core(size_t size)
{
	return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void *lv_realloc_core(void *p, size_t new_size)
{
	return heap_caps_realloc(p, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void lv_free_core(void *p)
{
	heap_caps_free(p);
}

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p)
{
	mon_p->total_size = 0;
	mon_p->free_cnt = 0;
	mon_p->free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
	mon_p->free_biggest_size = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
	mon_p->used_cnt = 0;
	mon_p->max_used = 0;
	mon_p->used_pct = 0;
	mon_p->frag_pct = 0;
}

lv_result_t lv_mem_test_core(void)
{
	return LV_RESULT_OK;
}
