#pragma once
#include <time.h>
#include "esp_err.h"

typedef struct {
    int hour;
    int minute;
    int second;
} rtc_time_t;

esp_err_t rtc_service_init(void);
esp_err_t rtc_get_time(rtc_time_t *time);
esp_err_t rtc_set_time(const rtc_time_t *time);
