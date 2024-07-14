#include "bios.h"
#include "drive.h"
#include "io.h"

void start(uint16_t drive_id)
{
    Drive drive;
    drive_init(&drive, drive_id);

    #define TARGET_STRING "Hello, World"
    uint8_t buf[512] = {0};
    uint32_t physical_address_of_target = ((int)TARGET_STRING - 0x500 + SECTOR_SIZE);

    if (!drive_read(&drive, physical_address_of_target / SECTOR_SIZE, buf, 512))
    {
        puts("Read failed");
    }

    puts((char *)buf + physical_address_of_target % SECTOR_SIZE);
}
