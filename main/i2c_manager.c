#include "i2c_manager.h"
#include "esp_log.h"

static const char *TAG = "i2c_mgr";
i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t i2c_manager_init(void) {
    if (i2c_bus_handle != NULL) return ESP_OK;

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
    }
    return ret;
}
