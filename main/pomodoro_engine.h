/**
 * @file pomodoro_engine.h
 * @brief Pomodoro state machine engine.
 */

#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Focus states for the Pomodoro engine.
 */
typedef enum {
    STATE_WORK,      /**< Work phase. */
    STATE_WARNING,   /**< Warning phase before work/rest ends. */
    STATE_REST,      /**< Rest phase. */
    STATE_PAUSE,     /**< Paused state. */
    STATE_ADMIN      /**< Administrative/Configuration state. */
} focus_state_t;

/**
 * @brief Events that can be sent to the Pomodoro engine.
 */
typedef enum {
    EVT_CLICK,          /**< Single button click. */
    EVT_DOUBLE_CLICK,   /**< Double button click. */
    EVT_LONG_PRESS,     /**< Long button press. */
    EVT_TICK            /**< Periodic timer tick. */
} engine_event_t;

/**
 * @brief Status structure broadcast by the engine.
 */
typedef struct {
    focus_state_t state;     /**< Current focus state. */
    uint32_t remaining_sec;  /**< Remaining time in current phase in seconds. */
    uint32_t total_sec;      /**< Total duration of current phase in seconds. */
} engine_status_t;

/**
 * @brief Initializes the Pomodoro engine.
 */
void pomodoro_engine_init(void);

/**
 * @brief Sends an event to the Pomodoro engine.
 * @param evt The event to send.
 */
void pomodoro_engine_send_event(engine_event_t evt);

/**
 * @brief Queue handle for receiving engine status updates.
 */
extern QueueHandle_t status_queue; 
