#pragma once

#include "FAT16.h"

#define FS_MAX_FILEPATH_LEN 512

extern fat16_Ref g_fs_fat16;

void fs_init(uint16_t drive_id);
