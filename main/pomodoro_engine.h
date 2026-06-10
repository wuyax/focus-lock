#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    STATE_WORK,
    STATE_WARNING,
    STATE_REST,
    STATE_PAUSE,
    STATE_ADMIN
} focus_state_t;

typedef enum {
    EVT_CLICK,
    EVT_DOUBLE_CLICK,
    EVT_LONG_PRESS,
    EVT_TICK 
} engine_event_t;

typedef struct {
    focus_state_t state;
    uint32_t remaining_sec;
    uint32_t total_sec;
} engine_status_t;

void pomodoro_engine_init(void);
void pomodoro_engine_send_event(engine_event_t evt);
extern QueueHandle_t status_queue; 
