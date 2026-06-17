#pragma once
#include "esp_err.h"

/**
 * @brief Initializes and mounts the SPIFFS partition.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t spiffs_manager_init(void);

/**
 * @brief Unmounts the SPIFFS partition.
 * @return ESP_OK on success.
 */
esp_err_t spiffs_manager_deinit(void);
