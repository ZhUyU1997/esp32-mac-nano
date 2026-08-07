#include <string.h>

#include "iface.h"

typedef struct iface_group_t {
	struct list_head list;
	const char *ifname;
	struct list_head impls;
} iface_group_t;

static LIST_HEAD(g_iface_groups);
static iface_group_t g_group_pool[32];

static iface_group_t *find_group(const char *ifname)
{
	iface_group_t *g;
	list_for_each_entry(g, &g_iface_groups, list) {
		if (g->ifname != NULL && strcmp(g->ifname, ifname) == 0) {
			return g;
		}
	}
	return NULL;
}

static iface_group_t *alloc_group(const char *ifname)
{
	for (size_t i = 0; i < sizeof(g_group_pool) / sizeof(g_group_pool[0]); i++) {
		iface_group_t *g = &g_group_pool[i];
		if (g->ifname == NULL) {
			g->ifname = ifname;
			init_list_head(&g->list);
			init_list_head(&g->impls);
			list_add_tail(&g->list, &g_iface_groups);
			return g;
		}
	}
	return NULL;
}

int iface_register(iface_base_t *impl)
{
	if (impl == NULL || impl->ifname == NULL || impl->name == NULL) {
		return -1;
	}
	iface_group_t *g = find_group(impl->ifname);
	if (g == NULL) {
		g = alloc_group(impl->ifname);
		if (g == NULL) {
			return -1;
		}
	}
	list_add_tail(&impl->list, &g->impls);
	return 0;
}

int iface_unregister(iface_base_t *impl)
{
	if (impl == NULL) {
		return -1;
	}
	if (impl->list.next == NULL || impl->list.prev == NULL) {
		return -1;
	}
	list_del(&impl->list);
	init_list_head(&impl->list);
	return 0;
}

iface_base_t *iface_search(const char *ifname, const char *name, size_t name_len)
{
	if (ifname == NULL || name == NULL || name_len == 0) {
		return NULL;
	}
	iface_group_t *g = find_group(ifname);
	if (g == NULL) {
		return NULL;
	}
	iface_base_t *p;
	list_for_each_entry(p, &g->impls, list) {
		size_t n = strlen(p->name);
		if (n == name_len && memcmp(p->name, name, name_len) == 0) {
			return p;
		}
	}
	return NULL;
}

struct list_head *iface_get_list(const char *ifname)
{
	static struct list_head empty;
	static int inited;
	if (!inited) {
		init_list_head(&empty);
		inited = 1;
	}
	if (ifname == NULL) {
		return &empty;
	}
	iface_group_t *g = find_group(ifname);
	if (g == NULL) {
		return &empty;
	}
	return &g->impls;
}
