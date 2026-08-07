#include <string.h>

#include "common/log.h"
#include "dtree_probe.h"
#include "driver.h"
#include "device.h"
#include "dt.h"

static int probe_pass(const dtree_view_t *dt)
{
	dtree_node_t n;
	int count = 0;

	if (!dtree_root_first(dt, &n)) {
		return 0;
	}
	for (;;) {
		const char *drv_name = NULL;
		size_t drv_len = 0;
		if (dtree_node_driver(dt, &n, &drv_name, &drv_len)) {
			driver_t *drv = search_driver(drv_name, drv_len);
			dtnode_t dn;
			dtnode_init(&dn, dt, &n);
			const char *status = dt_read_string(&dn, "status", "okay");
			if (status != NULL && strcmp(status, "disabled") == 0) {
				goto next_node;
			}

			struct device_t *dev = NULL;
			if (drv != NULL) {
				dev = drv->probe(drv, &dn);
			}
			if (drv != NULL && dev != NULL) {
				count++;
				LOGI("Probe device '%s' with %.*s", dev->name ? dev->name : "", (int)drv_len, drv_name);
			} else {
				LOGE("Fail to probe device with %.*s", (int)drv_len, drv_name);
			}
		}
	next_node:
		if (!dtree_root_next(dt, &n)) {
			break;
		}
	}
	return count;
}

int dtree_probe_all(const dtree_view_t *dt)
{
	if (dt == NULL || dt->toks == NULL || dt->tok_count <= 0) {
		return 0;
	}
	return probe_pass(dt);
}
