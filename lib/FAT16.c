#include "FAT16.h"
#include "drive.h"
#include "memory.h"
#include "string.h"
#include "assert.h"
#include "RTC.h"
#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define TAKE_DEFAULT_VALUE -1
#define FAT16_FILENAME_SIZE 8
#define FAT16_EXTENSION_SIZE 3
#define FAT16_FULL_FILENAME_SIZE (FAT16_FILENAME_SIZE + FAT16_EXTENSION_SIZE)

typedef struct
{
    uint8_t buf[SECTOR_SIZE];
    uint32_t cached_sector;
    bool valid; // true iff cache contains anything. Do not read cache unless true.
} FatCache;


bool fat16_read_BPB(Drive *drive, fat16_BootSector *bpb)
{
    uint8_t buf[SECTOR_SIZE];
    // Boot sector is located at the start of the disk
    if (!drive_read(drive, 0, buf, SECTOR_SIZE))
        return false;

    *bpb = *(fat16_BootSector *)buf;
    return true;
}

bool fat16_read_FAT_at(Drive *drive, fat16_BootSector *bpb, uint8_t FAT[static SECTOR_SIZE], uint32_t sector_offset)
{
    assert(sector_offset < bpb->FATSize);

    uint32_t total_fat_sectors = bpb->FATSize * bpb->numFATs;

    //reading out side of the fat/s (unwanted behavior)
    if(sector_offset >= total_fat_sectors)
    {
        return false;
    }
    return fat16_read_sectors(drive, bpb->reservedSectors + sector_offset, FAT, 1);
}

bool fat16_read_sectors(Drive *drive, uint32_t sector, uint8_t *buffer,
                  uint32_t count)
{
    if (count == TAKE_DEFAULT_VALUE)
        count = SECTOR_SIZE;
    return drive_read(drive, sector * SECTOR_SIZE, buffer, count * SECTOR_SIZE);
}


bool fat16_write_sectors(Drive *drive, uint32_t sector, const uint8_t *buffer, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        if (!drive_write(drive, sector + i, buffer + (i * SECTOR_SIZE), 1))
        {
            return false; 
        }
    }
    return true;
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

/**
 * @brief - Convert a filename like `kernel.bin` into fat16-padded name like `kernel  bin`.
 */
void filename_to_fat16_filename(const char *filename, char out_buf[static FAT16_FULL_FILENAME_SIZE])
{
    memset(out_buf, ' ', FAT16_FULL_FILENAME_SIZE);
    printf("file name: %s \n" , filename);
    char *dot = memrchr(filename, '.', FAT16_FILENAME_SIZE + 1);
    assert(dot != NULL && "Invalid path");

    size_t extension_len = strlen(dot) - 1;
    assert(extension_len <= 3 && "Extensions longer than 3 bytes are not supported");

    memmove(out_buf,                       filename, dot - filename);
    memmove(out_buf + FAT16_FILENAME_SIZE, dot + 1,  extension_len);
}

bool fat16_find_file(Drive *drive, fat16_BootSector *bpb, const char *filename, fat16_DirEntry *out_file)
{
    fat16_DirReader reader;
    fat16_init_dir_reader(&reader, bpb);

    fat16_DirEntry entry;
    while (fat16_read_next_root_entry(drive, &reader, &entry)) 
    {
        if (strncasecmp(filename, (const char *)entry.filename, FAT16_FULL_FILENAME_SIZE) == 0) 
        {
            *out_file = entry;
            return true;
        }
    }

    return false;
}

bool get_next_cluster(Drive *drive, fat16_BootSector *bpb, uint16_t cur_cluster, uint16_t *next_cluster, FatCache *cache)
{
    const uint32_t fat_offset = cur_cluster * 2;
    const uint32_t fat_sector = fat_offset / SECTOR_SIZE;
    const uint32_t fat_entry_offset = fat_offset % SECTOR_SIZE;
    if (!cache->valid || cache->cached_sector != fat_sector)
    {
        bool success = fat16_read_FAT_at(drive, bpb, cache->buf, fat_sector);
        if (!success) return false;

        cache->cached_sector = fat_sector;
        cache->valid = true;
    }

    *next_cluster = *(uint16_t *)&cache->buf[fat_entry_offset];
    return true;
}


bool fat16_get_file_chain(Drive *drive, fat16_BootSector *bpb, const char *filename, uint16_t *out_array)
{
    fat16_DirEntry fileEntry;
    if (!fat16_find_file(drive, bpb, filename, &fileEntry))
    {
        return false;
    }

    uint16_t curr_cluster = fileEntry.firstClusterLow;
    uint16_t *chain_ptr = out_array;
    uint16_t range_start = curr_cluster;
    uint16_t range_length = 0;
    FatCache cache = {0};

    while (curr_cluster < FAT16_CLUSTER_EOF)
    {
        range_length++;  //found 1 more cluster to the current range

           uint16_t next_cluster;

        //read next FAT cluster
        if (!get_next_cluster(drive, bpb, curr_cluster, &next_cluster, &cache))
        {
            return false;
        }

        // Check if its realeted for current chunk
        if (next_cluster != curr_cluster + 1 || next_cluster >= FAT16_CLUSTER_EOF)
        {
            //chunk ended
            *chain_ptr++ = range_start;
            *chain_ptr++ = range_length;

            //reset chunk
            range_start = next_cluster;
            range_length = 0;
        }

        curr_cluster = next_cluster;
    }

    //register last chunk
    if (range_length > 0)
    {
        *chain_ptr++ = range_start;
        *chain_ptr++ = range_length;
    }

    *chain_ptr = FAT16_CLUSTER_EOF;  //end of arr
    return true;
}


uint64_t fat16_read_file(fat16_File *file, Drive *drive, fat16_BootSector *bpb,
                     uint8_t *out_buffer, uint64_t buffer_size, uint64_t file_offset )
{
    uint64_t bytes_read = 0;
    FatCache cache = {0};

    const uint32_t rootDirectoryEnd =
        (bpb->reservedSectors + bpb->FATSize * bpb->numFATs) +
        (bpb->rootEntryCount * sizeof(fat16_DirEntry) + SECTOR_SIZE - 1) / SECTOR_SIZE;

    uint16_t index = 0;
    uint16_t first_chain_cluster = file->chain[index];
    uint16_t first_chain_size = file->chain[index+1];
    uint16_t cur_cluster = first_chain_cluster;
    uint32_t skip_cluster_amount = file_offset / SECTOR_SIZE;

    for(uint16_t i = 0; i<skip_cluster_amount;i++)
    {
        if(first_chain_size > 0)
        {
            first_chain_size = first_chain_cluster - 1;
            cur_cluster = cur_cluster + 1;
        }
        else
        {
            index = index+1;
            first_chain_cluster = file->chain[index];
            if(first_chain_cluster == FAT16_CLUSTER_EOF)
            {
                return -1;
            }
            else
            {
                first_chain_size = file->chain[index+1];
                cur_cluster = first_chain_cluster;
            }
        }
    }
    file_offset = file_offset % SECTOR_SIZE;

    uint64_t space_in_buffer_left = buffer_size;

    //read each cluster , get the next cluster (almost the same idea as going throught linked list)
    while (cur_cluster < FAT16_CLUSTER_EOF && space_in_buffer_left > 0)
    {
        //cluster to sector
        uint32_t lba =
            rootDirectoryEnd + (cur_cluster - 2) * bpb->sectorsPerCluster;
        uint32_t address = lba * SECTOR_SIZE + file_offset;
        uint32_t read_size = SECTOR_SIZE - file_offset;
        if (read_size > space_in_buffer_left) read_size = space_in_buffer_left;
        file_offset = 0; // First time, if there's an offset (!= 0), we will start at it, but after that the offset is basically ignored.

        //read cluster
#ifdef DRIVE_SUPPORTS_VERBOSE
        uint64_t bytes_read_cur = drive_read_verbose(drive, address, out_buffer, read_size);
        bool success = bytes_read_cur == read_size;
        bytes_read += bytes_read_cur;
        out_buffer += bytes_read_cur;
#else
        bool success = bytes_read = drive_read(drive, address, out_buffer, read_size);
        out_buffer += bpb->sectorsPerCluster * bpb->bytesPerSector;
#endif
        if (!success)
        {
            return bytes_read;
        }
        space_in_buffer_left -= read_size;

        // Get the next cluster
        success = get_next_cluster(drive, bpb, cur_cluster, &cur_cluster, &cache);
        if (!success) return false;
    };

#ifdef DRIVE_SUPPORTS_VERBOSE
    return bytes_read;
#else
    return true;
#endif
}
bool fat16_ref_init(fat16_Ref *fat16, Drive *drive)
{
    assert(fat16 && drive);
    fat16->drive = drive;
    return fat16_read_BPB(drive, &fat16->bpb);
}

bool fat16_open(fat16_Ref *fat16, const char *path, fat16_File *out_file)
{
    assert(fat16 && path && out_file);

    out_file->ref = fat16;

    char fat16_filename[FAT16_FULL_FILENAME_SIZE];
    filename_to_fat16_filename(path, fat16_filename);

    fat16_DirEntry dir_entry;
    if (!fat16_find_file(fat16->drive, &fat16->bpb, fat16_filename, &dir_entry)) 
    {
        return false;
    }

    out_file->file_entry = dir_entry;
    fat16_get_file_chain(fat16->drive, &fat16->bpb, fat16_filename, out_file->chain);
 
    return true;
}

uint64_t fat16_read(fat16_File *file, uint8_t *out_buffer, uint64_t buffer_size, uint64_t file_offset)
{
    assert(file && out_buffer);
    return fat16_read_file(file, file->ref->drive, &file->ref->bpb, out_buffer, buffer_size, file_offset);
}

uint32_t fat16_cluster_to_sector(fat16_Ref *fat16, uint16_t cluster)
{
    fat16_BootSector *bpb = &fat16->bpb;
    uint32_t firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->FATSize) + ((bpb->rootEntryCount*32) + (bpb->bytesPerSector-1)) / bpb->bytesPerSector;
    return firstDataSector + (cluster - 2) * bpb->sectorsPerCluster;
}


bool fat16_write_FAT_at(Drive *drive, fat16_BootSector *bpb, const uint8_t FAT[static SECTOR_SIZE], uint32_t sector_offset)
{
    assert(sector_offset < bpb->FATSize);

    uint32_t total_fat_sectors = bpb->FATSize * bpb->numFATs;

    // Prevent writing outside of the FAT/s (unwanted behavior)
    if (sector_offset >= total_fat_sectors)
    {
        return false;
    }
    return fat16_write_sectors(drive, bpb->reservedSectors + sector_offset, FAT, 1);
}



bool fat16_add_root_entry(Drive *drive, fat16_BootSector *bpb, fat16_DirEntry *new_entry) 
{
    fat16_DirReader reader;
    fat16_init_dir_reader(&reader, bpb);

    fat16_DirEntry entry;
    while (fat16_read_next_root_entry(drive, &reader, &entry)) 
    {
        if (entry.filename[0] != 0x00 && entry.filename[0] != 0xE5) 
            continue;

        uint64_t entry_address = reader.current_sector * SECTOR_SIZE + reader.entry_offset - sizeof(fat16_DirEntry);
        return drive_write(drive, entry_address, (uint8_t *)new_entry, sizeof(fat16_DirEntry));
    }

    return false;
}

bool fat16_create_dir_entry(fat16_Ref *fat16, const char *filename, const char *extension, uint8_t attributes, fat16_DirEntry *created_entry)
{
    fat16_DirEntry entry = {0};
    for(size_t i = 0; i < FAT16_FILENAME_SIZE; i++)
    {
        if(i<strlen(filename)) { entry.filename[i] = filename[i];}
        else {entry.filename[i] = ' ';}
    }
    for(size_t i = 0; i < FAT16_EXTENSION_SIZE; i++)
    {
        if(i<strlen(extension)){entry.extension[i] = extension[i];}
        else{entry.extension[i] = ' ';}
    }
    entry.attributes = attributes;
    uint8_t hours , minutes , seconds;
    RTC_get_time(&hours, &minutes, &seconds);
    entry.creationTime = (hours<<11) | (minutes << 5) | (seconds/2);

    uint16_t year;
    uint8_t month , day;
    RTC_get_date(&year, &month, &day);
    entry.creationDate = ((year-1980) << 9) | (month <<5) | day;
    entry.reserved = 0;
    entry.reserved = 0;
    entry.creationTimeTenths = 0;
    entry.lastAccessDate = entry.creationDate;
    entry.lastModTime = entry.creationTime;
    entry.lastModDate = entry.creationDate;
    entry.firstClusterHigh = 0; 
    entry.firstClusterLow = 0;
    entry.fileSize = 0; 
    
    *created_entry = entry;

    return true;
}


uint16_t fat16_allocate_cluster(fat16_Ref *fat16)
{
    uint16_t current_cluster = 2;
    uint16_t next_cluster;
    FatCache cache = {0};
    while (true)
    {
        if (!get_next_cluster(fat16->drive, &fat16->bpb, current_cluster, &next_cluster, &cache))
        {
            return 0xFFFF; // Error: Unable to read the FAT or no space left :) 
        }

        if (next_cluster == 0x0000)
        {
            uint32_t fat_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + current_cluster * 2;
            uint8_t eof_bytes[2] = {0xF8, 0xFF}; //EOF

            if (!drive_write(fat16->drive, fat_offset, eof_bytes, sizeof(eof_bytes))){return 0xFFFF;} //failed to mark cluster in FAT
            return current_cluster;
        }
        current_cluster++;
    }
}

bool fat16_link_clusters(fat16_Ref *fat16, uint16_t back_cluster, uint16_t front_cluster)
{
    const uint32_t back_fat_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + back_cluster * 2;
    const uint32_t front_fat_offset = fat16->bpb.reservedSectors* SECTOR_SIZE + front_cluster * 2;

    uint8_t front_cluster_bytes[2] ={ (uint8_t)(front_cluster & 0xFF),(uint8_t)((front_cluster >> 8) & 0xFF)};

    uint8_t eof_bytes[2] = { 0xF8, 0xFF };


    if (!drive_write(fat16->drive, back_fat_offset, front_cluster_bytes, sizeof(front_cluster_bytes))) 
    {
        return false; // Failed to write
    }

   if (!drive_write(fat16->drive, front_fat_offset, eof_bytes, sizeof(eof_bytes))) {return false;}

  return true;
}

bool fat16_unlink_clusters(fat16_Ref *fat16 , uint16_t back_cluster , uint16_t front_cluster)
{
    const uint32_t back_fat_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + back_cluster *2;

    const uint32_t front_fat_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + front_cluster *2;

    uint8_t front_cluster_bytes[2] = {0x00 , 0x00}; //Free cluster

    uint8_t back_cluster_bytes[2] = {0xF8 , 0xFF}; //EOF

    if (!drive_write(fat16->drive, front_fat_offset, front_cluster_bytes, sizeof(front_cluster_bytes))) 
    {
        return false; // Failed to write
    }

   if (!drive_write(fat16->drive, back_fat_offset, back_cluster_bytes , sizeof(back_cluster_bytes))) {return false;}

   return true;
}



void fat16_test(fat16_Ref *fat16) 
{
    uint16_t cluster1, cluster2;
    uint8_t fat_buffer[SECTOR_SIZE] = {0};

    // Allocate the first cluster
    cluster1 = fat16_allocate_cluster(fat16);
    if (cluster1 == 0xFFFF) {
        printf("Error: Failed to allocate first cluster\n");
        return;
    }
    printf("Allocated cluster1: %d\n", cluster1);

    // Allocate the second cluster
    cluster2 = fat16_allocate_cluster(fat16);
    if (cluster2 == 0xFFFF) {
        printf("Error: Failed to allocate second cluster\n");
        return;
    }
    printf("Allocated cluster2: %d\n", cluster2);

    // Link the clusters
    if (!fat16_link_clusters(fat16, cluster1, cluster2)) {
        printf("Error: Failed to link clusters %d -> %d\n", cluster1, cluster2);
        return;
    }
    printf("Successfully linked clusters %d -> %d\n", cluster1, cluster2);

    // Verify the FAT entries for cluster1 and cluster2
    uint32_t cluster1_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + cluster1 * 2;
    if (drive_read(fat16->drive, cluster1_offset, fat_buffer, sizeof(uint16_t))) {
        uint16_t cluster1_value = (fat_buffer[1] << 8) | fat_buffer[0];
        printf("FAT entry for cluster1 (%d): %d\n", cluster1, cluster1_value);
    } else {
        printf("Error: Failed to read FAT entry for cluster1 (%d)\n", cluster1);
        return;
    }

    uint32_t cluster2_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + cluster2 * 2;
    if (drive_read(fat16->drive, cluster2_offset, fat_buffer, sizeof(uint16_t))) {
        uint16_t cluster2_value = (fat_buffer[1] << 8) | fat_buffer[0];
        printf("FAT entry for cluster2 (%d): %d\n", cluster2, cluster2_value);
    } else {
        printf("Error: Failed to read FAT entry for cluster2 (%d)\n", cluster2);
        return;
    }

    // Unlink the clusters
    if (!fat16_unlink_clusters(fat16, cluster1, cluster2))
    {
        printf("Error: Failed to unlink clusters %d -> %d\n", cluster1, cluster2);
        return;
    }
    printf("Successfully unlinked clusters %d -> %d\n", cluster1, cluster2);

    // Verify the FAT entries for cluster1 and cluster2 after unlinking
    if (drive_read(fat16->drive, cluster1_offset, fat_buffer, sizeof(uint16_t))) {
        uint16_t cluster1_value = (fat_buffer[1] << 8) | fat_buffer[0];
        printf("FAT entry for cluster1 after unlink (%d): %d\n", cluster1, cluster1_value);
    } else {
        printf("Error: Failed to read FAT entry for cluster1 after unlink (%d)\n", cluster1);
        return;
    }

    if (drive_read(fat16->drive, cluster2_offset, fat_buffer, sizeof(uint16_t))) {
        uint16_t cluster2_value = (fat_buffer[1] << 8) | fat_buffer[0];
        printf("FAT entry for cluster2 after unlink (%d): %d\n", cluster2, cluster2_value);
    } else {
        printf("Error: Failed to read FAT entry for cluster2 after unlink (%d)\n", cluster2);
        return;
    }

    printf("FAT16 test completed successfully.\n");
}

