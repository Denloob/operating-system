#include "fs.h"
#include "assert.h"

fat16_Ref g_fs_fat16;
static Drive g_drive;

void fs_init(uint16_t drive_id)
{
    drive_init(&g_drive, drive_id);

    bool success = fat16_ref_init(&g_fs_fat16, &g_drive);
    assert(success && "fat16_ref_init");
}
