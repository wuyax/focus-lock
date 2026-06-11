#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void network_service_init(QueueHandle_t q);
