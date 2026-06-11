#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void oled_service_init(QueueHandle_t q);
