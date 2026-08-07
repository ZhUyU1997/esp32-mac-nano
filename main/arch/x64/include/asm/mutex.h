/* x64 architecture mutex — POSIX-based */
#ifndef ASM_MUTEX_H
#define ASM_MUTEX_H

#include <pthread.h>

typedef pthread_mutex_t asm_mutex_t;

#define ASM_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline void asm_mutex_init(asm_mutex_t *m)
{
	/* PTHREAD_MUTEX_INITIALIZER already covers static init */
	(void)m;
}

static inline void asm_mutex_lock(asm_mutex_t *m)
{
	pthread_mutex_lock(m);
}

static inline void asm_mutex_unlock(asm_mutex_t *m)
{
	pthread_mutex_unlock(m);
}

#endif
