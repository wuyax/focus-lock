/**
 * @file scheduler_service.c
 * @brief Implementation of the scheduler service.
 */

#include "scheduler_service.h"
#include "config_mgr.h"
#include "rtc_service.h"
#include "pomodoro_engine.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "scheduler";
static full_schedule_t schedule;
static EventGroupHandle_t scheduler_event_group;
#define RELOAD_BIT BIT0

static int time_to_min(const char *time_str) {
    int h, m;
    if (sscanf(time_str, "%d:%d", &h, &m) == 2) {
        return h * 60 + m;
    }
    return -1;
}

static schedule_mode_t* find_mode(int mode_id) {
    for (int i = 0; i < schedule.mode_count; i++) {
        if (schedule.modes[i].id == mode_id) {
            return &schedule.modes[i];
        }
    }
    return NULL;
}

static void check_schedule(int hour, int minute, int weekday) {
    if (weekday < 0 || weekday > 6) return;
    
    int now_min = hour * 60 + minute;
    day_schedule_t *day = &schedule.weekly[weekday];
    
    for (int i = 0; i < day->count; i++) {
        schedule_block_t *block = &day->blocks[i];
        int start_min = time_to_min(block->start);
        int end_min = time_to_min(block->end);
        
        if (now_min == start_min) {
            schedule_mode_t *mode = find_mode(block->mode_id);
            if (mode) {
                ESP_LOGI(TAG, "Scheduled start: %s (mode: %s)", block->start, mode->name);
                pomodoro_engine_start_mode(mode->work_min, mode->rest_min, mode->warn_sec);
            }
        } else if (now_min == end_min) {
            ESP_LOGI(TAG, "Scheduled end: %s, marking pending exit", block->end);
            pomodoro_engine_pending_exit();
        }
    }
}

void scheduler_service_debug_check(int hour, int minute, int weekday) {
    ESP_LOGI(TAG, "Debug check: %02d:%02d weekday=%d", hour, minute, weekday);
    check_schedule(hour, minute, weekday);
}

static void scheduler_task(void *arg) {
    ESP_LOGI(TAG, "Scheduler task started");
    config_mgr_load_schedule(&schedule);
    
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // Wait for 1 minute or a reload signal
        EventBits_t bits = xEventGroupWaitBits(scheduler_event_group, RELOAD_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(60000));
        
        if (bits & RELOAD_BIT) {
            ESP_LOGI(TAG, "Reloading schedule...");
            config_mgr_load_schedule(&schedule);
            // After reload, we might want to check immediately, but let's wait for next minute for simplicity
        }
        
        rtc_time_t now;
        if (rtc_get_time(&now) == ESP_OK) {
            check_schedule(now.hour, now.minute, now.weekday);
        }
        
        // Align to the next minute boundary if possible, but for simplicity just wait 60s
        // vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(60000));
    }
}

esp_err_t scheduler_service_init(void) {
    scheduler_event_group = xEventGroupCreate();
    if (scheduler_event_group == NULL) return ESP_ERR_NO_MEM;
    
    xTaskCreate(scheduler_task, "scheduler_task", 4096, NULL, 4, NULL);
    return ESP_OK;
}

void scheduler_service_reload(void) {
    if (scheduler_event_group) {
        xEventGroupSetBits(scheduler_event_group, RELOAD_BIT);
    }
}
