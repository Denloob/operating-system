#pragma once

#define VGA_GRAPHICS_ADDRESS (volatile uint8_t *)0xa0000
#define VGA_GRAPHICS_WIDTH 320
#define VGA_GRAPHICS_HEIGHT 200
#define VGA_TEXT_ADDRESS (volatile uint8_t *)0xb8000
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_WIDTH_BYTES (VGA_TEXT_WIDTH * 2)
#define VGA_TEXT_HEIGHT 25

void vga_mode_graphics();
void vga_mode_text();
