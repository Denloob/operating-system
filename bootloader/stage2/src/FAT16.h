#pragma once

#include <stdint.h>
#include <stddef.h>

#include <stdbool.h>

#include "drive.h"

#define SECTOR_SIZE 512
#define FAT16_CLUSTER_FREE 0x0000
#define FAT16_CLUSTER_EOF  0xFFFF

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
} __attribute__((packed)) BootSector;


typedef struct
{
    uint8_t  filename[8];
    uint8_t  extension[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  creationTimeTenths;
    uint16_t creationTime;
    uint16_t creationDate;
    uint16_t lastAccessDate;
    uint16_t firstClusterHigh;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} __attribute__((packed)) DirEntry;

//reads the bootsector of the drive into a the address given (third argument / buffer)
bool read_BPB(Drive *drive , BootSector bpb , uint8_t *buffer);
//reads the FAT in the drive into a given buffer (named FAT)
bool read_FAT(Drive *drive , BootSector bpb , uint8_t *FAT);

bool read_sectors(Drive *drive, uint32_t sector, uint8_t *buffer , uint32_t count);

bool read_root_directory(Drive *drive , BootSector bpb , DirEntry *buffer);

DirEntry* find_file(Drive *drive , const char* filename , BootSector *bpb , DirEntry *rootDir);

bool readFile(DirEntry *fileEntry , Drive *drive , BootSector *bpb,uint8_t *out_buffer , uint32_t rootDirecotryEnd , uint8_t *FAT);//rootDirecotryEnd = lba + sectors
