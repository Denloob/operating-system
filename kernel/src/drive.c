#include "IDE.h"
#include "assert.h"
#include "drive.h"

#define SECTOR_SIZE 512

bool drive_init(Drive *drive, int drive_id)
{
    drive->id = drive_id;
    return true;
}

#define IMPL_DRIVE_READ_WRITE_VERBOSE(func_name, ide_func, ide_func_bytes, buffer_type)    \
uint64_t func_name(Drive *drive, uint64_t address, buffer_type buffer, uint32_t size)  \
{                                                                                  \
    uint64_t bytes_read = 0;                                                       \
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
            return bytes_read;                                                     \
        bytes_read += partial_size;                                                \
        size -= partial_size;                                                      \
        address += partial_size;                                                   \
        buffer += partial_size;                                                    \
    }                                                                              \
                                                                                   \
    if (size == 0)                                                                 \
        return bytes_read;                                                         \
                                                                                   \
    const uint32_t size_remainder = size % SECTOR_SIZE;                            \
    size -= size_remainder;                                                        \
                                                                                   \
    if (size)                                                                      \
    {                                                                              \
        uint32_t sectors_begin = address / SECTOR_SIZE;                            \
        uint32_t sectors_end   = sectors_begin + size / SECTOR_SIZE;               \
        for (uint32_t sector = sectors_begin; sector < sectors_end;                \
             sector++, buffer += SECTOR_SIZE)                                      \
        {                                                                          \
            bool success = ide_func(drive->id, sector, buffer);                    \
            if (!success)                                                          \
                return bytes_read;                                                 \
            bytes_read += SECTOR_SIZE;                                             \
        }                                                                          \
    }                                                                              \
    address += size;                                                               \
                                                                                   \
    if (size_remainder)                                                            \
    {                                                                              \
        bool success = ide_func_bytes(drive->id, address / SECTOR_SIZE, buffer, 0, \
                                      size_remainder);                             \
        if (!success)                                                              \
            return bytes_read;                                                     \
        bytes_read += size_remainder;                                              \
    }                                                                              \
                                                                                   \
    return bytes_read;                                                             \
}                                                                                  \

IMPL_DRIVE_READ_WRITE_VERBOSE(drive_read_verbose, ide_read_sector, ide_read_bytes, uint8_t *);
IMPL_DRIVE_READ_WRITE_VERBOSE(drive_write_verbose, ide_write_sector, ide_write_bytes, const uint8_t *);

bool drive_read(Drive *drive, uint64_t address, uint8_t *buffer, uint32_t size)
{
    return drive_read_verbose(drive, address, buffer, size) == size;
}

bool drive_write(Drive *drive, uint64_t address, const uint8_t *buffer, uint32_t size)
{
    return drive_write_verbose(drive, address, buffer, size) == size;
}
