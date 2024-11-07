#include "IDE.h"
#include "assert.h"
#include "drive.h"

#define SECTOR_SIZE 512

bool drive_init(Drive *drive, int drive_id)
{
    drive->id = drive_id;
    return true;
}

#define IMPL_DRIVE_READ_WRITE(func_name, ide_func, ide_func_bytes, buffer_type)    \
bool func_name(Drive *drive, uint32_t address, buffer_type buffer, uint32_t size)  \
{                                                                                  \
    const uint32_t offset = address % SECTOR_SIZE;                                 \
    if (offset != 0)                                                               \
    {                                                                              \
        uint32_t partial_size = SECTOR_SIZE - offset;                              \
        if (partial_size > size)                                                   \
            partial_size = size;                                                   \
                                                                                   \
        bool success = ide_func_bytes(drive->id, address / SECTOR_SIZE, buffer,    \
                                      offset, partial_size);                       \
        if (!success)                                                              \
            return false;                                                          \
        size -= partial_size;                                                      \
        address += partial_size;                                                   \
        buffer += partial_size;                                                    \
    }                                                                              \
                                                                                   \
    if (size == 0)                                                                 \
        return true;                                                               \
                                                                                   \
    const uint32_t size_remainder = size % SECTOR_SIZE;                            \
    size -= size_remainder;                                                        \
                                                                                   \
    if (size)                                                                      \
    {                                                                              \
        for (uint32_t sector = address / SECTOR_SIZE; sector < size / SECTOR_SIZE; \
             sector++, buffer += SECTOR_SIZE)                                      \
        {                                                                          \
            bool success = ide_func(drive->id, sector, buffer);                    \
            if (!success)                                                          \
                return false;                                                      \
        }                                                                          \
    }                                                                              \
    address += size;                                                               \
                                                                                   \
    if (size_remainder)                                                            \
    {                                                                              \
        bool success = ide_func_bytes(drive->id, address / SECTOR_SIZE, buffer, 0, \
                                      size_remainder);                             \
        if (!success)                                                              \
            return false;                                                          \
    }                                                                              \
                                                                                   \
    return true;                                                                   \
}                                                                                  \

IMPL_DRIVE_READ_WRITE(drive_read, ide_read_sector, ide_read_bytes, uint8_t *);
IMPL_DRIVE_READ_WRITE(drive_write, ide_write_sector, ide_write_bytes, const uint8_t *);
