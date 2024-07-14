#include "bios.h"
#include "assert.h"
#include "drive.h"

void drive_lba_to_chs(Drive *drive, uint32_t lba, drive_CHS *ret)
{
    uint32_t temp = lba / drive->sectors;
    ret->head = lba % drive->sectors + 1;
    ret->cylinder = temp / drive->heads;
    ret->head = temp % drive->heads;
}

bool drive_init(Drive *drive, uint16_t drive_id)
{
    bool status = bios_drive_get_drive_params(drive_id, &drive->type, &drive->cylinders,
                                &drive->sectors, &drive->heads);
    if (status)
    {
        assert(drive->sectors && drive->heads && drive->heads && "Drive propreties cannot be 0");
    }

    return status;
}
