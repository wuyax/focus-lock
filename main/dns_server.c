#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "dns_server.h"

static const char *TAG = "dns_server";

#define DNS_PORT 53
#define DNS_MAX_PACKET_SIZE 512

typedef struct __attribute__((packed))
{
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} dns_header_t;

typedef struct __attribute__((packed))
{
    uint16_t type;
    uint16_t class;
} dns_question_t;

typedef struct __attribute__((packed))
{
    uint16_t name_ptr;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t data_len;
    uint32_t addr;
} dns_answer_t;

static TaskHandle_t dns_task_handle = NULL;
static int dns_sock = -1;

static void dns_task(void *pvParameters)
{
    uint8_t rx_buffer[DNS_MAX_PACKET_SIZE];
    uint8_t tx_buffer[DNS_MAX_PACKET_SIZE];
    struct sockaddr_in server_addr;

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);

    dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (dns_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    if (bind(dns_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(dns_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DNS server started on port %d", DNS_PORT);

    while (1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(dns_sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

        if (len < 0) {
            if (errno == EBADF) {
                ESP_LOGI(TAG, "Socket closed, stopping DNS task");
                break;
            }
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        }

        if (len < sizeof(dns_header_t)) {
            continue;
        }

        dns_header_t *header = (dns_header_t *)rx_buffer;
        
        // Only handle queries
        if ((ntohs(header->flags) & 0x8000) != 0) {
            continue;
        }

        // Prepare response
        dns_header_t *res_header = (dns_header_t *)tx_buffer;
        res_header->id = header->id;
        res_header->flags = htons(0x8180); // Response, No Error
        res_header->qd_count = header->qd_count;
        res_header->an_count = header->qd_count;
        res_header->ns_count = 0;
        res_header->ar_count = 0;

        int offset = sizeof(dns_header_t);
        memcpy(tx_buffer + offset, rx_buffer + offset, len - offset);
        offset += (len - offset);

        for (int i = 0; i < ntohs(header->qd_count); i++) {
            dns_answer_t *answer = (dns_answer_t *)(tx_buffer + offset);
            answer->name_ptr = htons(0xC00C); // Pointer to question name
            answer->type = htons(1);          // A record
            answer->class = htons(1);         // IN class
            answer->ttl = htonl(60);
            answer->data_len = htons(4);
            answer->addr = inet_addr("192.168.4.1");
            offset += sizeof(dns_answer_t);
        }

        sendto(dns_sock, tx_buffer, offset, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
    }

    if (dns_sock != -1) {
        close(dns_sock);
        dns_sock = -1;
    }
    dns_task_handle = NULL;
    vTaskDelete(NULL);
}

void dns_server_start(void)
{
    if (dns_task_handle == NULL) {
        xTaskCreate(dns_task, "dns_task", 4096, NULL, 5, &dns_task_handle);
    }
}

void dns_server_stop(void)
{
    if (dns_sock != -1) {
        // This will cause recvfrom to fail with EBADF
        close(dns_sock);
        dns_sock = -1;
    }
}
