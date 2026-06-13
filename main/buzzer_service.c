#include "buzzer_service.h"
#include "pomodoro_engine.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#define BUZZER_GPIO 3

static void beep(uint32_t duration_ms) {
    gpio_set_level(BUZZER_GPIO, 0); // Active Low
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    gpio_set_level(BUZZER_GPIO, 1);
}

static void buzzer_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    focus_state_t last_state = STATE_WORK;

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 1);

    while(1) {
        if (xQueuePeek(q, &status, 0)) {
            if (status.state != last_state) {
                if (status.state == STATE_WARNING) {
                    for(int i=0; i<3; i++) { beep(100); vTaskDelay(pdMS_TO_TICKS(900)); }
                } else if (status.state == STATE_REST) {
                    for(int i=0; i<5; i++) { beep(50); vTaskDelay(pdMS_TO_TICKS(50)); }
                } else if (last_state == STATE_REST && status.state == STATE_WORK) {
                    beep(500);
                }
                last_state = status.state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void buzzer_service_init(QueueHandle_t status_queue) {
    xTaskCreate(buzzer_task, "buzzer_task", 2048, status_queue, 5, NULL);
}
