#include "oled_service.h"
#include "pomodoro_engine.h"
#include "esp_log.h"
#include "u8g2.h"
#include "esp32_hw_i2c.h"
#include "driver/i2c_master.h"
#include "i2c_manager.h"
#include "rtc_service.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

static const char *TAG = "oled";

static u8g2_t u8g2;

static void draw_progress_pie(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t r, uint32_t remaining, uint32_t total) {
    if (total == 0) return;
    float ratio = 1.0f - (float)remaining / (float)total;
    int end_angle = (int)(ratio * 360.0f);
    
    // Use filled pie chart
    u8g2_DrawDisc(u8g2, x, y, r, U8G2_DRAW_ALL);
    u8g2_SetDrawColor(u8g2, 0); // Eraser mode
    
    // Draw the "remaining" portion as an empty slice or just draw the pie properly
    // U8G2 doesn't have a direct "DrawFilledPie", so we simulate with Circle + Triangle/Line if needed
    // Simplified: Draw a solid circle that gets smaller or use u8g2_DrawFilledEllipse
    // For a real pie chart, we'll draw a sequence of lines from center to perimeter
    for (int a = 0; end_angle > 0 && a <= end_angle; a++) {
        float rad = (float)(a - 90) * 0.01745329f; // -90 to start at top
        int8_t px = (int8_t)(cosf(rad) * (float)r);
        int8_t py = (int8_t)(sinf(rad) * (float)r);
        u8g2_DrawLine(u8g2, x, y, x + px, y + py);
    }
    u8g2_SetDrawColor(u8g2, 1); // Restore normal mode
}

static void oled_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    bool blinking = false;

    while (1) {
        if (xQueuePeek(q, &status, 0)) {
            u8g2_ClearBuffer(&u8g2);
            
            // --- TOP YELLOW AREA (y: 0-15) ---
            const char *state_name = "WORK";
            switch (status.state) {
                case STATE_REST: state_name = "RESTING"; break;
                case STATE_PAUSE: state_name = "PAUSED"; break;
                case STATE_ADMIN: state_name = "CONFIG"; break;
                default: break;
            }
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 12, state_name);

            // RTC Time in top-right yellow area
            rtc_time_t now;
            if (rtc_get_time(&now) == ESP_OK) {
                char rtc_str[8];
                snprintf(rtc_str, sizeof(rtc_str), "%02d:%02d", now.hour, now.minute);
                u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
                u8g2_DrawStr(&u8g2, 98, 12, rtc_str);
            }
            
            u8g2_DrawHLine(&u8g2, 0, 15, 128); // Divider

            // --- BOTTOM BLUE AREA (y: 16-63) ---
            char time_str[16];
            uint32_t mins = status.remaining_sec / 60;
            uint32_t secs = status.remaining_sec % 60;
            snprintf(time_str, sizeof(time_str), "%02lu:%02lu", mins, secs);

            if (status.state == STATE_WARNING) {
                if (blinking) {
                    u8g2_SetFont(&u8g2, u8g2_font_logisoso32_tf);
                    char warn_str[8];
                    snprintf(warn_str, sizeof(warn_str), "%lu", status.remaining_sec);
                    uint8_t w = u8g2_GetStrWidth(&u8g2, warn_str);
                    u8g2_DrawStr(&u8g2, (128 - w) / 2, 55, warn_str);
                }
                blinking = !blinking;
            } else {
                u8g2_SetFont(&u8g2, u8g2_font_ncenB18_tr);
                int8_t ascent = u8g2_GetAscent(&u8g2);
                uint8_t text_baseline_y = 42 + (ascent / 2);
                u8g2_DrawStr(&u8g2, 5, text_baseline_y, time_str);

                // Pie chart moved down to blue area and shrunken
                // Center (105, 42), Radius 18 (fits within y=24 to y=60)
                draw_progress_pie(&u8g2, 105, 42, 18, status.remaining_sec, status.total_sec);
                // Outer ring
                u8g2_DrawCircle(&u8g2, 105, 42, 20, U8G2_DRAW_ALL);
            }

            u8g2_SendBuffer(&u8g2);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void oled_service_init(QueueHandle_t q) {
    static u8g2_esp32_i2c_ctx_t i2c_ctx;
    memset(&i2c_ctx, 0, sizeof(i2c_ctx));
    i2c_ctx.cfg = (u8g2_esp32_i2c_config_t)U8G2_ESP32_I2C_CONFIG_DEFAULT();
    i2c_ctx.bus_handle = i2c_bus_handle;
    i2c_ctx.initialized = 1; // Mark as initialized so port doesn't try to create new bus
    u8g2_esp32_i2c_set_default_context(&i2c_ctx);

    // Try to probe the OLED at its default address 0x3C (7-bit)
    esp_err_t err = i2c_master_probe(i2c_bus_handle, 0x3C, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED Display NOT found on I2C bus (0x3C)! Disabling OLED service.");
        return;
    }

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
