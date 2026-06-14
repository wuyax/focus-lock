#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    uint32_t work_time_min;
    uint32_t rest_time_min;
    uint32_t warning_time_sec;
    char lock_shortcut[16];
    bool repeat_lock;
    uint32_t repeat_interval_sec;
    bool buzzer_enabled;
    uint8_t led_brightness;
} focuslock_config_t;

typedef struct {
    uint32_t total_pomodoros;
    uint32_t total_work_min;
    uint32_t total_rest_min;
} focuslock_stats_t;

/**
 * @brief Initializes the NVS flash for configuration storage.
 * @return ESP_OK on success.
 */
esp_err_t config_mgr_init(void);

/**
 * @brief Loads the focus lock configuration from NVS.
 * @param cfg Pointer to the configuration structure to fill.
 * @return ESP_OK on success.
 */
esp_err_t config_mgr_load(focuslock_config_t *cfg);

/**
 * @brief Saves the focus lock configuration to NVS.
 * @param cfg Pointer to the configuration structure to save.
 * @return ESP_OK on success.
 */
esp_err_t config_mgr_save(const focuslock_config_t *cfg);

/**
 * @brief Loads the focus lock statistics from NVS.
 * @param stats Pointer to the statistics structure to fill.
 * @return ESP_OK on success.
 */
esp_err_t config_mgr_load_stats(focuslock_stats_t *stats);

/**
 * @brief Saves the focus lock statistics to NVS.
 * @param stats Pointer to the statistics structure to save.
 * @return ESP_OK on success.
 */
esp_err_t config_mgr_save_stats(const focuslock_stats_t *stats);
