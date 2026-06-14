# Codebase Cleanup and Refactoring Design

## Overview
A comprehensive project-wide cleanup of the `espblink` application focused on removing unused code, standardizing documentation, and migrating from a polling-based architecture to a robust, event-driven architecture using ESP-IDF's default event loop.

## 1. Clean Up Unused Code
- **Target**: `main/test_shortcut_parser.c` and its invocation in `main/main.c`.
- **Action**: Delete the file and remove the `test_shortcut_parser()` call from `app_main()`.
- **Reasoning**: The project uses a dedicated root-level `pytest_blink.py` for integration testing. Ad-hoc test files in the `main` application directory constitute dead/unused code in production.

## 2. Documentation Standardization
- **Target**: All `_service.c` files and `config_mgr.c`.
- **Action**: Add standard Doxygen `@file` headers explaining the purpose of each module.
- **Action**: Add Doxygen `@brief` blocks to all public functions defined in header files (e.g., initialization routines like `button_service_init`).

## 3. Encapsulation and Robustness

### 3.1 Configuration Manager Consolidation
- **Target**: `config_mgr.c`.
- **Action**: Extract the hardcoded configuration default values (e.g., `work_time_min = 45`, `rest_time_min = 5`, `lock_shortcut = "Win+L"`) from both the missing-NVS block and the blob-read-failure block into a single internal helper `static void set_default_config(focuslock_config_t *cfg)`.
- **Benefit**: DRY principle; modifying default values only happens in one place.

### 3.2 Event-Driven Architecture Migration
- **Target**: `pomodoro_engine.c`, `buzzer_service.c`, `network_service.c`, `oled_service.c`, `rgb_service.c`, `usb_hid_service.c`, `main.c`.
- **Current State**: Services spawn dedicated FreeRTOS tasks that loop continuously, calling `xQueuePeek(status_queue)` every ~50-100ms.
- **Action**:
  - Define an ESP Event loop base: `ESP_EVENT_DECLARE_BASE(POMODORO_EVENTS);` in a shared header (e.g., `pomodoro_engine.h`).
  - Add event IDs (e.g., `POMODORO_EVENT_STATE_UPDATE`).
  - **Broadcaster**: Modify `pomodoro_engine.c` to post events containing the `engine_status_t` payload using `esp_event_post()` instead of writing to `status_queue`.
  - **Subscribers**: Remove the infinite polling loops and task creations (`xTaskCreate`) where applicable (like `rgb_service` or `network_service` which only react to state changes). Instead, register callbacks using `esp_event_handler_register(POMODORO_EVENTS, POMODORO_EVENT_STATE_UPDATE, ...)`.
  - *Exception*: Components that need to block or run continuously (like `button_service` reading GPIOs, or `oled_service` updating animations) will retain their tasks, but state updates will be pushed to them via direct FreeRTOS queues or they can simply cache the state received via the event callback. Wait, actually, the event callback runs in the default event loop task. If a service needs to blink an LED or drive a buzzer for a duration, it might still need a task.
  - **Refined Subscriber Action**: 
    - `network_service`: Can be pure callback (no task needed).
    - `buzzer_service`: Keep task, wait on a task-specific queue fed by the event handler, or handle simple non-blocking logic in the callback. Since buzzer beeps use `vTaskDelay`, we will keep the task but use `xQueueReceive(..., portMAX_DELAY)` instead of peeking.
    - `rgb_service`: Keep task (needs to animate breathing). Use `xQueueReceive` to receive state updates without timeout, or just a global variable updated by the event handler.
    - `usb_hid_service`: Keep task (needs to retry sending USB). Use `xQueueReceive`.
    - `main.c`: Can print status via callback instead of polling in `app_main`.
- **Benefit**: Eliminates CPU churn from busy polling, centralizes state distribution, and significantly improves system robustness and responsiveness.

## Testing Strategy
After implementation, we will compile using `idf.py build` (if available via devcontainer) or rely on static analysis, followed by standard testing procedures to verify the hardware functions as before.