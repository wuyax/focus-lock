/**
 * @file network_service.h
 * @brief Network service for FocusLock.
 */

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initialize the network service.
 * 
 * @param q Queue handle for receiving engine status updates.
 */
void network_service_init(QueueHandle_t q);
