#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void buzzer_service_init(QueueHandle_t status_queue);
void buzzer_beep(uint32_t duration_ms);
