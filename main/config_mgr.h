/**
 * @file config_mgr.h
 * @brief Configuration manager for storing and retrieving settings from NVS.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Configuration structure for FocusLock.
 */
typedef struct {
    uint32_t work_time_min;      /**< Duration of work phase in minutes. */
    uint32_t rest_time_min;      /**< Duration of rest phase in minutes. */
    uint32_t warning_time_sec;   /**< Warning time before phase ends in seconds. */
    char lock_shortcut[16];      /**< USB HID shortcut string to lock the screen. */
    bool repeat_lock;            /**< Whether to repeat the lock command during rest. */
    uint32_t repeat_interval_sec; /**< Interval between repeated lock commands. */
    bool buzzer_enabled;         /**< Whether the buzzer is enabled. */
    uint8_t led_brightness;      /**< RGB LED brightness (0-255). */
} focuslock_config_t;

/**
 * @brief Statistics structure for tracking usage.
 */
typedef struct {
    uint32_t total_pomodoros;    /**< Total number of completed work phases. */
    uint32_t total_work_min;     /**< Total accumulated work time in minutes. */
    uint32_t total_rest_min;     /**< Total accumulated rest time in minutes. */
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
