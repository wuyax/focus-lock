#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
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

extern void test_shortcut_parser();

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
             
    test_shortcut_parser();

    ESP_ERROR_CHECK(i2c_manager_init());
    ESP_ERROR_CHECK(rtc_service_init());
             
    pomodoro_engine_init();
    rgb_service_init(status_queue);
    usb_hid_service_init(status_queue);
    oled_service_init(status_queue);
    network_service_init(status_queue);
    button_service_init();
             
    while (1) {
        engine_status_t status;
        if (xQueuePeek(status_queue, &status, 0)) {
            const char *state_names[] = {"WORK", "WARNING", "REST", "PAUSE", "ADMIN"};
            uint32_t mins = status.remaining_sec / 60;
            uint32_t secs = status.remaining_sec % 60;
            ESP_LOGI(TAG, "Status: [%s] Time: %02lu:%02lu", 
                     state_names[status.state], mins, secs);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
