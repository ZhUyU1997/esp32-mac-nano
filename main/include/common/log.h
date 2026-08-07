#pragma once

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

#ifndef LOG_USE_COLOR
#define LOG_USE_COLOR 1
#endif

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#if LOG_USE_COLOR
#define LOGC_RESET "\033[0m"
#define LOGC_RED "\033[31m"
#define LOGC_YELLOW "\033[33m"
#define LOGC_GREEN "\033[32m"
#define LOGC_CYAN "\033[36m"
#define LOGC_GRAY "\033[90m"
#else
#define LOGC_RESET ""
#define LOGC_RED ""
#define LOGC_YELLOW ""
#define LOGC_GREEN ""
#define LOGC_CYAN ""
#define LOGC_GRAY ""
#endif

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5

#define LOG_IMPL_(lvl_num, lvl_tag, lvl_color, fmt, ...)                                                                                                       \
	do {                                                                                                                                                   \
		if (LOG_LEVEL >= (lvl_num)) {                                                                                                                  \
			printf(lvl_color lvl_tag " %s:%d/%s > " fmt LOGC_RESET "\n", __FILENAME__, __LINE__, __func__, ##__VA_ARGS__);                         \
		}                                                                                                                                              \
	} while (0)

#define LOGE(fmt, ...) LOG_IMPL_(LOG_LEVEL_ERROR, "E", LOGC_RED, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_IMPL_(LOG_LEVEL_WARN, "W", LOGC_YELLOW, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_IMPL_(LOG_LEVEL_INFO, "I", LOGC_GREEN, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) LOG_IMPL_(LOG_LEVEL_DEBUG, "D", LOGC_CYAN, fmt, ##__VA_ARGS__)
#define LOGT(fmt, ...) LOG_IMPL_(LOG_LEVEL_TRACE, "T", LOGC_GRAY, fmt, ##__VA_ARGS__)

#define PANIC(fmt, ...)                                                                                                                                        \
	do {                                                                                                                                                   \
		LOGE(fmt, ##__VA_ARGS__);                                                                                                                      \
		(void)fflush(stdout);                                                                                                                          \
		assert(0);                                                                                                                                     \
	} while (0)
