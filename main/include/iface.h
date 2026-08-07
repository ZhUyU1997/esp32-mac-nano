#ifndef IFACE_H
#define IFACE_H

#include <stddef.h>

#include "macro/base.h"
#include "common/list.h"

typedef struct iface_base_t {
	struct list_head list;
	const char *ifname;
	const char *name;
} iface_base_t;

int iface_register(iface_base_t *impl);
int iface_unregister(iface_base_t *impl);
iface_base_t *iface_search(const char *ifname, const char *name, size_t name_len);
struct list_head *iface_get_list(const char *ifname);

#define iface_name(type) type##_iface_name

#define impl(...) CAT(__iface_impl_, VARIADIC_SIZE(__VA_ARGS__))(__VA_ARGS__)
#define __iface_impl_1(type) __iface_impl_named(__COUNTER__, type)
#define __iface_impl_2(name, type) __iface_impl_named(name, type)

#define __iface_impl_named(name, type) __iface_impl_named2(name, type)
#define __iface_impl_named2(name, type)                                                                                                                        \
	static type __attribute__((used)) __iface_impl_obj_##name;                                                                                             \
	static void __attribute__((constructor)) __iface_impl_init_##name(void)                                                                                \
	{                                                                                                                                                      \
		if (__iface_impl_obj_##name.ifname == NULL) {                                                                                                  \
			__iface_impl_obj_##name.ifname = iface_name(type);                                                                                     \
		}                                                                                                                                              \
		if (__iface_impl_obj_##name.list.next == NULL || __iface_impl_obj_##name.list.prev == NULL) {                                                  \
			init_list_head(&__iface_impl_obj_##name.list);                                                                                         \
		}                                                                                                                                              \
		(void)iface_register((iface_base_t *)&__iface_impl_obj_##name);                                                                                \
	}                                                                                                                                                      \
	static type __iface_impl_obj_##name =

#endif
