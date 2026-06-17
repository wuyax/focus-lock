# SPIFFS Infrastructure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Initialize SPIFFS and integrate cJSON for future JSON-based configuration storage.

**Architecture:** Create a dedicated `spiffs_manager` to handle mounting and unmounting of the SPIFFS partition. Integrate this into the existing `config_mgr` initialization flow. Update the build system to automatically bundle a `data/` directory into the flash image.

**Tech Stack:** ESP-IDF (SPIFFS component), cJSON.

---

### Task 1: Setup Data Directory

**Files:**
- Create: `data/schedule.json`

- [ ] **Step 1: Create the data directory and a placeholder JSON file**

```bash
mkdir -p data
```

- [ ] **Step 2: Add placeholder content to `data/schedule.json`**

```json
{
  "version": "1.0",
  "schedules": []
}
```

---

### Task 2: Implement SPIFFS Manager

**Files:**
- Create: `main/spiffs_manager.h`
- Create: `main/spiffs_manager.c`

- [ ] **Step 1: Create `main/spiffs_manager.h`**

```c
#pragma once
#include "esp_err.h"

/**
 * @brief Initializes and mounts the SPIFFS partition.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t spiffs_manager_init(void);

/**
 * @brief Unmounts the SPIFFS partition.
 * @return ESP_OK on success.
 */
esp_err_t spiffs_manager_deinit(void);
```

- [ ] **Step 2: Create `main/spiffs_manager.c`**

```c
#include "spiffs_manager.h"
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "spiffs_mgr";

esp_err_t spiffs_manager_init(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

esp_err_t spiffs_manager_deinit(void) {
    return esp_vfs_spiffs_unregister(NULL);
}
```

---

### Task 3: Integrate into Config Manager

**Files:**
- Modify: `main/config_mgr.c`

- [ ] **Step 1: Add include for `spiffs_manager.h` and update `config_mgr_init`**

```c
// ... existing includes ...
#include "spiffs_manager.h"

// ... existing code ...

esp_err_t config_mgr_init(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    // Initialize SPIFFS
    return spiffs_manager_init();
}
```

---

### Task 4: Update Build System

**Files:**
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Update `main/CMakeLists.txt` to include new files and register SPIFFS assets**

```cmake
idf_component_register(SRCS "buzzer_service.c" "rtc_service.c" "main.c" "config_mgr.c" 
                    "pomodoro_engine.c" "rgb_service.c" "button_service.c" 
                    "usb_hid_service.c" "dns_server.c" "shortcut_parser.c" 
                    "oled_service.c" "i2c_manager.c" "network_service.c" 
                    "web_server.c" "spiffs_manager.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES led_strip nvs_flash esp_timer esp_driver_gpio 
                    esp_tinyusb driver esp_driver_i2c nixy4__u8g2 esp_wifi 
                    esp_http_server cjson esp_spiffs)

target_add_binary_data(${COMPONENT_LIB} "web_ui.html" BINARY)

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
```

---

### Task 5: Verification (TDD)

- [ ] **Step 1: Add a test log in `main/main.c` or use the existing log in `spiffs_manager_init` to verify mount success during startup.**
- [ ] **Step 2: Run build to ensure `spiffs_create_partition_image` works and partition is created.**
