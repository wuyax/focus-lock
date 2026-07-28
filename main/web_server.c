/**
 * @file web_server.c
 * @brief Implementation of the web server for configuration and status.
 */

#include <string.h>
#include <stdlib.h>
#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "config_mgr.h"
#include "rtc_service.h"
#include "scheduler_service.h"

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

extern focuslock_config_t global_config;
extern focuslock_stats_t global_stats;

extern const uint8_t web_ui_start[] asm("_binary_web_ui_html_start");
extern const uint8_t web_ui_end[]   asm("_binary_web_ui_html_end");

static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)web_ui_start, web_ui_end - web_ui_start);
    return ESP_OK;
}

static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t config_get_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "work_time_min", global_config.work_time_min);
    cJSON_AddNumberToObject(root, "rest_time_min", global_config.rest_time_min);
    cJSON_AddNumberToObject(root, "warning_time_sec", global_config.warning_time_sec);
    cJSON_AddStringToObject(root, "lock_shortcut", global_config.lock_shortcut);
    cJSON_AddBoolToObject(root, "repeat_lock", global_config.repeat_lock);
    cJSON_AddNumberToObject(root, "repeat_interval_sec", global_config.repeat_interval_sec);
    cJSON_AddBoolToObject(root, "buzzer_enabled", global_config.buzzer_enabled);
    cJSON_AddNumberToObject(root, "led_brightness", global_config.led_brightness);

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req) {
    char buf[512];
    int ret, remaining = req->content_len;
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return ESP_FAIL;
    }
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) return ESP_FAIL;

    cJSON *item;
    if ((item = cJSON_GetObjectItem(root, "work_time_min"))) global_config.work_time_min = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "rest_time_min"))) global_config.rest_time_min = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "warning_time_sec"))) global_config.warning_time_sec = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "lock_shortcut"))) strncpy(global_config.lock_shortcut, item->valuestring, sizeof(global_config.lock_shortcut)-1);
    if ((item = cJSON_GetObjectItem(root, "repeat_lock"))) global_config.repeat_lock = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "repeat_interval_sec"))) global_config.repeat_interval_sec = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "buzzer_enabled"))) global_config.buzzer_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "led_brightness"))) global_config.led_brightness = item->valueint;

    cJSON_Delete(root);
    config_mgr_save(&global_config);
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

static esp_err_t stats_get_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "total_pomodoros", global_stats.total_pomodoros);
    cJSON_AddNumberToObject(root, "total_work_min", global_stats.total_work_min);
    cJSON_AddNumberToObject(root, "total_rest_min", global_stats.total_rest_min);
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}

static esp_err_t time_sync_handler(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    rtc_time_t time;
    cJSON *hour_item = cJSON_GetObjectItem(root, "hour");
    cJSON *min_item = cJSON_GetObjectItem(root, "minute");
    cJSON *sec_item = cJSON_GetObjectItem(root, "second");

    if (!cJSON_IsNumber(hour_item) || !cJSON_IsNumber(min_item) || !cJSON_IsNumber(sec_item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid time parameters");
        return ESP_FAIL;
    }
    time.hour = hour_item->valueint;
    time.minute = min_item->valueint;
    time.second = sec_item->valueint;
    
    cJSON *weekday_item = cJSON_GetObjectItem(root, "weekday");
    if (weekday_item && cJSON_IsNumber(weekday_item)) {
        time.weekday = weekday_item->valueint;
    } else {
        time.weekday = 0; // Default to Monday if not provided
    }

    rtc_set_time(&time);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "RTC Time Synced: %02d:%02d:%02d weekday=%d", 
             time.hour, time.minute, time.second, time.weekday);
    
    // Also notify scheduler in case weekday changed
    scheduler_service_reload();

    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

static esp_err_t schedule_get_handler(httpd_req_t *req) {
    full_schedule_t schedule;
    config_mgr_load_schedule(&schedule);

    cJSON *root = cJSON_CreateObject();
    
    // Modes
    cJSON *modes = cJSON_CreateArray();
    for (int i = 0; i < schedule.mode_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", schedule.modes[i].id);
        cJSON_AddStringToObject(item, "name", schedule.modes[i].name);
        cJSON_AddNumberToObject(item, "work_min", schedule.modes[i].work_min);
        cJSON_AddNumberToObject(item, "rest_min", schedule.modes[i].rest_min);
        cJSON_AddNumberToObject(item, "warn_sec", schedule.modes[i].warn_sec);
        cJSON_AddItemToArray(modes, item);
    }
    cJSON_AddItemToObject(root, "modes", modes);

    // Weekly
    cJSON *weekly = cJSON_CreateArray();
    for (int i = 0; i < 7; i++) {
        cJSON *day_arr = cJSON_CreateArray();
        for (int j = 0; j < schedule.weekly[i].count; j++) {
            cJSON *block = cJSON_CreateObject();
            cJSON_AddStringToObject(block, "start", schedule.weekly[i].blocks[j].start);
            cJSON_AddStringToObject(block, "end", schedule.weekly[i].blocks[j].end);
            cJSON_AddNumberToObject(block, "mode_id", schedule.weekly[i].blocks[j].mode_id);
            cJSON_AddItemToArray(day_arr, block);
        }
        cJSON_AddItemToArray(weekly, day_arr);
    }
    cJSON_AddItemToObject(root, "weekly", weekly);

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}

static esp_err_t schedule_post_handler(httpd_req_t *req) {
    if (req->content_len > 4096) {
        ESP_LOGE(TAG, "Content length too large: %d", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large (Max 4KB)");
        return ESP_FAIL;
    }
    
    // 如果为 0，也是非法的配置提交
    if (req->content_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty payload");
        return ESP_FAIL;
    }

    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        free(buf);
        return ESP_FAIL;
    }

    full_schedule_t schedule;
    memset(&schedule, 0, sizeof(full_schedule_t));

    // Parse modes
    cJSON *modes = cJSON_GetObjectItem(root, "modes");
    if (cJSON_IsArray(modes)) {
        schedule.mode_count = cJSON_GetArraySize(modes);
        if (schedule.mode_count > 4) schedule.mode_count = 4;
        for (int i = 0; i < schedule.mode_count; i++) {
            cJSON *item = cJSON_GetArrayItem(modes, i);
            cJSON *id_item = cJSON_GetObjectItem(item, "id");
            cJSON *name_item = cJSON_GetObjectItem(item, "name");
            cJSON *work_item = cJSON_GetObjectItem(item, "work_min");
            cJSON *rest_item = cJSON_GetObjectItem(item, "rest_min");
            cJSON *warn_item = cJSON_GetObjectItem(item, "warn_sec");

            if (!cJSON_IsNumber(id_item) || !cJSON_IsString(name_item) || 
                !cJSON_IsNumber(work_item) || !cJSON_IsNumber(rest_item) || !cJSON_IsNumber(warn_item)) {
                free(buf);
                cJSON_Delete(root);
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid mode settings");
                return ESP_FAIL;
            }
            schedule.modes[i].id = id_item->valueint;
            strncpy(schedule.modes[i].name, name_item->valuestring, 31);
            schedule.modes[i].name[31] = '\0';
            schedule.modes[i].work_min = work_item->valueint;
            schedule.modes[i].rest_min = rest_item->valueint;
            schedule.modes[i].warn_sec = warn_item->valueint;
        }
    } else {
        free(buf);
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid modes array");
        return ESP_FAIL;
    }

    // Parse weekly
    cJSON *weekly = cJSON_GetObjectItem(root, "weekly");
    if (cJSON_IsArray(weekly)) {
        int days = cJSON_GetArraySize(weekly);
        if (days > 7) days = 7;
        for (int i = 0; i < days; i++) {
            cJSON *day_arr = cJSON_GetArrayItem(weekly, i);
            if (cJSON_IsArray(day_arr)) {
                schedule.weekly[i].count = cJSON_GetArraySize(day_arr);
                if (schedule.weekly[i].count > 8) schedule.weekly[i].count = 8;
                for (int j = 0; j < schedule.weekly[i].count; j++) {
                    cJSON *block = cJSON_GetArrayItem(day_arr, j);
                    cJSON *start_item = cJSON_GetObjectItem(block, "start");
                    cJSON *end_item = cJSON_GetObjectItem(block, "end");
                    cJSON *mode_id_item = cJSON_GetObjectItem(block, "mode_id");

                    if (!cJSON_IsString(start_item) || !cJSON_IsString(end_item) || !cJSON_IsNumber(mode_id_item)) {
                        free(buf);
                        cJSON_Delete(root);
                        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid schedule blocks");
                        return ESP_FAIL;
                    }
                    strncpy(schedule.weekly[i].blocks[j].start, start_item->valuestring, 5);
                    schedule.weekly[i].blocks[j].start[5] = '\0';
                    strncpy(schedule.weekly[i].blocks[j].end, end_item->valuestring, 5);
                    schedule.weekly[i].blocks[j].end[5] = '\0';
                    schedule.weekly[i].blocks[j].mode_id = mode_id_item->valueint;
                }
            } else {
                free(buf);
                cJSON_Delete(root);
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid daily schedule array");
                return ESP_FAIL;
            }
        }
    } else {
        free(buf);
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid weekly array");
        return ESP_FAIL;
    }

    config_mgr_save_schedule(&schedule);
    
    ESP_LOGI(TAG, "New schedule saved. Notify scheduler to reload.");
    scheduler_service_reload();

    cJSON_Delete(root);
    free(buf);
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

void web_server_start(void) {
    if (server) return;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 12; // Increased to accommodate more handlers
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_handlers[] = {
            {"/", HTTP_GET, index_get_handler, NULL},
            {"/api/config", HTTP_GET, config_get_handler, NULL},
            {"/api/config", HTTP_POST, config_post_handler, NULL},
            {"/api/stats", HTTP_GET, stats_get_handler, NULL},
            {"/api/sync_time", HTTP_POST, time_sync_handler, NULL},
            {"/api/schedule", HTTP_GET, schedule_get_handler, NULL},
            {"/api/schedule", HTTP_POST, schedule_post_handler, NULL}
        };
        for (int i = 0; i < sizeof(uri_handlers)/sizeof(httpd_uri_t); i++) {
            httpd_register_uri_handler(server, &uri_handlers[i]);
        }
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
}

void web_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
