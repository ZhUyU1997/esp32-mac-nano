#ifndef WEB_CONTROL_H
#define WEB_CONTROL_H

#include "esp_err.h"

/* Start/stop the web control stack (softAP + http server). Idempotent. */
esp_err_t web_control_enable(void);
void web_control_disable(void);
bool web_control_is_enabled(void);

#endif
