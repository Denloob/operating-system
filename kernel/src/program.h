#include "pcb.h"
#include "elf.h"
#include "res.h"

/**
 * @param argv - The argv to copy into the process. Assume that `argv`'s content won't change (aka const). We can't specify so in C because of C limitations.
 */
res program_setup_from_drive(uint64_t id,  PCB *parent, mmu_PageMapEntry *kernel_pml, fat16_Ref *fat16, const char *path_to_file, char **argv);
