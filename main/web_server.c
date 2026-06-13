#include <string.h>
#include <stdlib.h>
#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "config_mgr.h"
#include "rtc_service.h"

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
    if (root) {
        rtc_time_t time;
        time.hour = cJSON_GetObjectItem(root, "hour")->valueint;
        time.minute = cJSON_GetObjectItem(root, "minute")->valueint;
        time.second = cJSON_GetObjectItem(root, "second")->valueint;
        rtc_set_time(&time);
        cJSON_Delete(root);
        ESP_LOGI(TAG, "RTC Time Synced: %02d:%02d:%02d", time.hour, time.minute, time.second);
    }
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

void web_server_start(void) {
    if (server) return;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_handlers[] = {
            {"/", HTTP_GET, index_get_handler, NULL},
            {"/api/config", HTTP_GET, config_get_handler, NULL},
            {"/api/config", HTTP_POST, config_post_handler, NULL},
            {"/api/stats", HTTP_GET, stats_get_handler, NULL},
            {"/api/sync_time", HTTP_POST, time_sync_handler, NULL}
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
