#pragma once

#define TABLE_LENGTH 512

#define MMU_TABLE_COUNT 512
#define MMU_BITMAP_SIZE (MMU_TABLE_COUNT / 8)
#define MMU_BITMAP_LENGTH MMU_TABLE_COUNT

#define TABLE_SIZE_BYTES (TABLE_LENGTH * sizeof(mmu_PageMapEntry))

#define MMU_ALL_TABLES_SIZE (MMU_TABLE_COUNT * TABLE_SIZE_BYTES)
#define MMU_TOTAL_CHUNK_SIZE (MMU_BITMAP_SIZE + MMU_ALL_TABLES_SIZE)
