#include "FAT16.h"
#include "drive.h"
#include "memory.h"
#include "assert.h"
#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define TAKE_DEFAULT_VALUE -1
#define FAT16_CLUSTER_FREE 0x0000
#define FAT16_CLUSTER_EOF  0xFFFF

bool fat16_read_BPB(Drive *drive, fat16_BootSector *bpb)
{
    uint8_t buf[SECTOR_SIZE];
    // Boot sector is located at the start of the disk
    if (!drive_read(drive, 0, buf, SECTOR_SIZE))
        return false;

    *bpb = *(fat16_BootSector *)buf;

    return true;
}

bool fat16_read_FAT(Drive *drive, fat16_BootSector *bpb, uint8_t *FAT)
{
    return fat16_read_sectors(drive, bpb->reservedSectors, FAT, bpb->FATSize);
}

bool fat16_read_sectors(Drive *drive, uint32_t sector, uint8_t *buffer,
                  uint32_t count)
{
    if (count == TAKE_DEFAULT_VALUE)
        count = SECTOR_SIZE;
    return drive_read(drive, sector * SECTOR_SIZE, buffer, count);
}

bool fat16_read_root_directory(Drive *drive, fat16_BootSector *bpb, fat16_DirEntry *dir_entries_arr, size_t dir_entries_len)
{
    assert(drive && bpb && dir_entries_arr);
    assert(dir_entries_len >= bpb->rootEntryCount && "fat16_read_root_directory: all entries must fix inside the array");

    uint32_t sector_offset = bpb->reservedSectors + bpb->FATSize * bpb->numFATs;
    uint32_t address = sector_offset * SECTOR_SIZE;
    uint32_t size = sizeof(fat16_DirEntry) * bpb->rootEntryCount;

    // First, if any, we read all the data that's multiple of SECTOR_SIZE (aligned)
    // then, we read what's left over if any.
    uint32_t size_leftover_part2 = size % SECTOR_SIZE;
    uint32_t size_aligned_part1 = size - size_leftover_part2;
    if (size_aligned_part1)
    {
        bool success = drive_read(drive, address, (uint8_t *)dir_entries_arr, size_aligned_part1);
        if (!success) return false;
    }

    if (size_leftover_part2)
    {
        uint8_t tmp_buf[SECTOR_SIZE];
        bool success = drive_read(drive, address + size_aligned_part1, tmp_buf, SECTOR_SIZE);
        if (!success) return false;

        memmove((uint8_t *)dir_entries_arr + size_aligned_part1, tmp_buf, size_leftover_part2);
    }

    return true;
}

bool fat16_find_file(Drive *drive, fat16_BootSector *bpb,
                     fat16_DirEntry *direntry_arr, size_t dir_size,
                     const char *filename, fat16_DirEntry **out_file)
{
    for (uint32_t i = 0; i < dir_size; i++)
    {

// FAT16 uses a 8.3 file name. If extension or filename is shorter that 3 or 8
// bytes accordingly, the missing part is padded with spaces. As both are stored
// near each other in memory, we can simply memcmp the beginning of the one to
// the full filename.
#define FAT16_FILENAME_SIZE                                                    \
    (sizeof(direntry_arr->filename) + sizeof(direntry_arr->extension))

        if (memcmp(filename, direntry_arr[i].filename, FAT16_FILENAME_SIZE) == 0)
        {
            *out_file = &direntry_arr[i];
            return true;
        }
    }

    return false;
}

bool fat16_read_file(fat16_DirEntry *fileEntry, Drive *drive, fat16_BootSector *bpb,
                     uint8_t *out_buffer, uint8_t *FAT)
{
    const uint32_t rootDirectoryEnd =
        (bpb->reservedSectors + bpb->FATSize * bpb->numFATs) +
        (bpb->rootEntryCount * sizeof(fat16_DirEntry) + SECTOR_SIZE - 1) / SECTOR_SIZE;


    uint16_t currCluster = fileEntry->firstClusterLow;
    //read each cluster , get the next cluster (almost the same idea as going threw linked list)
    do
    {
        //cluster to sector
        uint32_t lba =
            rootDirectoryEnd + (currCluster - 2) * bpb->sectorsPerCluster;
        //read cluster
        if (!fat16_read_sectors(drive, lba, out_buffer, bpb->sectorsPerCluster))
        {
            return false;
        }

        out_buffer += bpb->sectorsPerCluster * bpb->bytesPerSector;

        //get next cluster
        uint32_t fatIndex = currCluster * 3 / 2;
        if (currCluster % 2 == 0)
            currCluster = (*(uint16_t *)(FAT + fatIndex)) & 0x0FFF;
        else
            currCluster = (*(uint16_t *)(FAT + fatIndex)) >> 4;
    } while (currCluster < 0xFF8);

    return true;
}

bool fat16_ref_init(fat16_Ref *fat16, Drive *drive)
{
    assert(fat16 && drive);
    fat16->drive = drive;
    return fat16_read_BPB(drive, &fat16->bpb);
}

bool fat16_open(fat16_Ref *fat16, char *path, fat16_File *out_file)
{
    assert(fat16 && path && out_file);

    fat16_DirEntry root[fat16->bpb.rootEntryCount];
    bool success = fat16_read_root_directory(fat16->drive , &fat16->bpb , root, fat16->bpb.rootEntryCount);
    if (!success) return false;

    fat16_DirEntry *dir_entry_ptr;
    success = fat16_find_file(fat16->drive, &fat16->bpb, root, fat16->bpb.rootEntryCount, path, &dir_entry_ptr);
    if (!success) return false;

    out_file->file_entry = *dir_entry_ptr;
    return true;
}

bool fat16_read(fat16_File *file, uint8_t *out_buffer)
{
    assert(file && out_buffer);
    uint8_t fat[file->ref->bpb.FATSize * SECTOR_SIZE];
    bool success = fat16_read_FAT(file->ref->drive, &file->ref->bpb, (uint8_t *)&fat);
    if (!success) return false;

    success = fat16_read_file(&file->file_entry , file->ref->drive , &file->ref->bpb ,out_buffer, fat); // TODO: it's not a good idea to read the whole FAT into memory.
    if (!success) return false;

    return true;
}
