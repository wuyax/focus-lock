#include "shortcut_parser.h"
#include <string.h>
#include <ctype.h>
#include "class/hid/hid_device.h"

typedef struct {
    const char *name;
    uint8_t mask;
} modifier_map_t;

static const modifier_map_t modifiers[] = {
    {"Ctrl", KEYBOARD_MODIFIER_LEFTCTRL},
    {"Shift", KEYBOARD_MODIFIER_LEFTSHIFT},
    {"Alt", KEYBOARD_MODIFIER_LEFTALT},
    {"Win", KEYBOARD_MODIFIER_LEFTGUI},
    {"Cmd", KEYBOARD_MODIFIER_LEFTGUI},
    {"Meta", KEYBOARD_MODIFIER_LEFTGUI},
};

bool parse_shortcut(const char *shortcut_str, uint8_t *modifier, uint8_t *keycode) {
    if (!shortcut_str || !modifier || !keycode) return false;

    *modifier = 0;
    *keycode = 0;

    char buf[64];
    strncpy(buf, shortcut_str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, "+");
    char *last_token = NULL;

    while (token != NULL) {
        last_token = token;
        bool is_modifier = false;
        for (int i = 0; i < sizeof(modifiers) / sizeof(modifiers[0]); i++) {
            if (strcasecmp(token, modifiers[i].name) == 0) {
                *modifier |= modifiers[i].mask;
                is_modifier = true;
                break;
            }
        }
        
        token = strtok(NULL, "+");
        if (token == NULL) {
            // This is the last token, should be the keycode if it's not a modifier
            if (is_modifier) {
                // Shortcut ends with a modifier? Invalid for our use case
                return false;
            }
            
            if (strlen(last_token) == 1) {
                char c = toupper((unsigned char)last_token[0]);
                if (c >= 'A' && c <= 'Z') {
                    *keycode = HID_KEY_A + (c - 'A');
                } else if (c >= '0' && c <= '9') {
                    if (c == '0') *keycode = HID_KEY_0;
                    else *keycode = HID_KEY_1 + (c - '1');
                } else {
                    return false;
                }
            } else {
                if (strcasecmp(last_token, "Space") == 0) *keycode = HID_KEY_SPACE;
                else if (strcasecmp(last_token, "Enter") == 0) *keycode = HID_KEY_ENTER;
                else if (strcasecmp(last_token, "Esc") == 0) *keycode = HID_KEY_ESCAPE;
                else if (strcasecmp(last_token, "Tab") == 0) *keycode = HID_KEY_TAB;
                else if (strcasecmp(last_token, "F1") == 0) *keycode = HID_KEY_F1;
                else if (strcasecmp(last_token, "F2") == 0) *keycode = HID_KEY_F2;
                else if (strcasecmp(last_token, "F3") == 0) *keycode = HID_KEY_F3;
                else if (strcasecmp(last_token, "F4") == 0) *keycode = HID_KEY_F4;
                else if (strcasecmp(last_token, "F5") == 0) *keycode = HID_KEY_F5;
                else if (strcasecmp(last_token, "F6") == 0) *keycode = HID_KEY_F6;
                else if (strcasecmp(last_token, "F7") == 0) *keycode = HID_KEY_F7;
                else if (strcasecmp(last_token, "F8") == 0) *keycode = HID_KEY_F8;
                else if (strcasecmp(last_token, "F9") == 0) *keycode = HID_KEY_F9;
                else if (strcasecmp(last_token, "F10") == 0) *keycode = HID_KEY_F10;
                else if (strcasecmp(last_token, "F11") == 0) *keycode = HID_KEY_F11;
                else if (strcasecmp(last_token, "F12") == 0) *keycode = HID_KEY_F12;
                else return false;
            }
        }
    }

    return (*keycode != 0);
}
