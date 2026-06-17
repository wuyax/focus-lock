/**
 * @file scheduler_service.h
 * @brief Scheduler service for automatic Pomodoro session management.
 */

#pragma once
#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Initialize the scheduler service.
 * @return ESP_OK on success.
 */
esp_err_t scheduler_service_init(void);

/**
 * @brief Notifies the scheduler to reload its configuration from SPIFFS.
 */
void scheduler_service_reload(void);

/**
 * @brief Debug function to simulate time and check scheduling logic.
 * @param hour Simulated hour (0-23).
 * @param minute Simulated minute (0-59).
 * @param weekday Simulated weekday (0-6).
 */
void scheduler_service_debug_check(int hour, int minute, int weekday);
