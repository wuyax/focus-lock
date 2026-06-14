/**
 * @file oled_service.h
 * @brief OLED display service for FocusLock.
 */

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initialize the OLED service.
 */
void oled_service_init(void);
