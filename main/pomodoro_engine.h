/**
 * @file pomodoro_engine.h
 * @brief Pomodoro state machine engine.
 */

#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_event.h"

/**
 * @brief Event base for Pomodoro engine events.
 */
ESP_EVENT_DECLARE_BASE(POMODORO_EVENTS);

/**
 * @brief Event IDs for Pomodoro engine events.
 */
typedef enum {
    POMODORO_EVENT_STATE_UPDATE /**< Broadcasted when engine status changes. */
} pomodoro_event_id_t;

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
    EVT_TICK,           /**< Periodic timer tick. */
    EVT_START_MODE,     /**< Automatically start a work session. */
    EVT_PENDING_EXIT    /**< Mark current session for exit at end of cycle. */
} engine_event_t;

/**
 * @brief Status structure broadcast by the engine.
 */
typedef struct {
    focus_state_t state;     /**< Current focus state. */
    uint32_t remaining_sec;  /**< Remaining time in current phase in seconds. */
    uint32_t total_sec;      /**< Total duration of current phase in seconds. */
    bool pending_exit;       /**< Whether the session is marked for auto-exit. */
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
 * @brief Starts a work session with specific parameters.
 * @param work_min Work duration in minutes.
 * @param rest_min Rest duration in minutes.
 * @param warn_sec Warning duration in seconds.
 */
void pomodoro_engine_start_mode(uint32_t work_min, uint32_t rest_min, uint32_t warn_sec);

/**
 * @brief Notifies the engine that it should exit after the current cycle.
 */
void pomodoro_engine_pending_exit(void);
