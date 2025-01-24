#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "drive.h"
#include "res.h"

#define SECTOR_SIZE 512
#define FAT16_CLUSTER_FREE 0x0000
#define FAT16_CLUSTER_EOF  0xFFF8 // anything above or equal to this = EOF
#define FAT16_CLUSTER_BAD  0xFFF7


#define FAT16_FILENAME_SIZE 8
#define FAT16_EXTENSION_SIZE 3
#define FAT16_FULL_FILENAME_SIZE (FAT16_FILENAME_SIZE + FAT16_EXTENSION_SIZE)

#define MAX_CHAIN_LEN 50
// FAT16-specific structures

typedef struct
{
    //BPB
    uint8_t  jmpBoot[3];
    uint8_t  OEMName[8];
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t  numFATs;
    uint16_t rootEntryCount;
    uint16_t totalSectors;
    uint8_t  mediaType;
    uint16_t FATSize;
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    uint32_t hiddenSectors;
    uint32_t largeSectors;
    //Extended Boot Record
    uint8_t  driveNumber;
    uint8_t  reserved1;            //this byte should be 0
    uint8_t  bootSignature;
    uint32_t volumeID;
    uint8_t  volumeLabel[11];
    uint8_t  fileSystemType[8];    // (FAT16)
} __attribute__((packed)) fat16_BootSector;

#define fat16_MDSCore_FLAGS_MASK 0xe7 // The mask is used because the bits 0x18 are used for case by MTools.
typedef enum {
    fat16_MDSCoreFlags_FILE   = 0, // A regular fat16 file.
    fat16_MDSCoreFlags_DEVICE = 1, /* This is not a file, it's a device.
                                    * For devices, `firstClusterHigh` is the major device number, and
                                    * `firstClusterLow` is the minor device number.
                                    * The major number is used to identify the driver responsible for the device.
                                    * The minor number is used to identify the part of the driver.
                                    */
} fat16_MDSCoreFlags;

typedef struct
{
    uint8_t  filename[8];
    uint8_t  extension[3];
    uint8_t  attributes;
    uint8_t  reserved; // Masked with fat16_MDSCore_FLAGS_MASK, used by MDSCore for special flags. See fat16_MDSCoreFlags enum.
    uint8_t  creationTimeTenths;
    uint16_t creationTime;
    uint16_t creationDate;
    uint16_t lastAccessDate;
    uint16_t firstClusterHigh;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} __attribute__((packed)) fat16_DirEntry;



// reads the bootsector of the drive into a the address given (third argument / buffer)
bool fat16_read_BPB(Drive *drive , fat16_BootSector *bpb);
// reads the FAT in the drive into a given buffer (named FAT)
bool fat16_read_FAT(Drive *drive , fat16_BootSector *bpb , uint8_t *FAT);

bool fat16_read_sectors(Drive *drive, uint32_t sector, uint8_t *buffer , uint32_t count);

typedef struct
{
    fat16_BootSector bpb;
    Drive *drive;
} fat16_Ref;
//new algo functions
typedef struct 
{
    uint32_t current_sector;
    uint32_t entry_offset;
    uint32_t dir_start;
    uint32_t dir_end;
} fat16_DirReader;

typedef struct 
{
    char name[13];
    uint8_t attr;
    uint16_t cluster; // First cluster
    uint32_t size;
    uint8_t mdscore_flags;
}fat16_dirent;
/*
 *The function returns the amount of entries read
 */
int fat16_getdents(uint16_t first_cluster, fat16_dirent *out_entries_buffer, int max_entries , fat16_Ref *fat16);

void fat16_init_dir_reader(fat16_DirReader *reader, fat16_Ref *fat16, uint16_t start_cluster); 
bool fat16_read_next_root_entry(Drive *drive, fat16_DirReader *reader, fat16_DirEntry *entry);
//----------------

bool fat16_read_root_directory(Drive *drive, fat16_BootSector *bpb, fat16_DirEntry *dir_entries_arr, size_t dir_entries_len);


/*bool fat16_find_file(Drive *drive, fat16_BootSector *bpb,
                     fat16_DirEntry *direntry_arr, size_t dir_size,
                     const char *filename, fat16_DirEntry **out_file);
*/
//new ver of fat16_find_file
bool fat16_find_file(fat16_Ref *fat16, const char *filename, fat16_DirEntry *out_file , int start_cluster);

bool fat16_find_file_based_on_path(fat16_Ref *fat16 , const char *path ,fat16_DirEntry *out_file , fat16_DirEntry *parent_directory);

bool fat16_get_file_chain(fat16_Ref *fat16 , fat16_DirEntry *fileEntry, uint16_t *out_array);

typedef struct
{
    fat16_Ref *ref;
    fat16_DirEntry file_entry;
    uint16_t parent_directory_first_cluster;
    uint16_t chain[MAX_CHAIN_LEN];
} fat16_File;


/**
 * @brief Initialises a fat16 reference struct.
 * @WARN: Invalidating the ref will invalidate all data structures obtained with the ref.
 *
 * @return if succeeded
 */
bool fat16_ref_init(fat16_Ref *fat16, Drive *drive);

/**
 * @brief Opens a path
 *
 * @param path - The path to the file to open
 * @param out_file[out] - The opened file
 * @return if succeeded
 *
 * @see fat16_ref_init
 */
bool fat16_open(fat16_Ref *fat16, const char *path, fat16_File *out_file);
/**
 * @brief Reads the whole file contents into out_buffer.
 *
 * @param file            - The file to read from
 * @param out_buffer[out] - The buffer where the read part will be written
 * @param buffer_size     - The size of the `out_buffer`
 * @param file_offset     - The offset into the file from which to read
 * @return
 *          When drive is verbose: the amount of bytes read
 *          Otherwise: 0 on fail, positive otherwise
 *
 * @see fat16_ref_init
 */
uint64_t fat16_read(fat16_File *file, uint8_t *out_buffer, uint64_t buffer_size, uint64_t file_offset);

/**
 * @brief converts a cluster to sector.
 *
 * @param fat16 the ref that holds the bpb , drive
 * @param cluster The cluster to be converted
 * @return the corresponds sector number.
 */
uint32_t fat16_cluster_to_sector(fat16_Ref *fat16, uint16_t cluster);

bool fat16_write_sectors(Drive *drive, uint32_t sector, const uint8_t *buffer, uint32_t count);
/**
 * @brief the function returns the next cluster number in a clusters chain
 *
 */
uint16_t fat16_get_next_cluster(fat16_Ref *fat16, uint16_t cluster);
/**
 *@brief the function gets a file and writes content into it
 *
 *@file - the file we want to write into
 *@buffer - the data to write into the file
 *@size - the size of the data
 */
bool fat16_add_dir_entry_to(fat16_Ref *fat16, fat16_DirEntry *new_entry , uint16_t first_cluster);

#define fat16_DIRENTRY_ATTR_IS_DIRECTORY 0x10

/**
 * *@brief the function creates a dir entry 
 *
 *@*fat16 - the fat ref (bpb , drive) 
 *@*filename - the file name for the entry
 @extension - the file extension (exe , com , txt , doc ,   bmp , gif , jpg ...)
 *@attributes - the attributes of the file (0x01 - read only / 0x02 - hidden / 0x04 - system / 0x08 -volume label / 0x10 - directory / 0x20  - archive / 0x0F - long file name) can be 2 things or more for example 0x12 is a hidden directory 
*@out_file - the variables where the refrence to the file will be stored
 */
bool fat16_create_dir_entry(fat16_Ref *fat16 , const char *filename , const char *extension , uint8_t attributes , fat16_DirEntry *created_entry);

/**
 * @brief Create a directory in the given path with the given name.
 *
 * @return Whether the function succeeded
 */
res fat16_create_directory(fat16_Ref *fat16 , const char* directory_name , const char *where_to_create);

/**
 *@brief finds a free clutser in the FAT and returns its number
 */
bool fat16_allocate_clusters(fat16_Ref *fat16, uint8_t amount_of_clusters, uint16_t *out_array);

/*
 *@brief the function gets 2 clusters and links them in the FAT
 *
 *@param fat16 - the fat ref (bpb , drive)
 *@param back_cluster - the cluster that the function will link a cluster to for exmaple in the chain 4->5 the cluster will be 4
 *@param front_cluster - the cluster that will be linked in the exampe above 5
 *
 * @return - true or false 
 */
bool fat16_link_clusters(fat16_Ref *fat16, uint16_t back_cluster, uint16_t front_cluster);

/*
 *@brief - the function gets 2 clusters and unlinks then in the FAT
 *
 *@param fat16 - the fat ref (bpb , drive)
 *@param back_cluster - the back cluster in the link
 *@param front_cluster - the front cluster in the link
 *
 * @return - true or false
 */
bool fat16_unlink_clusters(fat16_Ref *fat16 , uint16_t back_cluster , uint16_t front_cluster);

uint32_t fat16_get_file_end_offset(fat16_Ref *fat16, fat16_DirEntry *entry);

/**
 * @brief - Create a file in the given path.
 *
 * @param out_file[out] - The fat16_File structure of the created file
 * @param out_parent_cluster - The first cluster of the parent directory. Can
 *                              be passed in to `fat16_update_entry_in_directory`.
 */
bool fat16_create_file(fat16_Ref *fat16, const char *path, fat16_File *out_file, uint16_t *out_parent_cluster);

uint16_t get_file_offseted_cluster(fat16_File *file, uint32_t file_offset);
/**
 * @brief - Gets the next cluster of a file based on its chain.
 *
 * @param - chain Array representing the file's cluster chain.
 * @param - current_cluster The current cluster for which to find the next.
 * @return - The next cluster number, or FAT16_CLUSTER_EOF if the current cluster is the last one.
 */
uint16_t get_next_cluster_from_chain(const uint16_t *chain, uint16_t current_cluster);

uint64_t fat16_write_to_file_at_directory(fat16_File *file, uint8_t *out_buffer, uint64_t buffer_size, uint64_t file_offset ,res *string_result);

__attribute__((always_inline)) inline
fat16_MDSCoreFlags fat16_get_mdscore_flags(fat16_File *file)
{
    return file->file_entry.reserved & fat16_MDSCore_FLAGS_MASK;
}

/**
 * @brief - Update the given dir entry in the root dir.
 */
bool fat16_update_entry_in_directory(fat16_Ref *fat16, fat16_DirEntry *dir_entry , uint16_t start_cluster);

