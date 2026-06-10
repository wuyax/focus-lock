#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void rgb_service_init(QueueHandle_t q);
