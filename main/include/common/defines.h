/*
 * SPDX-FileCopyrightText: 2022 Yu Zhu
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>

#define container_of(ptr, type, member)                                                                                                                        \
	({                                                                                                                                                     \
		const typeof(((type *)0)->member) *__mptr = (ptr);                                                                                             \
		(type *)((char *)__mptr - offsetof(type, member));                                                                                             \
	})
