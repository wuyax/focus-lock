#include "rgb_service.h"
#include "pomodoro_engine.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS2812_GPIO CONFIG_BLINK_GPIO 

static led_strip_handle_t led_strip;

static void init_ws2812(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = 1, 
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

static void rgb_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    focus_state_t local_state = STATE_WORK;
    uint8_t breath_val = 0;
    int8_t breath_dir = 5;
    
    while(1) {
        if (xQueuePeek(q, &status, 0)) {
            local_state = status.state;
        }

        switch(local_state) {
            case STATE_WORK:
                led_strip_set_pixel(led_strip, 0, 0, 255, 0); 
                break;
            case STATE_WARNING:
                led_strip_set_pixel(led_strip, 0, breath_val, breath_val, 0);
                breath_val += breath_dir;
                if (breath_val >= 250) breath_dir = -5;
                else if (breath_val <= 5) breath_dir = 5;
                break;
            case STATE_REST:
                led_strip_set_pixel(led_strip, 0, 255, 0, 0); 
                break;
            case STATE_PAUSE:
            case STATE_ADMIN:
                led_strip_set_pixel(led_strip, 0, 0, 0, 255); 
                break;
        }
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void rgb_service_init(QueueHandle_t q) {
    init_ws2812();
    xTaskCreate(rgb_task, "rgb_task", 2048, (void *)q, 4, NULL);
}
