#pragma once

#include <stdint.h>
#include <stdbool.h>

bool scd30_soft_reset(void);

bool scd30_read_firmware_version(uint16_t fv[1]);

bool scd30_get_data_ready_status(void);
bool scd30_read_measurement(float m[3]);

bool scd30_trigger_continuous_measurement(uint16_t pressure);
bool scd30_stop_continuous_measurement(void);

bool scd30_set_measurement_interval(uint16_t interval);
bool scd30_get_measurement_interval(uint16_t interval[1]);

bool scd30_set_altitude(uint16_t altitude);
bool scd30_get_altitude(uint16_t altitude[1]);

bool scd30_set_temperature_offset(uint16_t offset);
bool scd30_get_temperature_offset(uint16_t offset[1]);
