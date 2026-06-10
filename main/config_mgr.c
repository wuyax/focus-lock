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
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        memset(stats, 0, sizeof(focuslock_stats_t));
        return ESP_OK;
    }
    size_t required_size = sizeof(focuslock_stats_t);
    err = nvs_get_blob(handle, "stats", stats, &required_size);
    if (err != ESP_OK) {
        memset(stats, 0, sizeof(focuslock_stats_t));
    }
    nvs_close(handle);
    return ESP_OK;
}

esp_err_t config_mgr_save_stats(const focuslock_stats_t *stats) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(handle, "stats", stats, sizeof(focuslock_stats_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}
