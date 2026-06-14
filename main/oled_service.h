/**
 * @file oled_service.h
 * @brief OLED display service for FocusLock.
 */

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initialize the OLED service.
 * 
 * @param q Queue handle for receiving engine status updates.
 */
void oled_service_init(QueueHandle_t q);
