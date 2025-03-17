#include "colors.h"

gx_palette_Color g_color_palette[256] = {
    // I - Cyan
    [0] = {0x00, 0xcc, 0xcc}, // Base
    [1] = {0x00, 0xff, 0xff}, // Highlight
    [2] = {0x00, 0x99, 0x99}, // Shadow

    // J - Blue
    [3] = {0x00, 0x00, 0xcc}, // Base
    [4] = {0x00, 0x00, 0xff}, // Highlight
    [5] = {0x00, 0x00, 0x99}, // Shadow

    // L - Orange
    [6] = {0xcc, 0x66, 0x00}, // Base
    [7] = {0xff, 0x88, 0x00}, // Highlight
    [8] = {0x99, 0x44, 0x00}, // Shadow

    // O - Yellow
    [9] = {0xcc, 0xcc, 0x00},  // Base
    [10] = {0xff, 0xff, 0x00}, // Highlight
    [11] = {0x99, 0x99, 0x00}, // Shadow

    // S - Green
    [12] = {0x00, 0xcc, 0x00}, // Base
    [13] = {0x00, 0xff, 0x00}, // Highlight
    [14] = {0x00, 0x99, 0x00}, // Shadow

    // T - Purple
    [15] = {0x99, 0x00, 0xcc}, // Base
    [16] = {0xcc, 0x00, 0xff}, // Highlight
    [17] = {0x66, 0x00, 0x99}, // Shadow

    // Z - Red
    [18] = {0xcc, 0x00, 0x00}, // Base
    [19] = {0xff, 0x00, 0x00}, // Highlight
    [20] = {0x99, 0x00, 0x00}, // Shadow

    [21] = {0xff, 0xff, 0xff}, // White
    [22] = {0x00, 0x00, 0x00}, // Black

    [23] = {0x77, 0x77, 0x77}, // Gray
    [24] = {0x99, 0x99, 0x99}, // Light gray
    [25] = {0x33, 0x33, 0x33}, // Dark gray
};
