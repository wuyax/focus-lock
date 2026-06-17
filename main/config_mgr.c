/**
 * @file config_mgr.c
 * @brief Configuration and statistics management using NVS flash.
 */

#include "config_mgr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "spiffs_manager.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "config_mgr";
static const char *NVS_NAMESPACE = "focuslock";
static const char *SCHEDULE_PATH = "/spiffs/schedule.json";

/**
 * @brief Sets the default configuration values.
 * @param cfg Pointer to the configuration structure to initialize.
 */
static void set_default_config(focuslock_config_t *cfg) {
    memset(cfg, 0, sizeof(focuslock_config_t));
    cfg->work_time_min = 45;
    cfg->rest_time_min = 5;
    cfg->warning_time_sec = 30;
    strcpy(cfg->lock_shortcut, "Win+L");
    cfg->repeat_lock = true;
    cfg->repeat_interval_sec = 10;
    cfg->buzzer_enabled = true;
    cfg->led_brightness = 100;
}

esp_err_t config_mgr_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    return spiffs_manager_init();
}

esp_err_t config_mgr_load(focuslock_config_t *cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        set_default_config(cfg);
        return ESP_OK;
    }
    
    size_t required_size = sizeof(focuslock_config_t);
    err = nvs_get_blob(handle, "config", cfg, &required_size);
    if (err != ESP_OK) {
        set_default_config(cfg);
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

static esp_err_t read_file_to_string(const char *path, char **out_str) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (buf == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t read_size = fread(buf, 1, size, f);
    buf[read_size] = '\0';
    fclose(f);

    *out_str = buf;
    return ESP_OK;
}

static esp_err_t write_string_to_file(const char *path, const char *str) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return ESP_FAIL;
    }
    fputs(str, f);
    fclose(f);
    return ESP_OK;
}

esp_err_t config_mgr_load_schedule(full_schedule_t *schedule) {
    char *json_str = NULL;
    esp_err_t ret = read_file_to_string(SCHEDULE_PATH, &json_str);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Schedule file not found or empty, providing empty schedule");
        memset(schedule, 0, sizeof(full_schedule_t));
        return ret;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse schedule JSON");
        free(json_str);
        return ESP_FAIL;
    }

    // Parse modes
    cJSON *modes = cJSON_GetObjectItem(root, "modes");
    schedule->mode_count = 0;
    if (cJSON_IsArray(modes)) {
        int m_count = cJSON_GetArraySize(modes);
        if (m_count > 4) m_count = 4;
        for (int i = 0; i < m_count; i++) {
            cJSON *item = cJSON_GetArrayItem(modes, i);
            cJSON *id = cJSON_GetObjectItem(item, "id");
            cJSON *name = cJSON_GetObjectItem(item, "name");
            cJSON *work = cJSON_GetObjectItem(item, "work_min");
            cJSON *rest = cJSON_GetObjectItem(item, "rest_min");
            cJSON *warn = cJSON_GetObjectItem(item, "warn_sec");

            if (id && name && work && rest && warn) {
                schedule->modes[i].id = id->valueint;
                strncpy(schedule->modes[i].name, name->valuestring, 31);
                schedule->modes[i].name[31] = '\0';
                schedule->modes[i].work_min = (uint32_t)work->valueint;
                schedule->modes[i].rest_min = (uint32_t)rest->valueint;
                schedule->modes[i].warn_sec = (uint32_t)warn->valueint;
                schedule->mode_count++;
            }
        }
    }

    // Parse weekly
    cJSON *weekly = cJSON_GetObjectItem(root, "weekly");
    if (cJSON_IsArray(weekly)) {
        int days = cJSON_GetArraySize(weekly);
        if (days > 7) days = 7;
        for (int i = 0; i < days; i++) {
            cJSON *day_arr = cJSON_GetArrayItem(weekly, i);
            schedule->weekly[i].count = 0;
            if (cJSON_IsArray(day_arr)) {
                int blocks = cJSON_GetArraySize(day_arr);
                if (blocks > 8) blocks = 8;
                for (int j = 0; j < blocks; j++) {
                    cJSON *block_item = cJSON_GetArrayItem(day_arr, j);
                    cJSON *start = cJSON_GetObjectItem(block_item, "start");
                    cJSON *end = cJSON_GetObjectItem(block_item, "end");
                    cJSON *mode_id = cJSON_GetObjectItem(block_item, "mode_id");

                    if (start && end && mode_id) {
                        strncpy(schedule->weekly[i].blocks[j].start, start->valuestring, 5);
                        schedule->weekly[i].blocks[j].start[5] = '\0';
                        strncpy(schedule->weekly[i].blocks[j].end, end->valuestring, 5);
                        schedule->weekly[i].blocks[j].end[5] = '\0';
                        schedule->weekly[i].blocks[j].mode_id = mode_id->valueint;
                        schedule->weekly[i].count++;
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    free(json_str);
    return ESP_OK;
}

esp_err_t config_mgr_save_schedule(const full_schedule_t *schedule) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return ESP_ERR_NO_MEM;
    
    // Modes
    cJSON *modes = cJSON_CreateArray();
    if (modes == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    for (int i = 0; i < schedule->mode_count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (item) {
            cJSON_AddNumberToObject(item, "id", schedule->modes[i].id);
            cJSON_AddStringToObject(item, "name", schedule->modes[i].name);
            cJSON_AddNumberToObject(item, "work_min", schedule->modes[i].work_min);
            cJSON_AddNumberToObject(item, "rest_min", schedule->modes[i].rest_min);
            cJSON_AddNumberToObject(item, "warn_sec", schedule->modes[i].warn_sec);
            cJSON_AddItemToArray(modes, item);
        }
    }
    cJSON_AddItemToObject(root, "modes", modes);

    // Weekly
    cJSON *weekly = cJSON_CreateArray();
    if (weekly == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    for (int i = 0; i < 7; i++) {
        cJSON *day_arr = cJSON_CreateArray();
        if (day_arr) {
            for (int j = 0; j < schedule->weekly[i].count; j++) {
                cJSON *block_item = cJSON_CreateObject();
                if (block_item) {
                    cJSON_AddStringToObject(block_item, "start", schedule->weekly[i].blocks[j].start);
                    cJSON_AddStringToObject(block_item, "end", schedule->weekly[i].blocks[j].end);
                    cJSON_AddNumberToObject(block_item, "mode_id", schedule->weekly[i].blocks[j].mode_id);
                    cJSON_AddItemToArray(day_arr, block_item);
                }
            }
            cJSON_AddItemToArray(weekly, day_arr);
        }
    }
    cJSON_AddItemToObject(root, "weekly", weekly);

    char *json_str = cJSON_PrintUnformatted(root);
    esp_err_t ret = ESP_OK;
    if (json_str) {
        ret = write_string_to_file(SCHEDULE_PATH, json_str);
        free(json_str);
    } else {
        ret = ESP_ERR_NO_MEM;
    }
    
    cJSON_Delete(root);
    return ret;
}
