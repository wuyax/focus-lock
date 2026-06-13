#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void buzzer_service_init(QueueHandle_t status_queue);
