#include "usb_hid_service.h"
#include "pomodoro_engine.h"
#include "config_mgr.h"
#include "shortcut_parser.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char *TAG = "usb_hid";
extern focuslock_config_t global_config;

static void usb_hid_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    static focus_state_t last_state = STATE_WORK;

    while (1) {
        if (xQueuePeek(q, &status, portMAX_DELAY)) {
            if (status.state == STATE_REST && last_state != STATE_REST) {
                uint8_t modifier = 0;
                uint8_t key = 0;

                if (parse_shortcut(global_config.lock_shortcut, &modifier, &key)) {
                    ESP_LOGI(TAG, "Sending lock command (%s)", global_config.lock_shortcut);
                    
                    // Wait for HID to be ready
                    int retry = 10;
                    while (!tud_hid_ready() && retry-- > 0) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }

                    uint8_t keycode[6] = {key, 0, 0, 0, 0, 0};
                    // Use report_id = 0 as our descriptor doesn't define one
                    tud_hid_keyboard_report(0, modifier, keycode);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    tud_hid_keyboard_report(0, 0, NULL);
                } else {
                    ESP_LOGE(TAG, "Failed to parse shortcut: %s", global_config.lock_shortcut);
                }
            }
            last_state = status.state;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//--------------- USB Descriptors ---------------//
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4002, // Generic PID
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    return desc_hid_report;
}

static uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), 0x81, 8, 10)
};

static char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "FocusLock",                   // 1: Manufacturer
    "FocusLock Device",            // 2: Product
    "123456",                      // 3: Serials
    "FocusLock HID",               // 4: HID Interface
};

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
}

void usb_hid_service_init(QueueHandle_t q) {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &desc_device,
        .string_descriptor = string_desc_arr,
        .string_descriptor_count = sizeof(string_desc_arr)/sizeof(string_desc_arr[0]),
        .external_phy = false,
        .configuration_descriptor = desc_configuration,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB HID Driver installed");

    xTaskCreate(usb_hid_task, "usb_hid_task", 4096, (void *)q, 5, NULL);
}
