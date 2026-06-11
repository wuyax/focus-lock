#include "usb_hid_service.h"
#include "pomodoro_engine.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char *TAG = "usb_hid";

static void usb_hid_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    static focus_state_t last_state = STATE_WORK;

    while (1) {
        if (xQueuePeek(q, &status, portMAX_DELAY)) {
            if (status.state == STATE_REST && last_state != STATE_REST) {
                ESP_LOGI(TAG, "Sending lock command (Win+L)");
                uint8_t keycode[6] = {HID_KEY_L, 0, 0, 0, 0, 0};
                tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, KEYBOARD_MODIFIER_LEFTGUI, keycode);
                vTaskDelay(pdMS_TO_TICKS(50));
                tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
            }
            last_state = status.state;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// HID Report Descriptor
static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// Invoked when received GET_REPORT control request
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    return desc_hid_report;
}

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

// Invoked when received SET_REPORT control request or received data on OUT endpoint
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

void usb_hid_service_init(QueueHandle_t q) {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB HID Driver installed");

    xTaskCreate(usb_hid_task, "usb_hid_task", 4096, (void *)q, 5, NULL);
}
