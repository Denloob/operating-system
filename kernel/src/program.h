#include "pcb.h"
#include "elf.h"
#include "res.h"

res program_setup(uint64_t id,  PCB *parent, mmu_PageMapEntry *kernel_pml, fat16_Ref *fat16, const char *path_to_file);
