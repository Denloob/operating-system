#include "elf.h"
#include "mmap.h"
#include "memory.h"
#include "res.h"
#include <stdint.h>

enum {
    ELF_ARCHITECTURE_32_BIT = 1,
    ELF_ARCHITECTURE_64_BIT = 2,

    ELF_ENDIANESS_LITTLE    = 1,
    ELF_ENDIANESS_BIG       = 2,

    ELF_ABI_SYSTEM_V        = 0,

    ELF_TYPE_RELOCATABLE    = 1,
    ELF_TYPE_EXECUTABLE     = 2,
    ELF_TYPE_SHARED         = 3,
    ELF_TYPE_CORE           = 4,

    ELF_VERSION             = 1,

    ELF_INSTRUCTION_SET_NO_SPECIFIC    = 0x00,
    ELF_INSTRUCTION_SET_SPARC          = 0x02,
    ELF_INSTRUCTION_SET_X86            = 0x03,
    ELF_INSTRUCTION_SET_MIPS           = 0x08,
    ELF_INSTRUCTION_SET_POWER_PC       = 0x14,
    ELF_INSTRUCTION_SET_ARM            = 0x28,
    ELF_INSTRUCTION_SET_SUPER_H        = 0x2A,
    ELF_INSTRUCTION_SET_IA_64          = 0x32,
    ELF_INSTRUCTION_SET_x86_64         = 0x3E,
    ELF_INSTRUCTION_SET_AARCH64        = 0xB7,
    ELF_INSTRUCTION_SET_RISC_V         = 0xF3,

    ELF_SEGMENT_TYPE_IGNORE      = 0,
    ELF_SEGMENT_TYPE_LOAD        = 1, // Zero out memory for the segment, then read the segment data in the elf to the memory.
                                      // AKA zero out `segment_size_in_memory` bytes at `segment_virtual_address`.
                                      // Then, read `segment_size_in_file` bytes from `segment_offset_in_file` to `segment_virtual_address`.
    ELF_SEGMENT_TYPE_DYNAMIC     = 2, // Requires dynamic linking
    ELF_SEGMENT_TYPE_INTERPRETER = 3, // Contains a path to an executable to use as an interpreter for the following segment
    ELF_SEGMENT_TYPE_NOTES       = 4, // A section for notes

    ELF_SEGMENT_FLAG_EXECUTABLE  = 0x1,
    ELF_SEGMENT_FLAG_WRITABLE    = 0x2,
    ELF_SEGMENT_FLAG_READABLE    = 0x4,
};

typedef struct {
    uint8_t magic_number[4];    // 0x7F,"ELF"
    uint8_t architecture;       // @see - ELF_ARCHITECTURE_* enum
    uint8_t endianess;          // @see - ELF_ENDIANESS_* enum
    uint8_t header_version;
    uint8_t os_abi;             // @see - ELF_ABI_* enum
    uint8_t pad[8];
    uint16_t type;              // @see - ELF_TYPE_* enum
    uint16_t instruction_set;   // @see - ELF_INSTRUCTION_SET_* enum
    uint32_t elf_version;       // @see - ELF_VERSION enum
    uint64_t entry_point;
    uint64_t program_header_table_offset;
    uint64_t section_header_table_offset;
    uint32_t flags;             // Ignored - there's no flags defined for x86
    uint16_t header_size;
    uint16_t program_header_table_entry_size;
    uint16_t program_header_table_length;
    uint16_t section_header_table_entry_size;
    uint16_t section_header_table_length;
    uint16_t section_index; // Section index to the section header string table
} elf_Header;

typedef struct {
    uint32_t segment_type;      // @see - ELF_SEGMENT_TYPE_* enum
    uint32_t flags;             // @see - ELF_SEGMENT_FLAG_* enum. ThE flags are bitwise OR between the enum values.
    uint64_t segment_offset_in_file;  // Offset in the file to the segment this entry is describing
    uint64_t segment_virtual_address; // The virtual address where this segment should be put
    uint64_t segment_physical_address;
    uint64_t segment_size_in_file;
    uint64_t segment_size_in_memory;
    uint64_t section_alignment;
} elf_ProgramHeaderEntry;

static mmap_Protection elf_flags_to_mmap_flags(int elf_flags)
{
    mmap_Protection prot = 0;

    if ((elf_flags & ELF_SEGMENT_FLAG_EXECUTABLE) != 0) prot |= MMAP_PROT_EXEC;
    if ((elf_flags & ELF_SEGMENT_FLAG_WRITABLE) != 0)   prot |= MMAP_PROT_WRITE;
    prot |= MMAP_PROT_RING_3; // All processes are ring 3.
    prot |= MMAP_PROT_READ;   // HACK: Not readable pages are not supported, so we mark all as readable

    return prot;
}

res elf_load(FILE *file, void **entry_point_ptr)
{
    elf_Header header;
    size_t read = fread(&header, sizeof(header), 1, file);
    if (read != 1)
    {
        return res_elf_INVALID_ELF;
    }

    if (memcmp(header.magic_number, "\x7f""ELF", sizeof(header.magic_number)) != 0)
    {
        return res_elf_INVALID_ELF;
    }

    if (header.elf_version != ELF_VERSION ||
        header.endianess != ELF_ENDIANESS_LITTLE ||
        header.architecture != ELF_ARCHITECTURE_64_BIT ||
        header.os_abi != ELF_ABI_SYSTEM_V)
    {
        return res_elf_INVALID_ELF;
    }

    if (entry_point_ptr)
    {
        *entry_point_ptr = (void *)header.entry_point;
    }

    elf_ProgramHeaderEntry entry;
    for (int i = 0; i < header.program_header_table_length; i++)
    {
        int fseek_ret = fseek(file, header.program_header_table_offset + i * header.program_header_table_entry_size, SEEK_SET);
        if (fseek_ret < 0)
        {
            return res_elf_INVALID_ELF;
        }

        size_t read = fread(&entry, sizeof(entry), 1, file);
        if (read != 1)
        {
            return res_elf_INVALID_ELF;
        }

        switch (entry.segment_type)
        {
            case ELF_SEGMENT_TYPE_NOTES:
            case ELF_SEGMENT_TYPE_IGNORE:
            default:
                continue;

            case ELF_SEGMENT_TYPE_LOAD:
            {
                // TODO: on any of the fails, gotta unmap all mapped memory.
                if (entry.segment_size_in_memory < entry.segment_size_in_file)
                {
                    return res_elf_INVALID_ELF;
                }

                // TODO: check that virtual address doesn't overlap with any of the kernel pages.
                res rs = mmap((void *)entry.segment_virtual_address, entry.segment_size_in_memory, MMAP_PROT_READ | MMAP_PROT_WRITE);
                if (!IS_OK(rs))
                {
                    return rs;
                }

                fseek_ret = fseek(file, entry.segment_offset_in_file, SEEK_SET);
                if (fseek_ret < 0)
                {
                    return res_elf_INVALID_ELF;
                }

                size_t read = fread((void *)entry.segment_virtual_address, 1, entry.segment_size_in_file, file);
                if (read != entry.segment_size_in_file)
                {
                    return res_elf_INVALID_ELF;
                }



                memset((char *)entry.segment_virtual_address, 0, entry.segment_size_in_memory - entry.segment_size_in_file);

                rs = mprotect((void *)entry.segment_virtual_address, entry.segment_size_in_memory, elf_flags_to_mmap_flags(entry.flags));
                if (!IS_OK(rs))
                {
                    return rs;
                }
                break;
            }

            case ELF_SEGMENT_TYPE_DYNAMIC:
            case ELF_SEGMENT_TYPE_INTERPRETER:
                return res_elf_UNSUPPORTED;
        }
    }


    return res_OK;
}
