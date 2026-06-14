/**
 * @brief USB HID service for keyboard emulation.
 */

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initialize the USB HID service.
 * @param q Handle to the queue for receiving HID events.
 */
void usb_hid_service_init(QueueHandle_t q);
