#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "shortcut_parser.h"
#include "esp_log.h"
#include "class/hid/hid_device.h"

static const char *TAG = "test_shortcut_parser";

void test_shortcut_parser() {
    uint8_t modifier = 0;
    uint8_t keycode = 0;

    // Test case 1: "Win+L"
    assert(parse_shortcut("Win+L", &modifier, &keycode));
    assert(modifier == KEYBOARD_MODIFIER_LEFTGUI);
    assert(keycode == HID_KEY_L);
    ESP_LOGI(TAG, "Win+L: mod=0x%02x, key=0x%02x", modifier, keycode);

    // Test case 2: "Ctrl+Cmd+Q"
    assert(parse_shortcut("Ctrl+Cmd+Q", &modifier, &keycode));
    assert(modifier == (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTGUI));
    assert(keycode == HID_KEY_Q);
    ESP_LOGI(TAG, "Ctrl+Cmd+Q: mod=0x%02x, key=0x%02x", modifier, keycode);

    // Test case 3: Invalid string
    assert(!parse_shortcut("Invalid", &modifier, &keycode));
}
