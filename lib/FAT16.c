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
#define FAT16_FILENAME_SIZE (8+3) //(filename + extension)

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
    return fat16_read_sectors(drive, bpb->reservedSectors, FAT, /* bpb->FATSize */ 1 ); // HACK: the FAT should be loaded dynamically.
}

bool fat16_read_sectors(Drive *drive, uint32_t sector, uint8_t *buffer,
                  uint32_t count)
{
    if (count == TAKE_DEFAULT_VALUE)
        count = SECTOR_SIZE;
    return drive_read(drive, sector * SECTOR_SIZE, buffer, count * SECTOR_SIZE);
}



void fat16_init_dir_reader(fat16_DirReader *reader, fat16_BootSector *bpb) 
{
    reader->current_sector = bpb->reservedSectors + bpb->FATSize * bpb->numFATs;
    reader->root_dir_start = reader->current_sector;
    reader->root_dir_end = reader->root_dir_start + (bpb->rootEntryCount * sizeof(fat16_DirEntry) + SECTOR_SIZE - 1) / SECTOR_SIZE;
    reader->entry_offset = 0;
}


bool fat16_read_next_root_entry(Drive *drive, fat16_DirReader *reader, fat16_DirEntry *entry)
{
    if (reader->current_sector >= reader->root_dir_end) return false;

    uint8_t sector_buf[SECTOR_SIZE];
    if (!fat16_read_sectors(drive, reader->current_sector, sector_buf, 1))
        return false;
    *entry = *(fat16_DirEntry *)(sector_buf + reader->entry_offset);

    reader->entry_offset += sizeof(fat16_DirEntry);
    if (reader->entry_offset >= SECTOR_SIZE)
    {
        reader->entry_offset = 0;
        reader->current_sector++;
    }

    return true;
}

bool fat16_read_root_directory(Drive *drive, fat16_BootSector *bpb, fat16_DirEntry *dir_entries_arr, size_t dir_entries_len)
{
    assert(drive && bpb && dir_entries_arr);
    assert(dir_entries_len >= bpb->rootEntryCount && "fat16_read_root_directory: all entries must fix inside the array");

    uint32_t sector_offset = bpb->reservedSectors + bpb->FATSize * bpb->numFATs;
    uint64_t sector_address = sector_offset * SECTOR_SIZE;
    uint32_t size = sizeof(fat16_DirEntry) * bpb->rootEntryCount;

#ifdef DRIVE_SUPPORTS_UNALIGNED
    return drive_read(drive, sector_address, (uint8_t *)dir_entries_arr, size);
#else
    // First, if any, we read all the data that's multiple of SECTOR_SIZE (aligned)
    // then, we read what's left over if any.
    uint32_t size_leftover_part2 = size % SECTOR_SIZE;
    uint32_t size_aligned_part1 = size - size_leftover_part2;
    if (size_aligned_part1)
    {
        bool success = drive_read(drive, sector_address, (uint8_t *)dir_entries_arr, size_aligned_part1);
        if (!success) return false;
    }

    if (size_leftover_part2)
    {
        uint8_t tmp_buf[SECTOR_SIZE];
        bool success = drive_read(drive, sector_address + size_aligned_part1, tmp_buf, SECTOR_SIZE);
        if (!success) return false;

        memmove((uint8_t *)dir_entries_arr + size_aligned_part1, tmp_buf, size_leftover_part2);
    }

    return true;
#endif
}

bool fat16_find_file(Drive *drive, fat16_BootSector *bpb, const char *filename, fat16_DirEntry *out_file)
{
    fat16_DirReader reader;
    fat16_init_dir_reader(&reader, bpb);

    fat16_DirEntry entry;
    while (fat16_read_next_root_entry(drive, &reader, &entry)) 
    {
        if (memcmp(filename, entry.filename, FAT16_FILENAME_SIZE) == 0) 
        {
            *out_file = entry;
            return true;
        }
    }

    return false;
}

bool fat16_read_file(fat16_DirEntry *fileEntry, Drive *drive, fat16_BootSector *bpb,
                     uint8_t *out_buffer, uint16_t *FAT)
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

        currCluster = FAT[currCluster];
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

    out_file->ref = fat16;

    fat16_DirEntry dir_entry;
    if (!fat16_find_file(fat16->drive, &fat16->bpb, path, &dir_entry)) 
    {
        return false;
    }

    out_file->file_entry = dir_entry;
    return true;
}

bool fat16_read(fat16_File *file, uint8_t *out_buffer)
{
    assert(file && out_buffer);

    uint8_t fat[/*file->ref->bpb.FATSize * SECTOR_SIZE*/ SECTOR_SIZE]; // HACK: the fat should be read dynamically
    bool success = fat16_read_FAT(file->ref->drive, &file->ref->bpb, (uint8_t *)&fat);
    if (!success) return false;

    success = fat16_read_file(&file->file_entry , file->ref->drive , &file->ref->bpb ,out_buffer, (uint16_t *)fat); // TODO: it's not a good idea to read the whole FAT into memory.
    if (!success) return false;

    return true;
}

uint32_t fat16_cluster_to_sector(fat16_Ref *fat16, uint16_t cluster)
{
    fat16_BootSector *bpb = &fat16->bpb;
    uint32_t firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->FATSize) + ((bpb->rootEntryCount*32) + (bpb->bytesPerSector-1)) / bpb->bytesPerSector;
    return firstDataSector + (cluster - 2) * bpb->sectorsPerCluster;
}

bool fat16_write_sectors(Drive *drive, uint32_t sector, const uint8_t *buffer, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        //if (!drive_write(drive, sector + i, buffer + (i * SECTOR_SIZE), 1)) // we dont have a drive write yet but assuming we d o
        {
            return false; // Failed to write to the sector
        }
    }
    return true;
}
/*
uint16_t fat16_allocate_cluster(fat16_Ref *fat16)
{
    uint8_t *FAT = malloc(fat16->bpb.FATSize * SECTOR_SIZE);
    if (!fat16_read_FAT(fat16->drive, &fat16->bpb, FAT))
    {
        free(FAT);
        return FAT16_CLUSTER_FREE; // Reading FAT failed
    }

    for (uint16_t i = 2; i < fat16->bpb.FATSize * SECTOR_SIZE / 2; i++)
    {
        uint16_t entry = ((uint16_t*)FAT)[i];
        if (entry == FAT16_CLUSTER_FREE)
        {
            ((uint16_t*)FAT)[i] = FAT16_CLUSTER_EOF;
            fat16_write_sectors(fat16->drive, fat16->bpb.reservedSectors, FAT, fat16->bpb.FATSize);
            free(FAT);
            return i; // Found and allocated a free cluster
        }
    }

    free(FAT);
    return FAT16_CLUSTER_FREE; // No free cluster found
}

uint16_t fat16_get_next_cluster(fat16_Ref *fat16, uint16_t cluster)
{
    uint8_t *FAT = malloc(fat16->bpb.FATSize * SECTOR_SIZE);
    if (!fat16_read_FAT(fat16->drive, &fat16->bpb, FAT))
    {
        free(FAT);
        return FAT16_CLUSTER_EOF; // Reading FAT failed
    }

    uint16_t nextCluster = ((uint16_t*)FAT)[cluster];
    free(FAT);
    return nextCluster;
}

bool fat16_set_next_cluster(fat16_Ref *fat16, uint16_t current_cluster, uint16_t next_cluster)
{
    uint8_t *FAT = malloc(fat16->bpb.FATSize * SECTOR_SIZE);
    if (!fat16_read_FAT(fat16->drive, &fat16->bpb, FAT))
    {
        free(FAT);
        return false; // Reading FAT failed
    }

    ((uint16_t*)FAT)[current_cluster] = next_cluster;

    bool result = fat16_write_sectors(fat16->drive, fat16->bpb.reservedSectors, FAT, fat16->bpb.FATSize);
    free(FAT);
    return result;
}
*/

