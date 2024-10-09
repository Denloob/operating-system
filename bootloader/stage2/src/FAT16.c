#include "FAT16.h"
#include "drive.h"
#include "memory.h"
#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define TAKE_DEFUALT_VALUE -1

bool read_BPB(Drive *drive , BootSector bpb , uint8_t *buffer)
{
    //Boot sector is located at the start of the disk
    if(!drive_read(drive , 0 , buffer, SECTOR_SIZE))
        return false;
    return true;
}

bool read_FAT(Drive *drive , BootSector bpb ,uint8_t *FAT)
{
    return read_sectors(drive , bpb.reservedSectors ,FAT , bpb.FATSize);
}
		
bool read_sectors(Drive *drive , uint32_t sector , uint8_t *buffer , uint32_t count)
{
     if(count == TAKE_DEFUALT_VALUE)
	     count = SECTOR_SIZE;
    return drive_read(drive , sector * SECTOR_SIZE , buffer , count);
}

bool read_root_directory(Drive *drive , BootSector bpb , DirEntry *buffer)
{
    uint32_t sector = bpb.reservedSectors + bpb.FATSize * bpb.numFATs;
    uint32_t size = sizeof(DirEntry) * bpb.rootEntryCount;
    uint32_t sectors = (size / bpb.bytesPerSector);
    if(size % bpb.bytesPerSector >0)
	sectors++;

    return drive_read(drive, sector , buffer, sectors);
}

DirEntry* find_file(Drive *drive , const char* filename , BootSector *bpb , DirEntry *rootDir)
{
  for(uint32_t i = 0; i<bpb->rootEntryCount; i++)
  {
	if(memcmp(filename, rootDir[i].filename , 11) ==0)
	{
	   return &rootDir[i]; 
	}
  }

  return NULL;
}

bool readFile(DirEntry *fileEntry, Drive *drive, BootSector *bpb ,  uint8_t *out_buffer , uint32_t rootDirectoryEnd , uint8_t* FAT)
{
    uint16_t currCluster = fileEntry->firstClusterLow;
    bool flag = true;
    //read each cluster , get the next cluster (almost the same idea as going threw linked list)
    do
    {
        //cluster to sector
        uint32_t lba = rootDirectoryEnd + (currCluster - 2) * bpb->sectorsPerCluster;
        //read cluster
        flag = flag && read_sectors(drive, lba, out_buffer, bpb->sectorsPerCluster);
        out_buffer += bpb->sectorsPerCluster * bpb->bytesPerSector;

        //get next cluster
        uint32_t fatIndex = currCluster * 3 / 2;
        if(currCluster % 2 == 0)
            currCluster = (*(uint16_t*)(FAT + fatIndex)) & 0x0FFF;
        else
            currCluster = (*(uint16_t*)(FAT + fatIndex)) >> 4;
    }while(flag && currCluster < 0xFF8);
}



