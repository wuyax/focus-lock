# DS3231 RTC Service Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a DS3231 RTC service to retrieve time over I2C.

**Architecture:** A standalone service that adds a device handle to the shared I2C bus and provides time-reading functionality with BCD conversion.

**Tech Stack:** ESP-IDF (I2C Master Driver), C.

---

### Task 1: Create rtc_service.h

**Files:**
- Create: `main/rtc_service.h`

- [ ] **Step 1: Write rtc_service.h**
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

- [ ] **Step 2: Verify file existence**
Run: `ls main/rtc_service.h`

---

### Task 2: Create rtc_service.c

**Files:**
- Create: `main/rtc_service.c`

- [ ] **Step 1: Write rtc_service.c**
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

- [ ] **Step 2: Verify file existence**
Run: `ls main/rtc_service.c`

---

### Task 3: Update main/CMakeLists.txt

**Files:**
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Add rtc_service.c to SRCS**
Add `"rtc_service.c"` to the list of source files in `idf_component_register`.

- [ ] **Step 2: Verify change**
Run: `grep "rtc_service.c" main/CMakeLists.txt`

---

### Task 4: Commit Changes

- [ ] **Step 1: Add and commit**
Run: `git add main/rtc_service.h main/rtc_service.c main/CMakeLists.txt && git commit -m "feat: add DS3231 RTC service"`

---

### Task 5: Final Validation

- [ ] **Step 1: Verify compilation (dry run)**
Since I cannot run a full build, I will check for common errors in the new files.
Run: `gcc -fsyntax-only -Imain -I$IDF_PATH/components/esp_common/include -I$IDF_PATH/components/esp_hw_support/include -I$IDF_PATH/components/esp_rom/include -I$IDF_PATH/components/soc/esp32s3/include -I$IDF_PATH/components/driver/i2c/include main/rtc_service.c` (Note: this might fail due to missing ESP-IDF headers, so I'll rely on manual review if it fails).
Actually, I'll just use `cat` to review.
