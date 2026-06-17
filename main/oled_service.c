/**
 * @file oled_service.c
 * @brief Implementation of the OLED display service.
 */

#include "oled_service.h"
#include "pomodoro_engine.h"
#include "esp_log.h"
#include "u8g2.h"
#include "esp32_hw_i2c.h"
#include "driver/i2c_master.h"
#include "i2c_manager.h"
#include "rtc_service.h"
#include "config_mgr.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

static const char *TAG = "oled";

static u8g2_t u8g2;
static engine_status_t g_status;

static void oled_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    engine_status_t *status = (engine_status_t *)event_data;
    g_status = *status;
}

static void get_next_block_str(rtc_time_t now, char *out_str, size_t max_len) {
    full_schedule_t sched;
    if (config_mgr_load_schedule(&sched) != ESP_OK) {
        snprintf(out_str, max_len, "No schedule");
        return;
    }
    
    int now_min = now.hour * 60 + now.minute;
    day_schedule_t *day = &sched.weekly[now.weekday];
    
    int next_start_min = 1440; // End of day
    bool found = false;
    
    for (int i = 0; i < day->count; i++) {
        int h, m;
        if (sscanf(day->blocks[i].start, "%d:%d", &h, &m) == 2) {
            int start_min = h * 60 + m;
            if (start_min > now_min && start_min < next_start_min) {
                next_start_min = start_min;
                found = true;
            }
        }
    }
    
    if (found) {
        snprintf(out_str, max_len, "Next: %02d:%02d", next_start_min / 60, next_start_min % 60);
    } else {
        snprintf(out_str, max_len, "Done for today");
    }
}

static void draw_progress_pie(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t r, uint32_t remaining, uint32_t total) {
    if (total == 0) return;
    float ratio = 1.0f - (float)remaining / (float)total;
    float end_angle = ratio * 360.0f;
    
    // Mathematically draw ONLY the remaining pie wedge directly.
    // This avoids using u8g2_DrawDisc and erasing, which leaves artifacts.
    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r2) {
                float angle = atan2f((float)dx, (float)-dy) * 180.0f / 3.14159265f;
                if (angle < 0) angle += 360.0f;
                // Only draw pixels that are ahead of the elapsed angle
                if (angle >= end_angle) {
                    u8g2_DrawPixel(u8g2, x + dx, y + dy);
                }
            }
        }
    }
}

static void oled_task(void *arg) {
    bool blinking = false;
    focus_state_t last_state = -1;
    char next_block_str[32] = "No schedule";

    while (1) {
        u8g2_ClearBuffer(&u8g2);
        
        rtc_time_t now;
        bool rtc_ok = (rtc_get_time(&now) == ESP_OK);

        if (g_status.state == STATE_ADMIN) {
            // --- STANDBY VIEW ---
            if (last_state != STATE_ADMIN) {
                if (rtc_ok) get_next_block_str(now, next_block_str, sizeof(next_block_str));
            }

            if (rtc_ok) {
                char time_str[10];
                snprintf(time_str, sizeof(time_str), "%02d:%02d", now.hour, now.minute);
                
                // Big HH:MM center
                u8g2_SetFont(&u8g2, u8g2_font_logisoso32_tf);
                uint8_t w = u8g2_GetStrWidth(&u8g2, time_str);
                u8g2_DrawStr(&u8g2, (128 - w) / 2, 40, time_str);
                
                // Weekday center-ish
                u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
                const char *weekdays[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
                if (now.weekday >= 0 && now.weekday <= 6) {
                    w = u8g2_GetStrWidth(&u8g2, weekdays[now.weekday]);
                    u8g2_DrawStr(&u8g2, (128 - w) / 2, 53, weekdays[now.weekday]);
                }
                
                // Next block bottom
                u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
                w = u8g2_GetStrWidth(&u8g2, next_block_str);
                u8g2_DrawStr(&u8g2, (128 - w) / 2, 63, next_block_str);
            } else {
                u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
                u8g2_DrawStr(&u8g2, 35, 32, "RTC ERROR");
            }
        } else {
            // --- ACTIVE SESSION VIEW ---
            // TOP YELLOW AREA (y: 0-15)
            const char *state_name = "WORK";
            switch (g_status.state) {
                case STATE_REST: state_name = "RESTING"; break;
                case STATE_PAUSE: state_name = "PAUSED"; break;
                default: break;
            }
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 12, state_name);

            // RTC Time in top-right yellow area
            if (rtc_ok) {
                char rtc_str[8];
                snprintf(rtc_str, sizeof(rtc_str), "%02d:%02d", now.hour, now.minute);
                u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
                u8g2_DrawStr(&u8g2, 98, 12, rtc_str);
            }
            
            u8g2_DrawHLine(&u8g2, 0, 15, 128); // Divider

            // BOTTOM BLUE AREA (y: 16-63)
            char time_str[16];
            uint32_t mins = g_status.remaining_sec / 60;
            uint32_t secs = g_status.remaining_sec % 60;
            snprintf(time_str, sizeof(time_str), "%02lu:%02lu", mins, secs);

            if (g_status.state == STATE_WARNING) {
                if (blinking) {
                    u8g2_SetFont(&u8g2, u8g2_font_logisoso32_tf);
                    char warn_str[8];
                    snprintf(warn_str, sizeof(warn_str), "%lu", g_status.remaining_sec);
                    uint8_t w = u8g2_GetStrWidth(&u8g2, warn_str);
                    u8g2_DrawStr(&u8g2, (128 - w) / 2, 55, warn_str);
                }
                blinking = !blinking;
            } else {
                u8g2_SetFont(&u8g2, u8g2_font_ncenB18_tr);
                int8_t ascent = u8g2_GetAscent(&u8g2);
                uint8_t text_baseline_y = 42 + (ascent / 2);
                u8g2_DrawStr(&u8g2, 5, text_baseline_y, time_str);

                draw_progress_pie(&u8g2, 105, 42, 18, g_status.remaining_sec, g_status.total_sec);
                u8g2_DrawCircle(&u8g2, 105, 42, 20, U8G2_DRAW_ALL);
            }
        }

        last_state = g_status.state;
        u8g2_SendBuffer(&u8g2);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void oled_service_init(void) {
    static u8g2_esp32_i2c_ctx_t i2c_ctx;
    memset(&i2c_ctx, 0, sizeof(i2c_ctx));
    i2c_ctx.cfg = (u8g2_esp32_i2c_config_t)U8G2_ESP32_I2C_CONFIG_DEFAULT();
    i2c_ctx.bus_handle = i2c_bus_handle;
    i2c_ctx.initialized = 1; // Mark as initialized so port doesn't try to create new bus
    u8g2_esp32_i2c_set_default_context(&i2c_ctx);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(POMODORO_EVENTS, 
                                                        POMODORO_EVENT_STATE_UPDATE, 
                                                        &oled_event_handler, 
                                                        NULL, 
                                                        NULL));

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
    xTaskCreate(oled_task, "oled_task", 4096, NULL, 4, NULL);
}
