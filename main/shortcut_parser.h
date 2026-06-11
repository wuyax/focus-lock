#ifndef SHORTCUT_PARSER_H
#define SHORTCUT_PARSER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Parse a shortcut string (e.g., "Win+L", "Ctrl+Cmd+Q") into modifiers and keycode.
 * 
 * @param shortcut_str The shortcut string to parse.
 * @param modifier Pointer to store the modifier mask.
 * @param keycode Pointer to store the HID keycode.
 * @return true if parsing was successful, false otherwise.
 */
bool parse_shortcut(const char *shortcut_str, uint8_t *modifier, uint8_t *keycode);

#endif // SHORTCUT_PARSER_H
