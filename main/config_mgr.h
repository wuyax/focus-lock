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

esp_err_t config_mgr_init(void);
esp_err_t config_mgr_load(focuslock_config_t *cfg);
esp_err_t config_mgr_save(const focuslock_config_t *cfg);
esp_err_t config_mgr_load_stats(focuslock_stats_t *stats);
esp_err_t config_mgr_save_stats(const focuslock_stats_t *stats);
