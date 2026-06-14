/**
 * @file usb_hid_service.h
 * @brief USB HID service for keyboard emulation.
 */

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Initialize the USB HID service.
 */
void usb_hid_service_init(void);
