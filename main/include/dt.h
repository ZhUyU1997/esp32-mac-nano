#ifndef DT_H
#define DT_H

#include <stddef.h>
#include <stdint.h>

#include "dtree_json.h"

typedef struct dtnode_t {
	const dtree_view_t *dt;
	dtree_node_t node;
	char name[64];
	uint64_t id;
	uint64_t addr;
} dtnode_t;

void dtnode_init(dtnode_t *out, const dtree_view_t *dt, const dtree_node_t *node);

const char *dt_read_name(const dtnode_t *n);
int dt_read_id(const dtnode_t *n);
uint64_t dt_read_address(const dtnode_t *n);

int dt_read_bool(const dtnode_t *n, const char *name, int def);
int dt_read_int(const dtnode_t *n, const char *name, int def);
long long dt_read_long(const dtnode_t *n, const char *name, long long def);
const char *dt_read_string(const dtnode_t *n, const char *name, const char *def);
uint32_t dt_read_u32(const dtnode_t *n, const char *name, uint32_t def);
uint64_t dt_read_u64(const dtnode_t *n, const char *name, uint64_t def);
dtnode_t *dt_read_object(const dtnode_t *n, const char *name, dtnode_t *out);
int dt_read_array_length(const dtnode_t *n, const char *name);
dtnode_t *dt_read_array_object(const dtnode_t *n, const char *name, int index, dtnode_t *out);

#endif
