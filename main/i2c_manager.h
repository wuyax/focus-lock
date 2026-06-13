#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_SDA_PIN 1
#define I2C_SCL_PIN 2

extern i2c_master_bus_handle_t i2c_bus_handle;

esp_err_t i2c_manager_init(void);
