#include "buzzer_service.h"
#include "pomodoro_engine.h"
#include "config_mgr.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define BUZZER_GPIO 10

static const char *TAG = "buzzer_service";
extern focuslock_config_t global_config;

// 使用开漏模式，依靠外部上拉至 5V 来关断
static void buzzer_set_active(bool active) {
    if (active) {
        gpio_set_level(BUZZER_GPIO, 0); // 响
    } else {
        gpio_set_level(BUZZER_GPIO, 1); // 依靠上拉电阻关断
    }
}

void buzzer_beep(uint32_t duration_ms) {
    if (!global_config.buzzer_enabled) return; 
    
    buzzer_set_active(true);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_set_active(false);
}

static void buzzer_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    focus_state_t last_state = STATE_WORK;

    // 配置为开漏输出模式
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
    };
    gpio_config(&io_conf);
    gpio_set_level(BUZZER_GPIO, 1); // 默认不响

    ESP_LOGI(TAG, "Buzzer Ready. System in WORK state.");

    while(1) {
        if (xQueuePeek(q, &status, 0)) {
            if (status.state != last_state) {
                if (status.state == STATE_WARNING) {
                    for(int i=0; i<3; i++) { buzzer_beep(100); vTaskDelay(pdMS_TO_TICKS(900)); }
                } else if (status.state == STATE_REST) {
                    for(int i=0; i<5; i++) { buzzer_beep(50); vTaskDelay(pdMS_TO_TICKS(50)); }
                } else if (last_state == STATE_REST && status.state == STATE_WORK) {
                    buzzer_beep(500);
                } else if (status.state == STATE_PAUSE) {
                    buzzer_beep(50); // 暂停响一声短的
                }
                last_state = status.state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void buzzer_service_init(QueueHandle_t status_queue) {
    if (xTaskCreate(buzzer_task, "buzzer_task", 2048, status_queue, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create buzzer task");
    }
}
