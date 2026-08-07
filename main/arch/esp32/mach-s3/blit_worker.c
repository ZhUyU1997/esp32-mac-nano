#include <stdlib.h>

#include "esp_task_wdt.h"
#include "fast_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "framebuffer.h"

#include "blit_worker.h"

static const UBaseType_t k_display_task_priority = 6;

typedef struct {
	mach_s3_blit_fn_t fn;
	void *user_ctx;
	TaskHandle_t notify_task;
} mach_s3_blit_job_t;

struct mach_s3_blit_worker {
	QueueHandle_t q;
	framebuffer_t *lcd;
};

static void FAST_FUNC_ATTR mach_s3_blit_task(void *arg)
{
	mach_s3_blit_worker_t *w = (mach_s3_blit_worker_t *)arg;
	mach_s3_blit_job_t job;

	assert(w != NULL);
	assert(w->lcd != NULL);
	assert(w->q != NULL);

	esp_task_wdt_add(NULL);

	for (;;) {
		esp_task_wdt_reset();
		if (!framebuffer_wait_vsync(w->lcd, 30)) {
			continue;
		}
		void *fb = framebuffer_get_framebuffer(w->lcd);
		if (fb == NULL) {
			continue;
		}
		if (xQueueReceive(w->q, &job, 0) != pdTRUE) {
			continue;
		}
		if (job.fn != NULL) {
			job.fn(w->lcd, job.user_ctx);
		}
		if (job.notify_task != NULL) {
			xTaskNotifyGive(job.notify_task);
		}
	}
}

mach_s3_blit_worker_t *mach_s3_blit_worker_create(framebuffer_t *lcd)
{
	mach_s3_blit_worker_t *w = (mach_s3_blit_worker_t *)calloc(1, sizeof(*w));
	if (w == NULL) {
		return NULL;
	}
	w->q = xQueueCreate(1, sizeof(mach_s3_blit_job_t));
	if (w->q == NULL) {
		free(w);
		return NULL;
	}
	w->lcd = lcd;
	if (w->lcd == NULL) {
		vQueueDelete(w->q);
		free(w);
		return NULL;
	}
	(void)framebuffer_restart(w->lcd);
	void *fb = framebuffer_get_framebuffer(w->lcd);
	if (fb == NULL) {
		vQueueDelete(w->q);
		free(w);
		return NULL;
	}

	const BaseType_t ok = xTaskCreatePinnedToCore(mach_s3_blit_task, "blit", 4096, w, k_display_task_priority, NULL, 0);
	if (ok != pdPASS) {
		vQueueDelete(w->q);
		free(w);
		return NULL;
	}
	return w;
}

bool mach_s3_blit_worker_submit_and_wait(mach_s3_blit_worker_t *worker, mach_s3_blit_fn_t fn, void *user_ctx)
{
	mach_s3_blit_worker_t *w = worker;
	if (w == NULL || w->q == NULL) {
		return false;
	}
	(void)ulTaskNotifyTake(pdTRUE, 0);
	const mach_s3_blit_job_t job = {
	        .fn = fn,
	        .user_ctx = user_ctx,
	        .notify_task = xTaskGetCurrentTaskHandle(),
	};
	(void)xQueueOverwrite(w->q, &job);
	(void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	return true;
}

bool mach_s3_blit_worker_submit_async(mach_s3_blit_worker_t *worker, mach_s3_blit_fn_t fn, void *user_ctx)
{
	mach_s3_blit_worker_t *w = worker;
	if (w == NULL || w->q == NULL) {
		return false;
	}
	const mach_s3_blit_job_t job = {
	        .fn = fn,
	        .user_ctx = user_ctx,
	        .notify_task = NULL,
	};
	if (xQueueSend(w->q, &job, 0) == pdTRUE) {
		return true;
	}
	mach_s3_blit_job_t cur;
	if (xQueuePeek(w->q, &cur, 0) == pdTRUE && cur.notify_task == NULL) {
		(void)xQueueOverwrite(w->q, &job);
		return true;
	}
	return false;
}
