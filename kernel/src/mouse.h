#pragma once

#include <stdint.h>

/**
 * @brief Initializes the PS/2 mouse, assuming the keyboard is already initialized.
 *        Enables the mouse port, resets the device, sets defaults, and enables
 *        IRQ12 for packet reporting.
 */
void mouse_init();

/**
 * @brief Interrupt handler for IRQ12 (PS/2 mouse). Processes 3-byte packets
 *        and updates mouse position and button state using 9-bit signed deltas.
 */
void mouse_isr();

/**
 * @brief Gets the current X position of the mouse.
 * @return The X coordinate (cumulative).
 */
int16_t mouse_get_x();

/**
 * @brief Gets the current Y position of the mouse.
 * @return The Y coordinate (cumulative, inverted for screen).
 */
int16_t mouse_get_y();

/**
 * @brief Gets the current button states of the mouse.
 * @return Button state (bit 0: left, 1: right, 2: middle).
 */
uint8_t mouse_get_buttons();
