#include <string.h>
#include <stdlib.h>

#include "dt.h"

static int parse_u64_base0(const char *s, size_t n, uint64_t *out_v)
{
	if (s == NULL || out_v == NULL || n == 0) {
		return 0;
	}
	char buf[32];
	if (n >= sizeof(buf)) {
		return 0;
	}
	memcpy(buf, s, n);
	buf[n] = '\0';
	char *end = NULL;
	unsigned long long v = strtoull(buf, &end, 0);
	if (end == buf) {
		return 0;
	}
	*out_v = (uint64_t)v;
	return 1;
}

void dtnode_init(dtnode_t *out, const dtree_view_t *dt, const dtree_node_t *node)
{
	if (out == NULL || dt == NULL || node == NULL) {
		return;
	}

	memset(out, 0, sizeof(*out));
	out->dt = dt;
	out->node = *node;

	const char *drv = NULL;
	size_t drv_len = 0;
	if (dtree_node_driver(dt, node, &drv, &drv_len)) {
		size_t n = drv_len;
		if (n >= sizeof(out->name)) {
			n = sizeof(out->name) - 1;
		}
		memcpy(out->name, drv, n);
		out->name[n] = '\0';
	}

	const char *id_s = NULL;
	size_t id_len = 0;
	if (dtree_node_id(dt, node, &id_s, &id_len)) {
		(void)parse_u64_base0(id_s, id_len, &out->id);
	}

	const char *addr_s = NULL;
	size_t addr_len = 0;
	if (dtree_node_addr(dt, node, &addr_s, &addr_len)) {
		(void)parse_u64_base0(addr_s, addr_len, &out->addr);
	}
}

const char *dt_read_name(const dtnode_t *n)
{
	if (n == NULL) {
		return "";
	}
	return n->name;
}

int dt_read_id(const dtnode_t *n)
{
	if (n == NULL) {
		return 0;
	}
	return (int)n->id;
}

uint64_t dt_read_address(const dtnode_t *n)
{
	if (n == NULL) {
		return 0;
	}
	return n->addr;
}

static int dt_find_tok(const dtnode_t *n, const char *key)
{
	if (n == NULL || key == NULL) {
		return -1;
	}
	int tok = -1;
	if (!dtree_prop_get(n->dt, &n->node, key, &tok)) {
		return -1;
	}
	if (tok < 0 || tok >= n->dt->tok_count) {
		return -1;
	}
	return tok;
}

static int tok_skip(const dtree_view_t *dt, int i)
{
	if (dt == NULL) {
		return i;
	}
	if (i < 0 || i >= dt->tok_count) {
		return i;
	}
	int j = i + 1;
	switch (dt->toks[i].type) {
	case JSMN_PRIMITIVE:
	case JSMN_STRING:
		return j;
	case JSMN_ARRAY:
		for (int k = 0; k < dt->toks[i].size && j < dt->tok_count; k++) {
			j = tok_skip(dt, j);
		}
		return j;
	case JSMN_OBJECT:
		for (int k = 0; k < dt->toks[i].size && j < dt->tok_count; k++) {
			j = tok_skip(dt, j);
			j = tok_skip(dt, j);
		}
		return j;
	default:
		return j;
	}
}

dtnode_t *dt_read_object(const dtnode_t *n, const char *key, dtnode_t *out)
{
	if (n == NULL || out == NULL || key == NULL) {
		return NULL;
	}
	int tok = dt_find_tok(n, key);
	if (tok < 0) {
		return NULL;
	}
	if (n->dt->toks[tok].type != JSMN_OBJECT) {
		return NULL;
	}
	dtree_node_t child;
	child.key = -1;
	child.val = tok;
	dtnode_init(out, n->dt, &child);
	out->id = 0;
	out->addr = 0;
	out->name[0] = '\0';
	return out;
}

int dt_read_array_length(const dtnode_t *n, const char *key)
{
	if (n == NULL || key == NULL) {
		return 0;
	}
	int tok = dt_find_tok(n, key);
	if (tok < 0) {
		return 0;
	}
	if (n->dt->toks[tok].type != JSMN_ARRAY) {
		return 0;
	}
	return n->dt->toks[tok].size;
}

dtnode_t *dt_read_array_object(const dtnode_t *n, const char *key, int index, dtnode_t *out)
{
	if (n == NULL || out == NULL || key == NULL) {
		return NULL;
	}
	if (index < 0) {
		return NULL;
	}
	int tok = dt_find_tok(n, key);
	if (tok < 0) {
		return NULL;
	}
	if (n->dt->toks[tok].type != JSMN_ARRAY) {
		return NULL;
	}
	int count = n->dt->toks[tok].size;
	if (index >= count) {
		return NULL;
	}

	int elem = tok + 1;
	for (int i = 0; i < index; i++) {
		elem = tok_skip(n->dt, elem);
	}
	if (elem < 0 || elem >= n->dt->tok_count) {
		return NULL;
	}
	if (n->dt->toks[elem].type != JSMN_OBJECT) {
		return NULL;
	}
	dtree_node_t child;
	child.key = -1;
	child.val = elem;
	dtnode_init(out, n->dt, &child);
	out->id = 0;
	out->addr = 0;
	out->name[0] = '\0';
	return out;
}

const char *dt_read_string(const dtnode_t *n, const char *key, const char *def)
{
	static char buf[8][256];
	static unsigned int buf_i;

	if (n == NULL || key == NULL) {
		return def;
	}
	int tok = dt_find_tok(n, key);
	if (tok < 0) {
		return def;
	}
	const char *s = NULL;
	size_t len = 0;
	if (!dtree_prop_str(n->dt, tok, &s, &len)) {
		return def;
	}
	char *out = buf[buf_i++ % (sizeof(buf) / sizeof(buf[0]))];
	if (len >= 256) {
		return def;
	}
	memcpy(out, s, len);
	out[len] = '\0';
	return out;
}

uint64_t dt_read_u64(const dtnode_t *n, const char *key, uint64_t def)
{
	if (n == NULL || key == NULL) {
		return def;
	}
	int tok = dt_find_tok(n, key);
	if (tok < 0) {
		return def;
	}
	const jsmntok_t *t = &n->dt->toks[tok];
	if (t->type != JSMN_PRIMITIVE) {
		return def;
	}
	const char *s = n->dt->json + t->start;
	size_t len = (size_t)(t->end - t->start);
	uint64_t v = 0;
	if (parse_u64_base0(s, len, &v)) {
		return v;
	}
	return def;
}

uint32_t dt_read_u32(const dtnode_t *n, const char *key, uint32_t def)
{
	return (uint32_t)dt_read_u64(n, key, (uint64_t)def);
}

int dt_read_bool(const dtnode_t *n, const char *key, int def)
{
	if (n == NULL || key == NULL) {
		return def;
	}
	int tok = dt_find_tok(n, key);
	if (tok < 0) {
		return def;
	}
	const jsmntok_t *t = &n->dt->toks[tok];
	if (t->type != JSMN_PRIMITIVE) {
		return def;
	}
	const char *s = n->dt->json + t->start;
	size_t len = (size_t)(t->end - t->start);
	if (len == 4 && memcmp(s, "true", 4) == 0) {
		return 1;
	}
	if (len == 5 && memcmp(s, "false", 5) == 0) {
		return 0;
	}
	uint64_t v = 0;
	if (parse_u64_base0(s, len, &v)) {
		return v ? 1 : 0;
	}
	return def;
}

int dt_read_int(const dtnode_t *n, const char *key, int def)
{
	return (int)dt_read_u64(n, key, (uint64_t)def);
}

long long dt_read_long(const dtnode_t *n, const char *key, long long def)
{
	return (long long)dt_read_u64(n, key, (uint64_t)def);
}
