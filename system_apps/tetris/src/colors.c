#include "colors.h"

gx_palette_Color g_color_palette[256] = {
    // I - Cyan
    [COLOR_I_BASE] = {0x00, 0xcc, 0xcc}, // Base
    [COLOR_I_HIGHLIGHT] = {0x00, 0xff, 0xff}, // Highlight
    [COLOR_I_SHADOW] = {0x00, 0x99, 0x99}, // Shadow

    // J - Blue
    [COLOR_J_BASE] = {0x00, 0x00, 0xcc}, // Base
    [COLOR_J_HIGHLIGHT] = {0x00, 0x00, 0xff}, // Highlight
    [COLOR_J_SHADOW] = {0x00, 0x00, 0x99}, // Shadow

    // L - Orange
    [COLOR_L_BASE] = {0xcc, 0x66, 0x00}, // Base
    [COLOR_L_HIGHLIGHT] = {0xff, 0x88, 0x00}, // Highlight
    [COLOR_L_SHADOW] = {0x99, 0x44, 0x00}, // Shadow

    // O - Yellow
    [COLOR_O_BASE] = {0xcc, 0xcc, 0x00},  // Base
    [COLOR_O_HIGHLIGHT] = {0xff, 0xff, 0x00}, // Highlight
    [COLOR_O_SHADOW] = {0x99, 0x99, 0x00}, // Shadow

    // S - Green
    [COLOR_S_BASE] = {0x00, 0xcc, 0x00}, // Base
    [COLOR_S_HIGHLIGHT] = {0x00, 0xff, 0x00}, // Highlight
    [COLOR_S_SHADOW] = {0x00, 0x99, 0x00}, // Shadow

    // T - Purple
    [COLOR_T_BASE] = {0x99, 0x00, 0xcc}, // Base
    [COLOR_T_HIGHLIGHT] = {0xcc, 0x00, 0xff}, // Highlight
    [COLOR_T_SHADOW] = {0x66, 0x00, 0x99}, // Shadow

    // Z - Red
    [COLOR_Z_BASE] = {0xcc, 0x00, 0x00}, // Base
    [COLOR_Z_HIGHLIGHT] = {0xff, 0x00, 0x00}, // Highlight
    [COLOR_Z_SHADOW] = {0x99, 0x00, 0x00}, // Shadow

    [COLOR_WHITE] = {0xff, 0xff, 0xff}, // White
    [COLOR_BLACK] = {0x00, 0x00, 0x00}, // Black

    [COLOR_BG_BLOCK_BASE] = {0x77, 0x77, 0x77}, // Gray
    [COLOR_BG_BLOCK_HIGHLIGHT] = {0x99, 0x99, 0x99}, // Light gray
    [COLOR_BG_BLOCK_SHADOW] = {0x33, 0x33, 0x33}, // Dark gray
};
