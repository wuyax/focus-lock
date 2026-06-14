# Design Spec: Standardize Documentation

## Purpose
The goal is to professionalize the codebase by adding standard Doxygen documentation to all source and header files in the `main/` directory. This improves maintainability and makes the code easier to understand for new developers.

## Scope
The following files in the `main/` directory are included:
- `button_service.c/.h`
- `buzzer_service.c/.h`
- `config_mgr.c/.h`
- `network_service.c/.h`
- `oled_service.c/.h`
- `rgb_service.c/.h`
- `rtc_service.c/.h`
- `usb_hid_service.c/.h`
- `i2c_manager.c/.h`
- `web_server.c/.h`
- `dns_server.c/.h`
- `pomodoro_engine.c/.h` (check/complete)
- `shortcut_parser.c/.h`

## Requirements
1. **Source Files (.c):** Add a `@file` header at the top of each file with a brief description of its purpose.
2. **Header Files (.h):** Add a `@brief` comment above every public function declaration.
3. **Style:** Match the existing Doxygen style in `pomodoro_engine.c`.

## Implementation Strategy
A subagent will be used to perform these changes systematically across all files.

### Example Styles

**File Header (.c):**
```c
/**
 * @file button_service.c
 * @brief Handles GPIO button input with debounce and multi-click detection.
 */
```

**Function Header (.h):**
```c
/**
 * @brief Initializes the button service task.
 */
void button_service_init(void);
```

## Verification
- Run `idf.py build` (if available) to ensure no syntax errors were introduced.
- Manual inspection of a subset of files to verify documentation quality.
