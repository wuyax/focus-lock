#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "network_service.h"
#include "pomodoro_engine.h"
#include "dns_server.h"

static const char *TAG = "network_service";
static bool wifi_started = false;
static esp_netif_t *ap_netif = NULL;

static void wifi_init_softap(void)
{
    if (ap_netif == NULL) {
        ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "FocusLock_Config",
            .ssid_len = strlen("FocusLock_Config"),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    dns_server_start();

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:FocusLock_Config");
}

static void wifi_stop_softap(void)
{
    dns_server_stop();
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_LOGI(TAG, "wifi_stop_softap finished.");
}

static void network_task(void *pvParameters)
{
    QueueHandle_t status_queue = (QueueHandle_t)pvParameters;
    engine_status_t status;

    while (1) {
        if (xQueuePeek(status_queue, &status, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (status.state == STATE_ADMIN && !wifi_started) {
                ESP_LOGI(TAG, "Entering ADMIN state, starting Wi-Fi AP");
                wifi_init_softap();
                wifi_started = true;
            } else if (status.state != STATE_ADMIN && wifi_started) {
                ESP_LOGI(TAG, "Leaving ADMIN state, stopping Wi-Fi AP");
                wifi_stop_softap();
                wifi_started = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void network_service_init(QueueHandle_t q)
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    xTaskCreate(network_task, "network_task", 4096, (void *)q, 5, NULL);
}
