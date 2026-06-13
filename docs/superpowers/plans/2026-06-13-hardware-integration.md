# Hardware Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate DS3231 RTC, SSD1306 OLED, Buzzer, Button, and WS2812B into the FocusLock ESP32-S3 project.

**Architecture:** Centralized I2C management to share the bus between OLED and RTC. Dedicated FreeRTOS services for RTC and Buzzer, with UI updates to display real-time clock.

**Tech Stack:** ESP-IDF v5.x, FreeRTOS, u8g2, led_strip, I2C Master Driver.

---

### Task 1: Centralized I2C Manager

**Files:**
- Create: `main/i2c_manager.h`
- Create: `main/i2c_manager.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Create i2c_manager.h**

```c
#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_SDA_PIN 1
#define I2C_SCL_PIN 2

extern i2c_master_bus_handle_t i2c_bus_handle;

esp_err_t i2c_manager_init(void);
```

- [ ] **Step 2: Create i2c_manager.c**

```c
#include "i2c_manager.h"
#include "esp_log.h"

static const char *TAG = "i2c_mgr";
i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t i2c_manager_init(void) {
    if (i2c_bus_handle != NULL) return ESP_OK;

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
    }
    return ret;
}
```

- [ ] **Step 3: Update main/CMakeLists.txt to include new files**

```cmake
# Add i2c_manager.c to SRCS
set(SRCS 
    "main.c" 
    "i2c_manager.c"
    # ... rest of files
)
```

- [ ] **Step 4: Commit**

```bash
git add main/i2c_manager.h main/i2c_manager.c main/CMakeLists.txt
git commit -m "feat: add centralized I2C manager"
```

---

### Task 2: DS3231 RTC Service

**Files:**
- Create: `main/rtc_service.h`
- Create: `main/rtc_service.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Create rtc_service.h**

```c
#pragma once
#include <time.h>
#include "esp_err.h"

typedef struct {
    int hour;
    int minute;
    int second;
} rtc_time_t;

esp_err_t rtc_service_init(void);
esp_err_t rtc_get_time(rtc_time_t *time);
```

- [ ] **Step 2: Create rtc_service.c**

```c
#include "rtc_service.h"
#include "i2c_manager.h"
#include "esp_log.h"

static const char *TAG = "rtc";
static i2c_master_dev_handle_t rtc_dev_handle;

esp_err_t rtc_service_init(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_7bit = 0x68,
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
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
# Add rtc_service.c to SRCS
```

- [ ] **Step 4: Commit**

```bash
git add main/rtc_service.h main/rtc_service.c main/CMakeLists.txt
git commit -m "feat: add DS3231 RTC service"
```

---

### Task 3: Buzzer Service

**Files:**
- Create: `main/buzzer_service.h`
- Create: `main/buzzer_service.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Create buzzer_service.h**

```c
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void buzzer_service_init(QueueHandle_t status_queue);
```

- [ ] **Step 2: Create buzzer_service.c**

```c
#include "buzzer_service.h"
#include "pomodoro_engine.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#define BUZZER_GPIO 3

static void beep(uint32_t duration_ms) {
    gpio_set_level(BUZZER_GPIO, 0); // Active Low
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    gpio_set_level(BUZZER_GPIO, 1);
}

static void buzzer_task(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;
    engine_status_t status;
    focus_state_t last_state = STATE_WORK;

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 1);

    while(1) {
        if (xQueuePeek(q, &status, 0)) {
            if (status.state != last_state) {
                if (status.state == STATE_WARNING) {
                    for(int i=0; i<3; i++) { beep(100); vTaskDelay(pdMS_TO_TICKS(900)); }
                } else if (status.state == STATE_REST) {
                    for(int i=0; i<5; i++) { beep(50); vTaskDelay(pdMS_TO_TICKS(50)); }
                } else if (last_state == STATE_REST && status.state == STATE_WORK) {
                    beep(500);
                }
                last_state = status.state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void buzzer_service_init(QueueHandle_t status_queue) {
    xTaskCreate(buzzer_task, "buzzer_task", 2048, status_queue, 5, NULL);
}
```

- [ ] **Step 3: Update main/CMakeLists.txt**

- [ ] **Step 4: Commit**

```bash
git add main/buzzer_service.h main/buzzer_service.c main/CMakeLists.txt
git commit -m "feat: add active buzzer service"
```

---

### Task 4: UI Updates (OLED RTC Display)

**Files:**
- Modify: `main/oled_service.c`

- [ ] **Step 1: Use centralized I2C in oled_service_init**

```c
// Replace u8g2_esp32_i2c_set_default_context logic
void oled_service_init(QueueHandle_t q) {
    u8g2_esp32_i2c_ctx_t i2c_ctx = {
        .cfg = U8G2_ESP32_I2C_CONFIG_DEFAULT(),
        .bus_handle = i2c_bus_handle, // Use the global handle
    };
    u8g2_esp32_i2c_set_default_context(&i2c_ctx);
    // ... setup u8g2 ...
}
```

- [ ] **Step 2: Update oled_task to display RTC time**

```c
// In oled_task loop
rtc_time_t now;
if (rtc_get_time(&now) == ESP_OK) {
    char rtc_str[16];
    snprintf(rtc_str, sizeof(rtc_str), "%02d:%02d", now.hour, now.minute);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 95, 10, rtc_str);
}
```

- [ ] **Step 3: Commit**

```bash
git add main/oled_service.c
git commit -m "feat: display real-time clock on OLED"
```

---

### Task 5: Hardware Alignment (GPIOs)

**Files:**
- Modify: `main/rgb_service.c`
- Modify: `main/button_service.c`

- [ ] **Step 1: Update RGB LED GPIO to 8**

```c
#define WS2812_GPIO 8
```

- [ ] **Step 2: Update Button GPIO to 0 (if different)**

```c
#define BUTTON_GPIO 0
```

- [ ] **Step 3: Commit**

```bash
git add main/rgb_service.c main/button_service.c
git commit -m "fix: update GPIO pin assignments for ESP32-S3"
```

---

### Task 6: Main Integration

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: Update app_main to initialize new services**

```c
#include "i2c_manager.h"
#include "rtc_service.h"
#include "buzzer_service.h"

// In app_main:
i2c_manager_init();
rtc_service_init();
buzzer_service_init(status_queue);
```

- [ ] **Step 2: Final Verification**
Compile and check for errors.

- [ ] **Step 3: Commit**

```bash
git add main/main.c
git commit -m "feat: integrate all new hardware services in main"
```
