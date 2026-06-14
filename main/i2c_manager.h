/**
 * @file i2c_manager.h
 * @brief I2C master bus manager.
 */

#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_SDA_PIN 41
#define I2C_SCL_PIN 42

extern i2c_master_bus_handle_t i2c_bus_handle;

/**
 * @brief Initializes the I2C master bus.
 * @return ESP_OK on success, or error code on failure.
 */
esp_err_t i2c_manager_init(void);
