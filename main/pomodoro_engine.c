/**
 * @file pomodoro_engine.c
 * @brief Core state machine and timer logic for the Pomodoro focus lock.
 * 
 * Manages the transitions between Work, Warning, Rest, Pause, and Admin states.
 * Handles the 1-second tick timer, updates statistics, and broadcasts state changes.
 */

#include "pomodoro_engine.h"
#include "config_mgr.h"
#include "esp_log.h"
#include "esp_timer.h"

ESP_EVENT_DEFINE_BASE(POMODORO_EVENTS);

static const char *TAG = "engine";

/* External dependencies for configuration and statistics */
extern focuslock_config_t global_config;
extern focuslock_stats_t global_stats;

/* Queues for event processing and status broadcasting */
QueueHandle_t status_queue;
static QueueHandle_t event_queue;

/* Current state of the engine */
static engine_status_t current_status;

/* Encapsulated structure for saving/restoring states during interruptions (Pause/Admin) */
typedef struct {
    focus_state_t state;
    uint32_t remaining_sec;
    uint32_t total_sec;
} saved_state_context_t;

/* Backups to support nested interruptions (e.g. Pause -> Admin -> Pause -> Work) */
static saved_state_context_t admin_backup = { .state = STATE_WORK };
static saved_state_context_t pause_backup = { .state = STATE_WORK };

/* Counters for accumulating minutes for statistical tracking */
static uint32_t work_sec_counter = 0;
static uint32_t rest_sec_counter = 0;

/**
 * @brief Broadcasts the current status to the status_queue (consumed by OLED UI).
 */
static void update_status_and_notify(void) {
    if (status_queue) {
        xQueueOverwrite(status_queue, &current_status);
    }
    esp_event_post(POMODORO_EVENTS, POMODORO_EVENT_STATE_UPDATE, &current_status, sizeof(engine_status_t), portMAX_DELAY);
}

/**
 * @brief Transitions to a new state, loading default countdown times from configuration.
 * @param new_state The target focus_state_t to transition into.
 */
static void transition_to(focus_state_t new_state) {
    current_status.state = new_state;
    switch (new_state) {
        case STATE_WORK:
            current_status.total_sec = global_config.work_time_min * 60;
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_WARNING:
            current_status.total_sec = global_config.warning_time_sec;
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_REST:
            current_status.total_sec = global_config.rest_time_min * 60;
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_PAUSE:
            current_status.total_sec = 5 * 60; // Hardcoded 5 minutes pause limit
            current_status.remaining_sec = current_status.total_sec;
            break;
        case STATE_ADMIN:
            current_status.total_sec = 0;
            current_status.remaining_sec = 0;
            break;
    }
    ESP_LOGI(TAG, "State transition to %d", new_state);
    update_status_and_notify();
}

/**
 * @brief Saves the current engine context to a backup slot.
 * @param backup Pointer to the backup slot.
 */
static void save_context(saved_state_context_t *backup) {
    backup->state = current_status.state;
    backup->remaining_sec = current_status.remaining_sec;
    backup->total_sec = current_status.total_sec;
}

/**
 * @brief Restores the engine state from a backup context without resetting times.
 * @param backup Pointer to the backup slot.
 */
static void restore_context(const saved_state_context_t *backup) {
    current_status.state = backup->state;
    current_status.remaining_sec = backup->remaining_sec;
    current_status.total_sec = backup->total_sec;
    ESP_LOGI(TAG, "Restored state %d, remaining: %lu", backup->state, backup->remaining_sec);
    update_status_and_notify();
}

/**
 * @brief Helper to accumulate seconds into minutes and save statistics to flash.
 * @param sec_counter Pointer to the second counter variable.
 * @param stat_minutes Pointer to the total minutes variable in global_stats.
 */
static void accumulate_minutes(uint32_t *sec_counter, uint32_t *stat_minutes) {
    (*sec_counter)++;
    if (*sec_counter >= 60) {
        (*stat_minutes)++;
        *sec_counter = 0;
        config_mgr_save_stats(&global_stats);
    }
}

/**
 * @brief Handles the 1-second tick event, updating timers and triggering state timeouts.
 */
static void handle_tick(void) {
    // Admin state suspends all timers
    if (current_status.state == STATE_ADMIN) return;
    
    // Accumulate time for stats
    if (current_status.state == STATE_WORK) {
        accumulate_minutes(&work_sec_counter, &global_stats.total_work_min);
    } else if (current_status.state == STATE_REST) {
        accumulate_minutes(&rest_sec_counter, &global_stats.total_rest_min);
    }

    // Decrement countdown
    if (current_status.remaining_sec > 0) {
        current_status.remaining_sec--;
        update_status_and_notify();
        
        // Handle timeout transitions
        if (current_status.remaining_sec == 0) {
            if (current_status.state == STATE_WORK) {
                transition_to(STATE_WARNING);
            } else if (current_status.state == STATE_WARNING) {
                global_stats.total_pomodoros++;
                config_mgr_save_stats(&global_stats);
                transition_to(STATE_REST);
            } else if (current_status.state == STATE_REST) {
                transition_to(STATE_WORK); 
            } else if (current_status.state == STATE_PAUSE) {
                ESP_LOGI(TAG, "Pause timeout, resetting to WORK");
                transition_to(STATE_WORK);
            }
        }
    }
}

/**
 * @brief Main engine task loop, processing events from the queue.
 */
static void engine_task(void *arg) {
    engine_event_t evt;
    transition_to(STATE_WORK);
    
    while(1) {
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY)) {
            switch(evt) {
                case EVT_TICK:
                    handle_tick();
                    break;
                    
                case EVT_CLICK:
                    // Single click toggles Pause when in Work state
                    if (current_status.state == STATE_WORK) {
                        save_context(&pause_backup);
                        transition_to(STATE_PAUSE);
                    } else if (current_status.state == STATE_PAUSE) {
                        restore_context(&pause_backup);
                    }
                    break;
                    
                case EVT_DOUBLE_CLICK:
                    // Double click skips the current cycle
                    if (current_status.state == STATE_WORK || current_status.state == STATE_WARNING) {
                        transition_to(STATE_REST);
                    } else if (current_status.state == STATE_REST) {
                        transition_to(STATE_WORK);
                    }
                    break;
                    
                case EVT_LONG_PRESS:
                    // Long press toggles Admin settings mode
                    if (current_status.state != STATE_ADMIN) {
                        save_context(&admin_backup);
                        transition_to(STATE_ADMIN);
                    } else {
                        restore_context(&admin_backup);
                    }
                    break;
            }
        }
    }
}

/**
 * @brief Hardware timer callback generating 1-second ticks.
 */
static void tick_timer_cb(void* arg) {
    engine_event_t evt = EVT_TICK;
    xQueueSendFromISR(event_queue, &evt, NULL); // Send tick to the queue from ISR context
}

void pomodoro_engine_send_event(engine_event_t evt) {
    xQueueSend(event_queue, &evt, 0);
}

void pomodoro_engine_init(void) {
    // Create queues for status publishing and event receiving
    status_queue = xQueueCreate(1, sizeof(engine_status_t));
    event_queue = xQueueCreate(10, sizeof(engine_event_t));
    
    // Initialize 1-second hardware timer
    const esp_timer_create_args_t tick_args = {
        .callback = &tick_timer_cb,
        .name = "engine_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000000));
    
    // Start engine core task
    xTaskCreate(engine_task, "engine_task", 4096, NULL, 5, NULL);
}
