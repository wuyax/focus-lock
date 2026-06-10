#include "pomodoro_engine.h"
#include "config_mgr.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "engine";
extern focuslock_config_t global_config;
extern focuslock_stats_t global_stats;

QueueHandle_t status_queue;
static QueueHandle_t event_queue;
static engine_status_t current_status;
static focus_state_t pre_admin_state = STATE_WORK;
static uint32_t work_sec_counter = 0;
static uint32_t rest_sec_counter = 0;

static void update_status_and_notify(void) {
    if (status_queue) {
        xQueueOverwrite(status_queue, &current_status);
    }
}

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
            current_status.total_sec = 5 * 60; 
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

static void handle_tick(void) {
    if (current_status.state == STATE_ADMIN) return;
    
    if (current_status.state == STATE_WORK) {
        work_sec_counter++;
        if (work_sec_counter >= 60) {
            global_stats.total_work_min++;
            work_sec_counter = 0;
            config_mgr_save_stats(&global_stats);
        }
    } else if (current_status.state == STATE_REST) {
        rest_sec_counter++;
        if (rest_sec_counter >= 60) {
            global_stats.total_rest_min++;
            rest_sec_counter = 0;
            config_mgr_save_stats(&global_stats);
        }
    }

    if (current_status.remaining_sec > 0) {
        current_status.remaining_sec--;
        update_status_and_notify();
        
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
                ESP_LOGI(TAG, "Pause timeout, resetting");
                transition_to(STATE_WORK);
            }
        }
    }
}

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
                    if (current_status.state == STATE_WORK) transition_to(STATE_PAUSE);
                    else if (current_status.state == STATE_PAUSE) transition_to(STATE_WORK);
                    break;
                case EVT_DOUBLE_CLICK:
                    if (current_status.state == STATE_WORK || current_status.state == STATE_WARNING) {
                        transition_to(STATE_REST);
                    } else if (current_status.state == STATE_REST) {
                        transition_to(STATE_WORK);
                    }
                    break;
                case EVT_LONG_PRESS:
                    if (current_status.state != STATE_ADMIN) {
                        pre_admin_state = current_status.state;
                        transition_to(STATE_ADMIN);
                    } else {
                        transition_to(pre_admin_state);
                    }
                    break;
            }
        }
    }
}

static void tick_timer_cb(void* arg) {
    engine_event_t evt = EVT_TICK;
    xQueueSendFromISR(event_queue, &evt, NULL);
}

void pomodoro_engine_send_event(engine_event_t evt) {
    xQueueSend(event_queue, &evt, 0);
}

void pomodoro_engine_init(void) {
    status_queue = xQueueCreate(1, sizeof(engine_status_t));
    event_queue = xQueueCreate(10, sizeof(engine_event_t));
    
    const esp_timer_create_args_t tick_args = {
        .callback = &tick_timer_cb,
        .name = "engine_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000000));
    
    xTaskCreate(engine_task, "engine_task", 4096, NULL, 5, NULL);
}