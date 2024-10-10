#include "FAT16.h"
#include "drive.h"
#include "memory.h"
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

bool fat16_read_FAT(Drive *drive, fat16_BootSector bpb, uint8_t *FAT)
{
    return fat16_read_sectors(drive, bpb.reservedSectors, FAT, bpb.FATSize);
}

bool fat16_read_sectors(Drive *drive, uint32_t sector, uint8_t *buffer,
                  uint32_t count)
{
    if (count == TAKE_DEFAULT_VALUE)
        count = SECTOR_SIZE;
    return drive_read(drive, sector * SECTOR_SIZE, buffer, count);
}

bool fat16_read_root_directory(Drive *drive, fat16_BootSector bpb, fat16_DirEntry *buffer)
{
    uint32_t sector = bpb.reservedSectors + bpb.FATSize * bpb.numFATs;
    uint32_t size = sizeof(fat16_DirEntry) * bpb.rootEntryCount;
    uint32_t sectors = (size / bpb.bytesPerSector);
    if (size % bpb.bytesPerSector > 0)
        sectors++;

    return drive_read(drive, sector, buffer, sectors);
}

bool fat16_find_file(Drive *drive, const char *filename, fat16_BootSector *bpb,
                    fat16_DirEntry *rootDir, fat16_DirEntry **out_file)
{
    for (uint32_t i = 0; i < bpb->rootEntryCount; i++)
    {
        if (memcmp(filename, rootDir[i].filename, 11) == 0)
        {
            *out_file = &rootDir[i];
            return true;
        }
    }

    return false;
}

bool fat16_read_file(fat16_DirEntry *fileEntry, Drive *drive, fat16_BootSector *bpb,
              uint8_t *out_buffer, uint32_t rootDirectoryEnd, uint8_t *FAT)
{
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