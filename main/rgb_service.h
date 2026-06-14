#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initializes the RGB LED service task.
 * @param q Queue to receive engine status updates.
 */
void rgb_service_init(QueueHandle_t q);
