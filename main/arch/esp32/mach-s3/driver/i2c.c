#include "sensors/i2c_bus.h"

esp_err_t i2c_bus_create(const i2c_bus_create_config_t *config, i2c_master_bus_handle_t *out_bus)
{
	if (config == NULL || out_bus == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	i2c_master_bus_config_t bus_config = {
	        .clk_source = config->clk_source,
	        .i2c_port = config->port,
	        .sda_io_num = config->sda_io_num,
	        .scl_io_num = config->scl_io_num,
	        .glitch_ignore_cnt = config->glitch_ignore_cnt,
	        .flags.enable_internal_pullup = config->enable_internal_pullup,
	};

	return i2c_new_master_bus(&bus_config, out_bus);
}
