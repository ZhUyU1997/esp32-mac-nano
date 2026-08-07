#ifndef DTREE_JSON_H
#define DTREE_JSON_H

#include <stddef.h>
#include <stdint.h>

#ifndef JSMN_HEADER
#define JSMN_HEADER
#define DTREE_JSON_UNDEF_JSMN_HEADER
#endif
#include "jsmn.h"
#ifdef DTREE_JSON_UNDEF_JSMN_HEADER
#undef JSMN_HEADER
#undef DTREE_JSON_UNDEF_JSMN_HEADER
#endif

typedef struct dtree_view_t {
	const char *json;
	size_t json_len;
	const jsmntok_t *toks;
	int tok_count;
} dtree_view_t;

typedef struct dtree_node_t {
	int key;
	int val;
} dtree_node_t;

int dtree_json_parse(dtree_view_t *out, const char *json, size_t json_len, jsmntok_t *toks, size_t tok_cap);

int dtree_root_first(const dtree_view_t *dt, dtree_node_t *out_node);
int dtree_root_next(const dtree_view_t *dt, dtree_node_t *inout_node);

int dtree_node_driver(const dtree_view_t *dt, const dtree_node_t *node, const char **out_name, size_t *out_len);
int dtree_node_id(const dtree_view_t *dt, const dtree_node_t *node, const char **out_id, size_t *out_len);
int dtree_node_addr(const dtree_view_t *dt, const dtree_node_t *node, const char **out_addr, size_t *out_len);

int dtree_prop_get(const dtree_view_t *dt, const dtree_node_t *node, const char *key, int *out_tok);
int dtree_prop_str(const dtree_view_t *dt, int tok, const char **out_s, size_t *out_len);
int dtree_prop_u32(const dtree_view_t *dt, int tok, uint32_t *out_v);

#endif
