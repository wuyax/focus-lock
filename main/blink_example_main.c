#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "smart_intersection";

// 外接红绿灯 GPIO 引脚
#define PIN_RED    4
#define PIN_YELLOW 5
#define PIN_GREEN  6

// HC-SR505 传感器 OUT 引脚
#define PIN_SENSOR 7

// 板载 WS2812B 引脚 (从 sdkconfig 中获取)
#define WS2812_GPIO CONFIG_BLINK_GPIO

static led_strip_handle_t led_strip;

// --- 初始化部分 ---

static void init_ws2812(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = 1, 
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
    #error "unsupported LED strip backend"
#endif
    led_strip_clear(led_strip);
}

void init_gpio_lights(void)
{
    gpio_reset_pin(PIN_RED);
    gpio_set_direction(PIN_RED, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(PIN_YELLOW);
    gpio_set_direction(PIN_YELLOW, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(PIN_GREEN);
    gpio_set_direction(PIN_GREEN, GPIO_MODE_OUTPUT);
}

void init_sensor(void)
{
    gpio_reset_pin(PIN_SENSOR);
    // 配置为输入模式
    gpio_set_direction(PIN_SENSOR, GPIO_MODE_INPUT);
}


// --- 任务 1：传感器与 WS2812B 控制任务 ---
// 这是一个独立的线程，不会被红绿灯的超长延时阻塞
void sensor_task(void *pvParameter)
{
    bool is_crazy_blinking = false;

    while (1) {
        int motion_detected = gpio_get_level(PIN_SENSOR);

        if (motion_detected) {
            // 检测到人体，疯狂闪烁 (红蓝警灯爆闪效果)
            led_strip_set_pixel(led_strip, 0, 255, 0, 0); // 红
            led_strip_refresh(led_strip);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            
            led_strip_set_pixel(led_strip, 0, 0, 0, 255); // 蓝
            led_strip_refresh(led_strip);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            
            is_crazy_blinking = true;
        } else {
            // 未检测到人体
            if (is_crazy_blinking) {
                ESP_LOGI(TAG, "Motion stopped, returning to 10%% white.");
                is_crazy_blinking = false;
            }
            // 白色，亮度 10% (大约是 25/255)
            led_strip_set_pixel(led_strip, 0, 25, 25, 25);
            led_strip_refresh(led_strip);
            
            // 没人的时候不需要刷新那么频繁
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}


// --- 任务 2：红绿灯逻辑 (主循环) ---

// 设置外接红绿灯的状态 (不再联动 WS2812B)
void set_lights(int r, int y, int g)
{
    gpio_set_level(PIN_RED, r);
    gpio_set_level(PIN_YELLOW, y);
    gpio_set_level(PIN_GREEN, g);
}

void run_light_cycle(int pin_to_blink, int solid_sec, int blink_sec)
{
    int r = (pin_to_blink == PIN_RED);
    int y = (pin_to_blink == PIN_YELLOW);
    int g = (pin_to_blink == PIN_GREEN);

    // 常亮
    set_lights(r, y, g);
    vTaskDelay((solid_sec * 1000) / portTICK_PERIOD_MS);

    // 闪烁
    for (int i = 0; i < blink_sec * 2; i++) {
        set_lights(0, 0, 0); 
        vTaskDelay(250 / portTICK_PERIOD_MS);
        set_lights(r, y, g); 
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // 初始化所有外设
    init_gpio_lights();
    init_ws2812();
    init_sensor();

    // 初始状态红绿灯全部熄灭
    set_lights(0, 0, 0);

    ESP_LOGI(TAG, "System started!");

    // 创建一个独立的 FreeRTOS 任务来运行传感器和 WS2812B (优先级设为 5)
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

    // 下面是主任务：专心跑红绿灯死循环
    while (1) {
        ESP_LOGI(TAG, "GREEN light phase");
        run_light_cycle(PIN_GREEN, 27, 3);
        
        ESP_LOGI(TAG, "YELLOW light phase");
        run_light_cycle(PIN_YELLOW, 1, 2);
        
        ESP_LOGI(TAG, "RED light phase");
        run_light_cycle(PIN_RED, 27, 3);
    }
}
