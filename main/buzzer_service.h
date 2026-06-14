/**
 * @file buzzer_service.h
 * @brief Header for buzzer service.
 */

#pragma once
#include <stdint.h>
/**
 * @brief Initializes the buzzer service task.
 */
void buzzer_service_init(void);

/**
 * @brief Produces a single beep.
 * @param duration_ms Duration of the beep in milliseconds.
 */
void buzzer_beep(uint32_t duration_ms);
