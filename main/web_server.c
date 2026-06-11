#include <string.h>
#include <stdlib.h>
#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "config_mgr.h"

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

extern focuslock_config_t global_config;
extern focuslock_stats_t global_stats;

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
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

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

void web_server_start(void) {
    if (server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t config_get = {
            .uri       = "/api/config",
            .method    = HTTP_GET,
            .handler   = config_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &config_get);

        httpd_uri_t config_post = {
            .uri       = "/api/config",
            .method    = HTTP_POST,
            .handler   = config_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &config_post);

        httpd_uri_t stats_get = {
            .uri       = "/api/stats",
            .method    = HTTP_GET,
            .handler   = stats_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &stats_get);
    } else {
        ESP_LOGE(TAG, "Error starting server!");
    }
}

void web_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
