# FocusLock Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the core state machine, hardware services, and Web UI configuration for the FocusLock hardware Pomodoro timer based on ESP-IDF and FreeRTOS.

**Architecture:** A FreeRTOS multi-tasking architecture decoupling the Pomodoro engine (state machine), UI service (OLED/RGB), Hardware service (Buzzer/HID), and Network service (Wi-Fi/Web Server).

**Tech Stack:** C, ESP-IDF (FreeRTOS, NVS, esp_timer, driver/gpio, driver/led_strip), HTML/JS/CSS (embedded).

---

### Task 1: Project Skeleton and NVS Initialization

**Files:**
- Create: `main/config_mgr.h`
- Create: `main/config_mgr.c`
- Modify: `main/blink_example_main.c` -> Rename to `main/main.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Rename main file and update CMakeLists.txt**

Run: `mv main/blink_example_main.c main/main.c`

Modify `main/CMakeLists.txt` to replace `blink_example_main.c` with `main.c` and add `config_mgr.c`.

- [ ] **Step 2: Create NVS Config Manager Header**

Create `main/config_mgr.h`:
```c
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    uint32_t work_time_min;
    uint32_t rest_time_min;
    uint32_t warning_time_sec;
    char lock_shortcut[16];
    bool repeat_lock;
    uint32_t repeat_interval_sec;
    bool buzzer_enabled;
    uint8_t led_brightness;
} focuslock_config_t;

typedef struct {
    uint32_t total_pomodoros;
    uint32_t total_work_min;
    uint32_t total_rest_min;
} focuslock_stats_t;

esp_err_t config_mgr_init(void);
esp_err_t config_mgr_load(focuslock_config_t *cfg);
esp_err_t config_mgr_save(const focuslock_config_t *cfg);
esp_err_t config_mgr_load_stats(focuslock_stats_t *stats);
esp_err_t config_mgr_save_stats(const focuslock_stats_t *stats);
```

- [ ] **Step 3: Implement NVS Config Manager**

Create `main/config_mgr.c`:
```c
#include "config_mgr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "config_mgr";
static const char *NVS_NAMESPACE = "focuslock";

esp_err_t config_mgr_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t config_mgr_load(focuslock_config_t *cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        cfg->work_time_min = 45;
        cfg->rest_time_min = 5;
        cfg->warning_time_sec = 30;
        strcpy(cfg->lock_shortcut, "Win+L");
        cfg->repeat_lock = true;
        cfg->repeat_interval_sec = 10;
        cfg->buzzer_enabled = true;
        cfg->led_brightness = 100;
        return ESP_OK;
    }
    
    size_t required_size = sizeof(focuslock_config_t);
    err = nvs_get_blob(handle, "config", cfg, &required_size);
    if (err != ESP_OK) {
        cfg->work_time_min = 45;
        cfg->rest_time_min = 5;
        cfg->warning_time_sec = 30;
        strcpy(cfg->lock_shortcut, "Win+L");
        cfg->repeat_lock = true;
        cfg->repeat_interval_sec = 10;
        cfg->buzzer_enabled = true;
        cfg->led_brightness = 100;
    }
    nvs_close(handle);
    return ESP_OK;
}

esp_err_t config_mgr_save(const focuslock_config_t *cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(handle, "config", cfg, sizeof(focuslock_config_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t config_mgr_load_stats(focuslock_stats_t *stats) {
    memset(stats, 0, sizeof(focuslock_stats_t));
    return ESP_OK; 
}
esp_err_t config_mgr_save_stats(const focuslock_stats_t *stats) { return ESP_OK; }
```

- [ ] **Step 4: Update main.c to initialize NVS**

Replace contents of `main/main.c` with:
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config_mgr.h"

static const char *TAG = "main";
focuslock_config_t global_config;
focuslock_stats_t global_stats;

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FocusLock");
    
    ESP_ERROR_CHECK(config_mgr_init());
    config_mgr_load(&global_config);
    config_mgr_load_stats(&global_stats);
    
    ESP_LOGI(TAG, "Config loaded. Work: %lu min, Rest: %lu min", 
             global_config.work_time_min, global_config.rest_time_min);
             
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

- [ ] **Step 5: Verify Build**

Run: `idf.py build`
Expected: Successful build.

- [ ] **Step 6: Commit**

```bash
git add main/main.c main/config_mgr.c main/config_mgr.h main/CMakeLists.txt
git rm main/blink_example_main.c
git commit -m "feat: setup project skeleton and NVS config manager"
```

---

### Task 2: Core State Machine (Pomodoro Engine)

**Files:**
- Create: `main/pomodoro_engine.h`
- Create: `main/pomodoro_engine.c`
- Modify: `main/main.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Create State definitions and Engine Header**

Create `main/pomodoro_engine.h`:
```c
#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    STATE_WORK,
    STATE_WARNING,
    STATE_REST,
    STATE_PAUSE,
    STATE_ADMIN
} focus_state_t;

typedef enum {
    EVT_CLICK,
    EVT_DOUBLE_CLICK,
    EVT_LONG_PRESS,
    EVT_TICK 
} engine_event_t;

typedef struct {
    focus_state_t state;
    uint32_t remaining_sec;
    uint32_t total_sec;
} engine_status_t;

void pomodoro_engine_init(void);
void pomodoro_engine_send_event(engine_event_t evt);
extern QueueHandle_t status_queue; 
```

- [ ] **Step 2: Implement State Machine Logic**

Create `main/pomodoro_engine.c`:
```c
#include "pomodoro_engine.h"
#include "config_mgr.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "engine";
extern focuslock_config_t global_config;
extern focuslock_stats_t global_stats;

QueueHandle_t status_queue;
static QueueHandle_t event_queue;
static engine_status_t current_status;
static focus_state_t pre_admin_state = STATE_WORK;

static void update_status_and_notify(void) {
    if (status_queue) {
        xQueueOverwrite(status_queue, &current_status);
    }
}

static void transition_to(focus_state_t new_state) {
    current_status.state = new_state;
    switch (new_state) {
        case STATE_WORK:
            current_status.total_sec = global_config.work_time_min * 60;
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_WARNING:
            current_status.total_sec = global_config.warning_time_sec;
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_REST:
            current_status.total_sec = global_config.rest_time_min * 60;
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_PAUSE:
            current_status.total_sec = 5 * 60; 
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_ADMIN:
            current_status.total_sec = 0;
            current_status.remaining_sec = 0;
            break;
    }
    ESP_LOGI(TAG, "State transition to %d", new_state);
    update_status_and_notify();
}

static void handle_tick(void) {
    if (current_status.state == STATE_ADMIN) return;
    
    if (current_status.remaining_sec > 0) {
        current_status.remaining_sec--;
        update_status_and_notify();
        
        if (current_status.remaining_sec == 0) {
            if (current_status.state == STATE_WORK) {
                transition_to(STATE_WARNING);
            } else if (current_status.state == STATE_WARNING) {
                global_stats.total_pomodoros++;
                config_mgr_save_stats(&global_stats);
                transition_to(STATE_REST);
            } else if (current_status.state == STATE_REST) {
                transition_to(STATE_WORK); 
            } else if (current_status.state == STATE_PAUSE) {
                ESP_LOGI(TAG, "Pause timeout, resetting");
                transition_to(STATE_WORK);
            }
        }
    }
}

static void engine_task(void *arg) {
    engine_event_t evt;
    transition_to(STATE_WORK);
    
    while(1) {
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY)) {
            switch(evt) {
                case EVT_TICK:
                    handle_tick();
                    break;
                case EVT_CLICK:
                    if (current_status.state == STATE_WORK) transition_to(STATE_PAUSE);
                    else if (current_status.state == STATE_PAUSE) transition_to(STATE_WORK);
                    break;
                case EVT_DOUBLE_CLICK:
                    if (current_status.state == STATE_WORK || current_status.state == STATE_WARNING) {
                        transition_to(STATE_REST);
                    } else if (current_status.state == STATE_REST) {
                        transition_to(STATE_WORK);
                    }
                    break;
                case EVT_LONG_PRESS:
                    if (current_status.state != STATE_ADMIN) {
                        pre_admin_state = current_status.state;
                        transition_to(STATE_ADMIN);
                    } else {
                        transition_to(pre_admin_state);
                    }
                    break;
            }
        }
    }
}

static void tick_timer_cb(void* arg) {
    engine_event_t evt = EVT_TICK;
    xQueueSendFromISR(event_queue, &evt, NULL);
}

void pomodoro_engine_send_event(engine_event_t evt) {
    xQueueSend(event_queue, &evt, 0);
}

void pomodoro_engine_init(void) {
    status_queue = xQueueCreate(1, sizeof(engine_status_t));
    event_queue = xQueueCreate(10, sizeof(engine_event_t));
    
    const esp_timer_create_args_t tick_args = {
        .callback = &tick_timer_cb,
        .name = "engine_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000000));
    
    xTaskCreate(engine_task, "engine_task", 4096, NULL, 5, NULL);
}
```

- [ ] **Step 3: Update CMakeLists.txt**

Modify `main/CMakeLists.txt` to add `pomodoro_engine.c`.

- [ ] **Step 4: Update main.c to start engine**

Modify `main/main.c`: Add `#include "pomodoro_engine.h"` and initialize it before the while loop.
```c
    pomodoro_engine_init();
```

- [ ] **Step 5: Verify Build**

Run: `idf.py build`

- [ ] **Step 6: Commit**

```bash
git add main/pomodoro_engine.* main/main.c main/CMakeLists.txt
git commit -m "feat: implement pomodoro engine state machine"
```

---

### Task 3: RGB LED Service (UI Layer)

**Files:**
- Create: `main/rgb_service.h`
- Create: `main/rgb_service.c`
- Modify: `main/CMakeLists.txt`
- Modify: `main/main.c`

- [ ] **Step 1: Create Header**

Create `main/rgb_service.h`:
```c
#pragma once
void rgb_service_init(void);
```

- [ ] **Step 2: Implement RGB Task**

Create `main/rgb_service.c`.
```c
#include "rgb_service.h"
#include "pomodoro_engine.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS2812_GPIO CONFIG_BLINK_GPIO 

static led_strip_handle_t led_strip;

static void init_ws2812(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = 1, 
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

static void rgb_task(void *arg) {
    engine_status_t status;
    uint8_t breath_val = 0;
    int8_t breath_dir = 5;
    
    while(1) {
        if (xQueuePeek(status_queue, &status, portMAX_DELAY)) {
            switch(status.state) {
                case STATE_WORK:
                    led_strip_set_pixel(led_strip, 0, 0, 255, 0); 
                    break;
                case STATE_WARNING:
                    led_strip_set_pixel(led_strip, 0, breath_val, breath_val, 0);
                    breath_val += breath_dir;
                    if (breath_val >= 250 || breath_val <= 5) breath_dir = -breath_dir;
                    break;
                case STATE_REST:
                    led_strip_set_pixel(led_strip, 0, 255, 0, 0); 
                    break;
                case STATE_PAUSE:
                case STATE_ADMIN:
                    led_strip_set_pixel(led_strip, 0, 0, 0, 255); 
                    break;
            }
            led_strip_refresh(led_strip);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void rgb_service_init(void) {
    init_ws2812();
    xTaskCreate(rgb_task, "rgb_task", 2048, NULL, 4, NULL);
}
```

- [ ] **Step 3: Update CMakeLists.txt**

Modify `main/CMakeLists.txt` to add `rgb_service.c`.

- [ ] **Step 4: Integrate and Verify Build**

Add `#include "rgb_service.h"` and `rgb_service_init();` to `app_main` in `main/main.c`.
Run `idf.py build`.

- [ ] **Step 5: Commit**

```bash
git add main/rgb_service.* main/main.c main/CMakeLists.txt
git commit -m "feat: implement RGB LED service"
```

---

### Task 4: Button Input Service

**Files:**
- Create: `main/button_service.h`
- Create: `main/button_service.c`
- Modify: `main/CMakeLists.txt`
- Modify: `main/main.c`

- [ ] **Step 1: Create Header**

```c
#pragma once
void button_service_init(void);
```

- [ ] **Step 2: Implement Button Task**

Create `main/button_service.c`.
```c
#include "button_service.h"
#include "pomodoro_engine.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_GPIO 0

static void button_task(void *arg) {
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);
    
    bool last_state = true;
    uint32_t press_time = 0;
    uint32_t release_time = 0;
    int click_count = 0;
    
    while(1) {
        bool current_state = gpio_get_level(BUTTON_GPIO);
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (last_state == true && current_state == false) { 
            press_time = now;
        } else if (last_state == false && current_state == true) { 
            uint32_t duration = now - press_time;
            if (duration > 50) { 
                if (duration > 5000) {
                    pomodoro_engine_send_event(EVT_LONG_PRESS);
                    click_count = 0;
                } else {
                    click_count++;
                    release_time = now;
                }
            }
        } else if (last_state == true && current_state == true) { 
            if (click_count > 0 && (now - release_time > 300)) { 
                if (click_count == 1) {
                    pomodoro_engine_send_event(EVT_CLICK);
                } else if (click_count >= 2) {
                    pomodoro_engine_send_event(EVT_DOUBLE_CLICK);
                }
                click_count = 0;
            }
        }
        
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void button_service_init(void) {
    xTaskCreate(button_task, "btn_task", 2048, NULL, 5, NULL);
}
```

- [ ] **Step 3: Update CMakeLists.txt**

Modify `main/CMakeLists.txt` to add `button_service.c`.

- [ ] **Step 4: Integrate and Verify Build**

Add `#include "button_service.h"` and `button_service_init();` to `app_main` in `main/main.c`.
Run `idf.py build`.

- [ ] **Step 5: Commit**

```bash
git add main/button_service.* main/main.c main/CMakeLists.txt
git commit -m "feat: implement button input service"
```
