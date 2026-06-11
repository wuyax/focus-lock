#include "oled_service.h"
#include "pomodoro_engine.h"
#include "esp_log.h"
#include "u8g2.h"
#include "esp32_hw_i2c.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

static const char *TAG = "oled";

#define I2C_SDA_PIN 1
#define I2C_SCL_PIN 2

static u8g2_t u8g2;

static void draw_progress_ring(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t r, uint32_t remaining, uint32_t total) {
    u8g2_DrawCircle(u8g2, x, y, r, U8G2_DRAW_ALL);
    if (total > 0) {
        float angle = 2.0f * 3.14159f * (1.0f - (float)remaining / (float)total);
        int8_t hand_x = (int8_t)(sinf(angle) * (float)r);
        int8_t hand_y = (int8_t)(-cosf(angle) * (float)r);
        u8g2_DrawLine(u8g2, x, y, x + hand_x, y + hand_y);
    }
}

static void oled_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    bool blinking = false;

    while (1) {
        if (xQueuePeek(q, &status, 0)) {
            u8g2_ClearBuffer(&u8g2);
            
            char time_str[16];
            uint32_t mins = status.remaining_sec / 60;
            uint32_t secs = status.remaining_sec % 60;
            snprintf(time_str, sizeof(time_str), "%02lu:%02lu", mins, secs);

            if (status.state == STATE_WARNING) {
                // Large blinking seconds
                if (blinking) {
                    u8g2_SetFont(&u8g2, u8g2_font_logisoso32_tf);
                    char warn_str[8];
                    snprintf(warn_str, sizeof(warn_str), "%lu", status.remaining_sec);
                    uint8_t w = u8g2_GetStrWidth(&u8g2, warn_str);
                    u8g2_DrawStr(&u8g2, (128 - w) / 2, 45, warn_str);
                }
                blinking = !blinking;
            } else {
                const char *state_name = "WORK";
                switch (status.state) {
                    case STATE_REST: state_name = "REST"; break;
                    case STATE_PAUSE: state_name = "PAUSE"; break;
                    case STATE_ADMIN: state_name = "ADMIN"; break;
                    default: break;
                }

                u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
                u8g2_DrawStr(&u8g2, 0, 15, state_name);

                u8g2_SetFont(&u8g2, u8g2_font_ncenB18_tr);
                u8g2_DrawStr(&u8g2, 0, 45, time_str);

                draw_progress_ring(&u8g2, 100, 32, 25, status.remaining_sec, status.total_sec);
            }

            u8g2_SendBuffer(&u8g2);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void oled_service_init(QueueHandle_t q) {
    u8g2_esp32_i2c_ctx_t i2c_ctx = {
        .cfg = U8G2_ESP32_I2C_CONFIG_DEFAULT(),
    };
    i2c_ctx.cfg.sda_pin = I2C_SDA_PIN;
    i2c_ctx.cfg.scl_pin = I2C_SCL_PIN;
    u8g2_esp32_i2c_set_default_context(&i2c_ctx);

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_esp32_hw_i2c,
        u8x8_gpio_and_delay_esp32_i2c
    );

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);

    ESP_LOGI(TAG, "OLED Service Initialized");
    xTaskCreate(oled_task, "oled_task", 4096, (void *)q, 4, NULL);
}
