#include "rtc_service.h"
#include "i2c_manager.h"
#include "esp_log.h"

static const char *TAG = "rtc";
static i2c_master_dev_handle_t rtc_dev_handle;

esp_err_t rtc_service_init(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 100000,
    };
    return i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &rtc_dev_handle);
}

static uint8_t bcd2dec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }

esp_err_t rtc_get_time(rtc_time_t *time) {
    uint8_t data[3];
    uint8_t reg = 0x00;
    esp_err_t ret = i2c_master_transmit_receive(rtc_dev_handle, &reg, 1, data, 3, -1);
    if (ret == ESP_OK) {
        time->second = bcd2dec(data[0] & 0x7F);
        time->minute = bcd2dec(data[1]);
        time->hour = bcd2dec(data[2] & 0x3F);
    }
    return ret;
}
