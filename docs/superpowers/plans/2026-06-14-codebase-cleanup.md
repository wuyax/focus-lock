# Codebase Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clean up dead code, add Doxygen comments, and migrate the system to a robust, event-driven architecture using `esp_event`.

**Architecture:** We will delete unused test files, standardize documentation for maintainability, and replace `xQueuePeek` polling in service tasks with direct `esp_event` callbacks to improve CPU utilization and robustness.

**Tech Stack:** C, ESP-IDF FreeRTOS, ESP Event Loop.

---

### Task 1: Clean Up Unused Code

**Files:**
- Modify: `main/main.c`
- Delete: `main/test_shortcut_parser.c`

- [ ] **Step 1: Remove invocation from main.c**
Remove `extern void test_shortcut_parser();` and the call to `test_shortcut_parser();` from `app_main()` in `main/main.c`.

- [ ] **Step 2: Delete test_shortcut_parser.c**
Run: `rm main/test_shortcut_parser.c`

- [ ] **Step 3: Verify build**
Run: `idf.py build`
Expected: Compile succeeds.

- [ ] **Step 4: Commit**
```bash
git rm main/test_shortcut_parser.c
git add main/main.c
git commit -m "refactor: remove unused test_shortcut_parser ad-hoc code"
```

---

### Task 2: Standardize Documentation

**Files:**
- Modify: `main/button_service.c`, `main/buzzer_service.c`, `main/config_mgr.c`, `main/network_service.c`, `main/oled_service.c`, `main/rgb_service.c`, `main/rtc_service.c`, `main/usb_hid_service.c`
- Modify: All corresponding `.h` files.

- [ ] **Step 1: Add `@file` headers to source files**
For each `.c` file listed above, add a Doxygen `@file` header explaining its purpose.
Example:
```c
/**
 * @file button_service.c
 * @brief Handles GPIO button input with debounce and multi-click detection.
 */
```

- [ ] **Step 2: Add `@brief` blocks to header files**
For each public function in the `.h` files, add a `@brief` description.
Example:
```c
/**
 * @brief Initializes the button service task.
 */
void button_service_init(void);
```

- [ ] **Step 3: Verify build**
Run: `idf.py build`
Expected: Compile succeeds.

- [ ] **Step 4: Commit**
```bash
git add main/*.c main/*.h
git commit -m "docs: standardize Doxygen headers across services"
```

---

### Task 3: Configuration Manager Consolidation

**Files:**
- Modify: `main/config_mgr.c`

- [ ] **Step 1: Add default config helper**
Add a static helper to `config_mgr.c`:
```c
static void set_default_config(focuslock_config_t *cfg) {
    cfg->work_time_min = 45;
    cfg->rest_time_min = 5;
    cfg->warning_time_sec = 30;
    strcpy(cfg->lock_shortcut, "Win+L");
    cfg->repeat_lock = true;
    cfg->repeat_interval_sec = 10;
    cfg->buzzer_enabled = true;
    cfg->led_brightness = 100;
}
```

- [ ] **Step 2: Replace duplicated defaults**
In `config_mgr_load()`, replace the two blocks where defaults are hardcoded with calls to `set_default_config(cfg);`.

- [ ] **Step 3: Verify build**
Run: `idf.py build`
Expected: Compile succeeds.

- [ ] **Step 4: Commit**
```bash
git add main/config_mgr.c
git commit -m "refactor: consolidate config defaults in config_mgr"
```

---

### Task 4: Event-Driven Architecture (Broadcaster)

**Files:**
- Modify: `main/pomodoro_engine.h`
- Modify: `main/pomodoro_engine.c`

- [ ] **Step 1: Define Event Base**
In `pomodoro_engine.h`, define the event base and event IDs:
```c
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(POMODORO_EVENTS);

typedef enum {
    POMODORO_EVENT_STATE_UPDATE
} pomodoro_event_id_t;
```

- [ ] **Step 2: Implement Event Base**
In `pomodoro_engine.c`, declare the base at the top:
```c
ESP_EVENT_DEFINE_BASE(POMODORO_EVENTS);
```

- [ ] **Step 3: Post Events**
In `pomodoro_engine_init` or where state transitions occur (e.g., `transition_to` or the tick loop where state changes), use `esp_event_post()` to broadcast the state:
```c
// Inside pomodoro_engine.c after updating state:
esp_event_post(POMODORO_EVENTS, POMODORO_EVENT_STATE_UPDATE, &current_status, sizeof(engine_status_t), portMAX_DELAY);
```

- [ ] **Step 4: Verify build**
Run: `idf.py build`
Expected: Compile succeeds (warnings for unused queue are expected for now).

- [ ] **Step 5: Commit**
```bash
git add main/pomodoro_engine.h main/pomodoro_engine.c
git commit -m "feat: introduce esp_event base for pomodoro state broadcasts"
```

---

### Task 5: Refactor Subscribers

**Files:**
- Modify: `main/network_service.c`, `main/buzzer_service.c`, `main/rgb_service.c`, `main/usb_hid_service.c`, `main/oled_service.c`, `main/main.c`

- [ ] **Step 1: Update network_service.c**
Remove the task. Register an event handler instead:
```c
static void state_update_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    engine_status_t *status = (engine_status_t *)event_data;
    // same logic as old task
}
void network_service_init(QueueHandle_t q) { // Remove q parameter in .h and .c
    esp_event_handler_register(POMODORO_EVENTS, POMODORO_EVENT_STATE_UPDATE, state_update_handler, NULL);
}
```

- [ ] **Step 2: Update task-based services**
For `buzzer_service.c`, `rgb_service.c`, `usb_hid_service.c`: change their task parameter from a queue to wait on a task-specific queue, or better, register an event handler that writes to their specific queue, or simply store a volatile global state that their internal loop uses instead of `xQueuePeek`.
Using `esp_event_handler_register` to update a global state struct protected by a mutex or just atomic access is simpler. 
For `main.c`, change the polling loop to an event handler that prints the status.

- [ ] **Step 3: Clean up status_queue**
Remove `status_queue` definitions and peeking logic globally.

- [ ] **Step 4: Verify build**
Run: `idf.py build`
Expected: Compile succeeds.

- [ ] **Step 5: Commit**
```bash
git add main/*.c main/*.h
git commit -m "refactor: migrate all services from queue polling to esp_event subscriptions"
```