#pragma once

#include "FAT16.h"

extern fat16_Ref g_fs_fat16;

void fs_init(uint16_t drive_id);
