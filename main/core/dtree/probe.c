#include "probe.h"
#include "dtree_json.h"
#include "dtree_probe.h"

#include <stdlib.h>

int probe_device(const char *json, size_t len)
{
	if (json == NULL || len == 0) {
		return 0;
	}

	const size_t toks_count = 512u;
	jsmntok_t *toks = NULL;
	toks = (jsmntok_t *)malloc(sizeof(jsmntok_t) * toks_count);
	if (toks == NULL) {
		return 0;
	}

	dtree_view_t dt;
	if (dtree_json_parse(&dt, json, len, toks, toks_count) != 0) {
		free(toks);
		return 0;
	}

	const int ok = dtree_probe_all(&dt);
	free(toks);
	return ok;
}
