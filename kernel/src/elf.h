#pragma once

#include "file.h"
#include "res.h"

#define res_elf_INVALID_ELF "The given ELF is not valid"
#define res_elf_UNSUPPORTED "The given ELF depends on an unsupported feature"

/**
 * @brief - Load the ELF from the given file and map all segments.
 *
 * @param file - The ELF file to load (must be a static executable).
 * @param entry_point[out] - The entry point of the parsed ELF. Nullable.
 * @return     - res_OK on success, one of the error codes otherwise.
 */
res elf_load(FILE *file, void **entry_point);
