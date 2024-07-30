#include "bios.h"
#include "drive.h"
#include "io.h"
#include "assert.h"

#define KERNEL_BASE_ADDRESS (uint8_t *)0x7e00
#define KERNEL_SEGMENT 7

typedef void (*kernel_main)(Drive drive);

void start(uint16_t drive_id)
{
    Drive drive;
    if (!drive_init(&drive, drive_id))
    {
        assert(false && "drive_init failed");
    }

    if (!drive_read(&drive, KERNEL_SEGMENT, KERNEL_BASE_ADDRESS, 512*3))
    {
        assert(false && "Read failed");
    }    
    ((kernel_main)KERNEL_BASE_ADDRESS)(drive);
}
