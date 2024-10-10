#include "drive.h"
#include <FAT16.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool fat16_initialize_file_allocation_table(Drive *drive, fat16_BootSector *bpb)
{
    uint32_t totalSectors = (bpb->largeSectors) ? bpb->largeSectors : bpb->totalSectors;
    uint32_t reservedSectors = bpb->reservedSectors;
    uint32_t numFATs = bpb->numFATs;
    uint32_t fatSize = bpb->FATSize;
    uint32_t dataAreaSize = totalSectors - (reservedSectors + (numFATs * fatSize));
    uint32_t numClusters = dataAreaSize / bpb->sectorsPerCluster;

    uint32_t fatBytes = fatSize * SECTOR_SIZE;
    uint16_t *fatTable = malloc(fatBytes);

    memset(fatTable, 0, fatBytes);

    for (uint32_t i = numClusters + 2; i < (fatBytes / sizeof(uint16_t)); i++)
    {
        fatTable[i] = FAT16_CLUSTER_EOF;
    }

    fseek(drive->file, bpb->reservedSectors * SECTOR_SIZE, SEEK_SET);
    fwrite(fatTable, fatBytes, 1, drive->file);
    free(fatTable);

    printf("FAT16 initialized successfully.\n");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <filepath>\n", argv[0]);
        return 1;
    }

    Drive drive = {
        .file = fopen(argv[1], "r+b"),
    };

    fat16_BootSector bpb;
    fseek(drive.file, 0, SEEK_SET);
    fread(&bpb, sizeof(fat16_BootSector), 1, drive.file);

    if (!fat16_initialize_file_allocation_table(&drive, &bpb))
    {
        fclose(drive.file);
        return 1;
    }

    fclose(drive.file);
    return 0;
}
