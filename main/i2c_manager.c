#include "i2c_manager.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "i2c_mgr";
i2c_master_bus_handle_t i2c_bus_handle = NULL;

// Force reset the I2C bus to release stuck slave devices
static void i2c_bus_reset(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = (1ULL << I2C_SCL_PIN) | (1ULL << I2C_SDA_PIN),
    };
    gpio_config(&io_conf);

    // Send 9 clock pulses on SCL to let slave release SDA
    for (int i = 0; i < 9; i++) {
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    gpio_set_level(I2C_SCL_PIN, 1);
}

esp_err_t i2c_manager_init(void) {
    if (i2c_bus_handle != NULL) return ESP_OK;

    // 1. Perform bus reset for robustness
    i2c_bus_reset();

    // 2. Initialize I2C master bus
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C Driver Install Failed: 0x%x", ret);
        return ret;
    }

    ESP_LOGI(TAG, "I2C initialized on SDA:%d SCL:%d", I2C_SDA_PIN, I2C_SCL_PIN);
    return ESP_OK;
}
