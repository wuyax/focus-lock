#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void usb_hid_service_init(QueueHandle_t q);
