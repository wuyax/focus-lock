#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initializes the buzzer service task.
 * @param status_queue Queue to receive engine status updates.
 */
void buzzer_service_init(QueueHandle_t status_queue);

/**
 * @brief Produces a single beep.
 * @param duration_ms Duration of the beep in milliseconds.
 */
void buzzer_beep(uint32_t duration_ms);
