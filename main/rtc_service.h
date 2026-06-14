/**
 * @brief RTC service for managing real-time clock hardware.
 */

#pragma once
#include <time.h>
#include "esp_err.h"

typedef struct {
    int hour;
    int minute;
    int second;
} rtc_time_t;

/**
 * @brief Initialize the RTC service.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t rtc_service_init(void);

/**
 * @brief Get the current time from the RTC.
 * @param time Pointer to a struct to store the retrieved time.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t rtc_get_time(rtc_time_t *time);

/**
 * @brief Set the time on the RTC.
 * @param time Pointer to a struct containing the time to set.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t rtc_set_time(const rtc_time_t *time);
