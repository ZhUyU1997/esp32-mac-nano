#ifndef MACH_S3_BLIT_WORKER_H
#define MACH_S3_BLIT_WORKER_H

#include <stdbool.h>
#include "framebuffer.h"

typedef void (*mach_s3_blit_fn_t)(framebuffer_t *lcd, void *user_ctx);

typedef struct mach_s3_blit_worker mach_s3_blit_worker_t;

mach_s3_blit_worker_t *mach_s3_blit_worker_create(framebuffer_t *lcd);
bool mach_s3_blit_worker_submit_and_wait(mach_s3_blit_worker_t *worker, mach_s3_blit_fn_t fn, void *user_ctx);
bool mach_s3_blit_worker_submit_async(mach_s3_blit_worker_t *worker, mach_s3_blit_fn_t fn, void *user_ctx);

#endif
