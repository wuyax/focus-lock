#include "button_service.h"
#include "pomodoro_engine.h"
#include "buzzer_service.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_GPIO 0

static void button_task(void *arg) {
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);
    
    bool last_state = true;
    uint32_t press_time = 0;
    uint32_t release_time = 0;
    int click_count = 0;
    bool long_press_notified = false;
    
    while(1) {
        bool current_state = gpio_get_level(BUTTON_GPIO);
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (last_state == true && current_state == false) { 
            // Button pressed (active low)
            press_time = now;
            long_press_notified = false;
        } else if (last_state == false && current_state == false) {
            // Button held
            uint32_t duration = now - press_time;
            if (duration >= 5000 && !long_press_notified) {
                buzzer_beep(100);
                long_press_notified = true;
            }
        } else if (last_state == false && current_state == true) { 
            // Button released
            uint32_t duration = now - press_time;
            if (duration > 50) { // Debounce
                if (duration >= 5000) {
                    pomodoro_engine_send_event(EVT_LONG_PRESS);
                    click_count = 0;
                } else {
                    click_count++;
                    release_time = now;
                }
            }
        } else if (last_state == true && current_state == true) { 
            // Button is released, check for multi-click timeout
            if (click_count > 0 && (now - release_time > 300)) { 
                if (click_count == 1) {
                    pomodoro_engine_send_event(EVT_CLICK);
                } else if (click_count >= 2) {
                    pomodoro_engine_send_event(EVT_DOUBLE_CLICK);
                }
                click_count = 0;
            }
        }
        
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void button_service_init(void) {
    xTaskCreate(button_task, "btn_task", 2048, NULL, 5, NULL);
}
