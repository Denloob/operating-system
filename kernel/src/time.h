#pragma once

#include <stdint.h>

/**
 * @brief Halt execution for at least delay_ms milliseconds.
 *
 * @param delay_ms The sleep delay in milliseconds.
 */
void sleep_ms(uint64_t delay_ms);
