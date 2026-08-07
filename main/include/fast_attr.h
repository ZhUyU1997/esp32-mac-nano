/* ESP-IDF: IRAM_ATTR / DRAM_ATTR; other hosts: empty. */
#ifndef FAST_ATTR_H
#define FAST_ATTR_H

#if defined(ESP_PLATFORM)
#include "esp_attr.h"
#define FAST_FUNC_ATTR IRAM_ATTR
#define FAST_DATA_ATTR DRAM_ATTR
#else
#define FAST_FUNC_ATTR
#define FAST_DATA_ATTR
#endif

#if defined(__GNUC__)
#define FAST_O3_ATTR __attribute__((optimize("O3")))
#else
#define FAST_O3_ATTR
#endif

#endif /* FAST_ATTR_H */
