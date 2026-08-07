#include <string.h>

#include "dtree_json.h"

static int tok_eq(const dtree_view_t *dt, int tok, const char *s)
{
	const jsmntok_t *t = &dt->toks[tok];
	if (t->type != JSMN_STRING) {
		return 0;
	}
	size_t n = (size_t)(t->end - t->start);
	return strlen(s) == n && memcmp(dt->json + t->start, s, n) == 0;
}

static int tok_skip(const dtree_view_t *dt, int i)
{
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

int dtree_json_parse(dtree_view_t *out, const char *json, size_t json_len, jsmntok_t *toks, size_t tok_cap)
{
	if (out == NULL || json == NULL || json_len == 0 || toks == NULL || tok_cap == 0) {
		return -1;
	}
	jsmn_parser p;
	jsmn_init(&p);
	int r = jsmn_parse(&p, json, json_len, toks, (unsigned int)tok_cap);
	if (r <= 0) {
		return -1;
	}
	if (toks[0].type != JSMN_OBJECT) {
		return -1;
	}
	out->json = json;
	out->json_len = json_len;
	out->toks = toks;
	out->tok_count = r;
	return 0;
}

int dtree_root_first(const dtree_view_t *dt, dtree_node_t *out_node)
{
	if (dt == NULL || out_node == NULL) {
		return 0;
	}
	if (dt->tok_count <= 0 || dt->toks[0].type != JSMN_OBJECT) {
		return 0;
	}
	if (dt->toks[0].size <= 0) {
		return 0;
	}
	out_node->key = 1;
	out_node->val = 2;
	if (out_node->val >= dt->tok_count) {
		return 0;
	}
	return 1;
}

int dtree_root_next(const dtree_view_t *dt, dtree_node_t *inout_node)
{
	if (dt == NULL || inout_node == NULL) {
		return 0;
	}
	int next = tok_skip(dt, inout_node->val);
	int key = next;
	int val = next + 1;
	if (key < 0 || val < 0 || val >= dt->tok_count) {
		return 0;
	}
	inout_node->key = key;
	inout_node->val = val;
	return 1;
}

static int node_key_slice(const dtree_view_t *dt, const dtree_node_t *node, const char **out_s, size_t *out_len)
{
	if (dt == NULL || node == NULL || out_s == NULL || out_len == NULL) {
		return 0;
	}
	if (node->key < 0 || node->key >= dt->tok_count) {
		return 0;
	}
	const jsmntok_t *t = &dt->toks[node->key];
	if (t->type != JSMN_STRING) {
		return 0;
	}
	*out_s = dt->json + t->start;
	*out_len = (size_t)(t->end - t->start);
	return 1;
}

int dtree_node_driver(const dtree_view_t *dt, const dtree_node_t *node, const char **out_name, size_t *out_len)
{
	const char *s;
	size_t n;
	if (!node_key_slice(dt, node, &s, &n)) {
		return 0;
	}
	size_t i = 0;
	while (i < n && s[i] != ':' && s[i] != '@') {
		i++;
	}
	*out_name = s;
	*out_len = i;
	return 1;
}

int dtree_node_id(const dtree_view_t *dt, const dtree_node_t *node, const char **out_id, size_t *out_len)
{
	const char *s;
	size_t n;
	if (!node_key_slice(dt, node, &s, &n)) {
		return 0;
	}
	const char *p = memchr(s, ':', n);
	if (!p) {
		return 0;
	}
	size_t start = (size_t)(p - s) + 1;
	size_t end = n;
	const char *q = memchr(s, '@', n);
	if (q && (size_t)(q - s) > start) {
		end = (size_t)(q - s);
	}
	if (end <= start) {
		return 0;
	}
	*out_id = s + start;
	*out_len = end - start;
	return 1;
}

int dtree_node_addr(const dtree_view_t *dt, const dtree_node_t *node, const char **out_addr, size_t *out_len)
{
	const char *s;
	size_t n;
	if (!node_key_slice(dt, node, &s, &n)) {
		return 0;
	}
	const char *p = memchr(s, '@', n);
	if (!p) {
		return 0;
	}
	size_t start = (size_t)(p - s) + 1;
	if (start >= n) {
		return 0;
	}
	*out_addr = s + start;
	*out_len = n - start;
	return 1;
}

int dtree_prop_get(const dtree_view_t *dt, const dtree_node_t *node, const char *key, int *out_tok)
{
	if (dt == NULL || node == NULL || key == NULL || out_tok == NULL) {
		return 0;
	}
	int obj = node->val;
	if (obj < 0 || obj >= dt->tok_count || dt->toks[obj].type != JSMN_OBJECT) {
		return 0;
	}
	int i = obj + 1;
	for (int kv = 0; kv < dt->toks[obj].size && i + 1 < dt->tok_count; kv++) {
		int k = i;
		int v = i + 1;
		if (tok_eq(dt, k, key)) {
			*out_tok = v;
			return 1;
		}
		i = tok_skip(dt, v);
	}
	return 0;
}

int dtree_prop_str(const dtree_view_t *dt, int tok, const char **out_s, size_t *out_len)
{
	if (dt == NULL || out_s == NULL || out_len == NULL) {
		return 0;
	}
	if (tok < 0 || tok >= dt->tok_count) {
		return 0;
	}
	const jsmntok_t *t = &dt->toks[tok];
	if (t->type != JSMN_STRING) {
		return 0;
	}
	*out_s = dt->json + t->start;
	*out_len = (size_t)(t->end - t->start);
	return 1;
}

static int parse_u32_10(const char *s, size_t n, uint32_t *out_v)
{
	uint32_t v = 0;
	if (n == 0) {
		return 0;
	}
	for (size_t i = 0; i < n; i++) {
		if (s[i] < '0' || s[i] > '9') {
			return 0;
		}
		uint32_t d = (uint32_t)(s[i] - '0');
		v = v * 10u + d;
	}
	*out_v = v;
	return 1;
}

int dtree_prop_u32(const dtree_view_t *dt, int tok, uint32_t *out_v)
{
	if (dt == NULL || out_v == NULL) {
		return 0;
	}
	if (tok < 0 || tok >= dt->tok_count) {
		return 0;
	}
	const jsmntok_t *t = &dt->toks[tok];
	if (t->type != JSMN_PRIMITIVE) {
		return 0;
	}
	const char *s = dt->json + t->start;
	size_t n = (size_t)(t->end - t->start);
	return parse_u32_10(s, n, out_v);
}
