#include "FAT16.h"
#include "drive.h"
#include "math.h"
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

uint16_t get_file_offseted_cluster(fat16_File *file, uint32_t file_offset)
{
    uint16_t index = 0;
    uint16_t first_chain_cluster = file->chain[index];
    uint16_t first_chain_size = file->chain[index + 1];
    uint16_t cur_cluster = first_chain_cluster;
    uint32_t skip_cluster_amount = file_offset / SECTOR_SIZE;

    for (uint16_t i = 0; i < skip_cluster_amount; i++)
    {
        if (first_chain_size > 0) 
        {
            first_chain_size--;
            cur_cluster++;
        } else
        {
            index++;
            first_chain_cluster = file->chain[index];

            if (first_chain_cluster == FAT16_CLUSTER_EOF)
            {
                return -1;
            }

            first_chain_size = file->chain[index + 1];
            cur_cluster = first_chain_cluster;
        }
    }

    return cur_cluster;
}

uint64_t fat16_read_file(fat16_File *file, Drive *drive, fat16_BootSector *bpb,
                     uint8_t *out_buffer, uint64_t buffer_size, uint64_t file_offset )
{
    assert(file->file_entry.reserved == fat16_MDSCoreFlags_FILE && "Cannot read with fat16 a non-regular mdscore file");
    if (file_offset >= file->file_entry.fileSize)
    {
        return 0;
    }

    uint64_t bytes_read = 0;
    FatCache cache = {0};

    const uint32_t rootDirectoryEnd =
        (bpb->reservedSectors + bpb->FATSize * bpb->numFATs) +
        (bpb->rootEntryCount * sizeof(fat16_DirEntry) + SECTOR_SIZE - 1) / SECTOR_SIZE;

    uint16_t cur_cluster = get_file_offseted_cluster(file, file_offset); 
    
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
        if (read_size > file->file_entry.fileSize - file_offset)
        {
            read_size = file->file_entry.fileSize - file_offset;
        }

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
    entry.reserved = fat16_MDSCoreFlags_FILE;
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

bool fat16_update_root_entry(Drive *drive, fat16_BootSector *bpb, fat16_DirEntry *dir_entry)
{
    const uint16_t size_of_dir_entry = 32;
    fat16_DirReader reader;
    fat16_init_dir_reader(&reader, bpb);

    fat16_DirEntry entry;
    while (fat16_read_next_root_entry(drive, &reader, &entry)) 
    {
        if (entry.filename[0] == 0x00 || entry.filename[0] == 0xE5) 
        {
            continue;
        }
        if (memcmp(entry.filename, dir_entry->filename, FAT16_FULL_FILENAME_SIZE) == 0) 
        {
            reader.entry_offset -= size_of_dir_entry;
            uint64_t entry_address = reader.current_sector * SECTOR_SIZE + reader.entry_offset;
            return drive_write(drive, entry_address, (uint8_t *)dir_entry, sizeof(fat16_DirEntry));
        }
    }

    return false;
}

bool fat16_allocate_clusters(fat16_Ref *fat16, uint8_t amount_of_clusters, uint16_t *out_array)
{
    uint16_t current_cluster = 2;
    uint16_t next_cluster;
    FatCache cache = {0};
    uint8_t index = 0;

    while (amount_of_clusters > 0)
    {
        // Find the next available cluster
        if (!get_next_cluster(fat16->drive, &fat16->bpb, current_cluster, &next_cluster, &cache))
        {
            return false; // Error: Unable to read the FAT or no space left
        }

        if (next_cluster == 0x0000)
        {
            // Mark the cluster as EOF in the FAT
            uint32_t fat_offset = fat16->bpb.reservedSectors * SECTOR_SIZE + current_cluster * 2;
            uint8_t eof_bytes[2] = {0xF8, 0xFF}; // EOF marker

            if (!drive_write(fat16->drive, fat_offset, eof_bytes, sizeof(eof_bytes)))
            {
                return false; // Failed to mark cluster in FAT
            }

            // Add the allocated cluster to the output array
            out_array[index++] = current_cluster;
            amount_of_clusters--;
        }

        current_cluster++;
    }

    return true;
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


uint32_t fat16_get_file_end_offset(fat16_Ref *fat16, const fat16_DirEntry *entry) 
{
    uint16_t file_chain[MAX_CHAIN_LEN];

    if (!fat16_get_file_chain(fat16->drive, &fat16->bpb, (char *)&entry->filename[0], file_chain)) {return 0;}

    int len = 0;
    while (file_chain[len] != FAT16_CLUSTER_EOF && len < MAX_CHAIN_LEN) {len++;}
    if (len < 2) {return 0;}

    uint16_t last_cluster = file_chain[len - 2] + file_chain[len - 1] - 1;

    uint32_t cluster_size = fat16->bpb.bytesPerSector * fat16->bpb.sectorsPerCluster;
    uint32_t offset_within_last_cluster = entry->fileSize % cluster_size;
    uint32_t end_offset = (last_cluster - 2) * cluster_size + fat16->bpb.reservedSectors + offset_within_last_cluster;

    return end_offset;
}


bool fat16_create_file(fat16_Ref *fat16, const char *full_filename)
{
    //filename handling
    char fat16_filename[FAT16_FULL_FILENAME_SIZE];
    filename_to_fat16_filename(full_filename, fat16_filename);
    char filename[FAT16_FILENAME_SIZE] = { ' ' };
    char extension[FAT16_EXTENSION_SIZE] = { ' ' };
    memmove(filename, fat16_filename, FAT16_FILENAME_SIZE);
    memmove(extension, fat16_filename + FAT16_FILENAME_SIZE, FAT16_EXTENSION_SIZE);

    fat16_DirEntry new_entry;
    if (!fat16_create_dir_entry(fat16, filename, extension, 0x20 /* Archive attribute */, &new_entry)) {
        printf("Failed to create directory entry structure\n");
        return false;
    }

    if (!fat16_add_root_entry(fat16->drive, &fat16->bpb, &new_entry)) 
    {
        printf("Failed to add the directory entry to the root directory\n");
        return false;
    }

    return true;

}
bool fat16_create_file_with_return(fat16_File *out_file, fat16_Ref *fat16, const char *full_filename)
{

    //filename handling
    char fat16_filename[FAT16_FULL_FILENAME_SIZE];
    filename_to_fat16_filename(full_filename, fat16_filename);
    char filename[FAT16_FILENAME_SIZE] = { ' ' };
    char extension[FAT16_EXTENSION_SIZE] = { ' ' };
    memmove(filename, fat16_filename, FAT16_FILENAME_SIZE);
    memmove(extension, fat16_filename + FAT16_FILENAME_SIZE, FAT16_EXTENSION_SIZE);

    fat16_DirEntry new_entry;
    if (!fat16_create_dir_entry(fat16, filename, extension, 0x20 /* Archive attribute */, &new_entry)) {
        printf("Failed to create directory entry structure\n");
        return false;
    }

    if (!fat16_add_root_entry(fat16->drive, &fat16->bpb, &new_entry)) 
    {
        printf("Failed to add the directory entry to the root directory\n");
        return false;
    }

    out_file->file_entry = new_entry;
    out_file->ref = fat16;
    return true;
}


uint16_t get_next_cluster_from_chain(const uint16_t *chain, uint16_t current_cluster)
{
    const uint16_t *ptr = chain;
    while(*ptr != FAT16_CLUSTER_EOF)
    {
        uint16_t start = *ptr++;
        uint16_t lenght = *ptr++;
        if(current_cluster >= start && current_cluster < start+lenght) //if current cluster is in the current range
        {
            if(current_cluster < start + lenght -1)
            {
                return current_cluster+1;
            }
            else
            {
                return *ptr++;
            }
        }
    }
    return FAT16_CLUSTER_EOF;
}


void print_root_filenames(fat16_Ref *fat16)
{
    fat16_DirReader reader;
    fat16_init_dir_reader(&reader, &fat16->bpb);
    fat16_DirEntry entry;

    printf("Files in the root directory:\n");
    while (fat16_read_next_root_entry(fat16->drive, &reader, &entry)) 
    {
        // Check if the entry is valid (not empty or deleted)
        if (entry.filename[0] != 0x00 && entry.filename[0] != 0xE5)
        {
            // Print the filename
            for (int i = 0; i < 8; i++) 
            {
                if (entry.filename[i] == ' ') 
                    break; // Stop printing at space
                putc(entry.filename[i]);
            }

            // Print extension if present
            if (entry.extension[0] != ' ') 
            {
                putc('.');
                for (int i = 0; i < 3; i++) 
                {
                    if (entry.extension[i] == ' ') 
                        break; // Stop printing at space
                    putc(entry.extension[i]);
                }
            }

            putc('\n'); // Newline after each filename
        }
    }
}

uint64_t fat16_write_to_file(fat16_File *file,Drive *drive,fat16_BootSector *bpb,uint8_t *buffer_to_write,uint64_t buffer_size,uint64_t file_offset)
{
    assert(file->file_entry.reserved == fat16_MDSCoreFlags_FILE && "Cannot write with fat16 to a non-regular mdscore file");

    uint32_t new_wanted_filesize;

    // Make sure the request fits inside a file (doesn't actually check against FAT, only the fat16 struct field).
    bool overflow = __builtin_add_overflow(file_offset, buffer_size, &new_wanted_filesize);
    if (overflow)
    {
        return 0;
    }

    uint16_t cur_cluster;
    uint64_t offset_within_cluster; 
    fat16_DirEntry *entry = &file->file_entry;
    if (entry->firstClusterHigh == 0 && entry->firstClusterLow == 0) // If the file is empty (there's no cluster at all allocated for it)
    {
        file_offset = 0;
        uint16_t clusters_needed = (buffer_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
        uint16_t allocated_clusters[clusters_needed]; // VLA - might not be the best decision, but it's fine

        if (!fat16_allocate_clusters(file->ref, clusters_needed, allocated_clusters))
        {
            return 0; 
        }

        entry->firstClusterLow = allocated_clusters[0] & 0xFFFF;
        entry->firstClusterHigh = (allocated_clusters[0] >> 16) & 0xFFFF;
        for (int i = 1; i < clusters_needed; i++)
        {
            if (!fat16_link_clusters(file->ref, allocated_clusters[i-1], allocated_clusters[i]))
            {
                // TODO: deallocate allocated clusters
                return 0;
            }
        }

        cur_cluster = allocated_clusters[0];
        fat16_get_file_chain(file->ref->drive, &file->ref->bpb, (char *)entry->filename, file->chain);
    }
    else
    {
        cur_cluster = get_file_offseted_cluster(file, file_offset);
        if (cur_cluster == FAT16_CLUSTER_EOF) 
        {
            return 0; 
        }
    }

    offset_within_cluster = file_offset % SECTOR_SIZE;

    const uint32_t rootDirectoryEnd = 
        (bpb->reservedSectors + bpb->FATSize * bpb->numFATs) +
        (bpb->rootEntryCount * sizeof(fat16_DirEntry) + SECTOR_SIZE - 1) / SECTOR_SIZE;

    uint64_t space_left = buffer_size;
    uint64_t bytes_written = 0;
    while (space_left > 0) //while the amount the function was asked to write haven't reached 0
    {
        uint32_t lba = rootDirectoryEnd + (cur_cluster - 2) * bpb->sectorsPerCluster;
        uint32_t address = lba * SECTOR_SIZE + offset_within_cluster;

        // Determine how much to write to the current cluster
        uint64_t cluster_space = SECTOR_SIZE - offset_within_cluster;
        uint64_t write_size = MIN(space_left, cluster_space);
    

#ifdef DRIVE_SUPPORTS_VERBOSE
        uint64_t bytes_written_cur = drive_write_verbose(drive, address, buffer_to_write, write_size);
        bool success = bytes_written_cur == write_size;
#else  
        bool success = drive_write(drive, address, buffer_to_write, write_size);
        uint64_t bytes_written_cur = write_size;
#endif
        bytes_written += bytes_written_cur;

        if (!success)
        {
            break;
        }

        buffer_to_write += bytes_written_cur; //move the array pointer
        space_left -= bytes_written_cur;
        offset_within_cluster = 0; //the offset for cluster is used only for the first cluster 

        // Check if we need to allocate a new cluster
        if (space_left <= 0)
        {
            break;
        }

        uint16_t next_cluster = get_next_cluster_from_chain(file->chain, cur_cluster);
        if (next_cluster != FAT16_CLUSTER_EOF) //no need for new clusters continue writing in the next cluster
        {
            cur_cluster = next_cluster;
            continue;
        }

        // Allocate needed clusters

        uint16_t clusters_needed = (space_left + SECTOR_SIZE - 1) / SECTOR_SIZE; //SECTOR_SIZE - 1 in the calculation is there because of the round down of the operator "/"
        uint16_t allocated_clusters[clusters_needed]; // VLA - again - not good, but not bad

        if (!fat16_allocate_clusters(file->ref, clusters_needed, allocated_clusters))
        {
            break;
        }

        //link the clusters
        bool linking_failed = false;
        if (!fat16_link_clusters(file->ref, cur_cluster, allocated_clusters[0]))
        {
            break;
        }

        cur_cluster = allocated_clusters[0];

        for (int i = 1; i < clusters_needed; i++) 
        {
            if (!fat16_link_clusters(file->ref, allocated_clusters[i - 1], allocated_clusters[i])) 
            {
                // TODO: deallocate the allocated clusters
                linking_failed = true;
                break;
            }
        }

        if (linking_failed)
        {
            break;
        }

        fat16_get_file_chain(file->ref->drive, &file->ref->bpb, (char *)entry->filename, file->chain);
    }

    uint32_t new_filesize = file_offset + bytes_written;
    if (new_filesize > entry->fileSize)
    {
        entry->fileSize = new_filesize;
    }

    fat16_update_root_entry(drive, bpb, entry);

    return bytes_written;
}

uint64_t fat16_write(fat16_File *file, uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset)
{
    assert(file && buffer);
    return fat16_write_to_file(file, file->ref->drive, &file->ref->bpb, buffer , buffer_size, file_offset);
}
