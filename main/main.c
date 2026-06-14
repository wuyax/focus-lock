/**
 * @file main.c
 * @brief Entry point for the FocusLock application.
 * 
 * This file initializes all services and starts the Pomodoro engine.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "config_mgr.h"
#include "pomodoro_engine.h"
#include "rgb_service.h"
#include "button_service.h"
#include "usb_hid_service.h"
#include "shortcut_parser.h"
#include "oled_service.h"
#include "network_service.h"
#include "i2c_manager.h"
#include "rtc_service.h"
#include "buzzer_service.h"

static const char *TAG = "main";
focuslock_config_t global_config;
focuslock_stats_t global_stats;

static void main_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    engine_status_t *status = (engine_status_t *)event_data;
    const char *state_names[] = {"WORK", "WARNING", "REST", "PAUSE", "ADMIN"};
    uint32_t mins = status->remaining_sec / 60;
    uint32_t secs = status->remaining_sec % 60;
    ESP_LOGI(TAG, "Status: [%s] Time: %02lu:%02lu", 
             state_names[status->state], mins, secs);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FocusLock");
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_ERROR_CHECK(config_mgr_init());
    config_mgr_load(&global_config);
    config_mgr_load_stats(&global_stats);
    
    ESP_LOGI(TAG, "Config loaded. Work: %lu min, Rest: %lu min", 
             global_config.work_time_min, global_config.rest_time_min);

    ESP_ERROR_CHECK(i2c_manager_init());
    ESP_ERROR_CHECK(rtc_service_init());
             
    pomodoro_engine_init();
    rgb_service_init();
    usb_hid_service_init();
    oled_service_init();
    network_service_init();
    buzzer_service_init();
    button_service_init();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(POMODORO_EVENTS, 
                                                        POMODORO_EVENT_STATE_UPDATE, 
                                                        &main_event_handler, 
                                                        NULL, 
                                                        NULL));
             
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
