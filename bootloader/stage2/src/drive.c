#include "bios.h"
#include "assert.h"
#include "drive.h"

void drive_lba_to_chs(Drive *drive, uint32_t lba, drive_CHS *ret)
{
    uint32_t temp = lba / drive->sectors;
    ret->sector = lba % drive->sectors + 1;
    ret->cylinder = temp / drive->heads;
    ret->head = temp % drive->heads;

    assert((ret->sector >> 6) == 0    && "Sector cannot be larger than 6 bits");
    assert((ret->cylinder >> 10) == 0 && "Cylinder cannot be larger than 10 bits");
    assert(lba == ((uint32_t)(ret->cylinder) * drive->heads + ret->head) * drive->sectors + (ret->sector - 1) && "Integer overflow detected");
}

bool drive_init(Drive *drive, uint16_t drive_id)
{
    bool success = bios_drive_get_drive_params(drive_id, &drive->type, &drive->cylinders,
                                &drive->sectors, &drive->heads);
    if (success)
    {
        drive->id = drive_id;
        assert(drive->sectors && drive->heads && drive->cylinders && "Drive properties cannot be 0");
    }

    return success;
}
}
