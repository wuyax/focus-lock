#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config_mgr.h"
#include "pomodoro_engine.h"
#include "rgb_service.h"
#include "button_service.h"
#include "usb_hid_service.h"

static const char *TAG = "main";
focuslock_config_t global_config;
focuslock_stats_t global_stats;

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FocusLock");
    
    ESP_ERROR_CHECK(config_mgr_init());
    config_mgr_load(&global_config);
    config_mgr_load_stats(&global_stats);
    
    ESP_LOGI(TAG, "Config loaded. Work: %lu min, Rest: %lu min", 
             global_config.work_time_min, global_config.rest_time_min);
             
    pomodoro_engine_init();
    rgb_service_init(status_queue);
    usb_hid_service_init(status_queue);
    button_service_init();
             
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
