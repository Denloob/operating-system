#include "error.isr.h"
#include "io.h"
#include "isr.h"
#include "vga.h"
#include "assert.h"
#include "memory.h"
#include <stdint.h>

typedef enum {
    GPF_TABLE_GDT = 0b00,
    GPF_TABLE_IDT = 0b01,
    GPF_TABLE_LDT = 0b10,
    GPF_TABLE_IDT2 = 0b11,
} GPFTableType;

typedef struct {
    uint64_t external : 1; // When set, the exception originated externally to the processor.
    uint64_t table_type: 2; // @see GPFTableType
    uint64_t selector_index : 13; // The index inside the corresponding table
    uint64_t pad : 48;
} GPFErrorCode;

static const char *gpf_table_name_str(GPFErrorCode code)
{
    switch (code.table_type)
    {
        case GPF_TABLE_GDT:
            return "GDT";
        case GPF_TABLE_LDT:
            return "LDT";
        case GPF_TABLE_IDT:
        case GPF_TABLE_IDT2:
            return "IDT";
        default:
            assert(false && "Invalid GPF error code table number");
    }
}

static bool gpf_has_error_code(GPFErrorCode code)
{
    uint64_t empty_code = 0;
    return memcmp(&code, &empty_code, sizeof(empty_code)) != 0;
}

static void __attribute__((used, sysv_abi))
error_isr_general_protection_fault_impl(isr_InterruptFrame *frame, GPFErrorCode code)
{
    if (vga_current_mode() != VGA_MODE_TYPE_TEXT)
    {
        vga_mode_text();
        io_clear_vga();
    }

    printf("\nGeneral protection fault: RIP=0x%llx; ", frame->rip);

    if (!gpf_has_error_code(code))
    {
        printf("No error code\n");
    }
    else
    {
        if (code.external)
            printf("[EXTERNAL] ");

        printf("%s at 0x%x\n", gpf_table_name_str(code), code.selector_index);
    }

    printf("Press any key to continue\n");
    io_wait_key_raw();
}

void __attribute__((naked)) error_isr_general_protection_fault()
{
    isr_IMPL_INTERRUPT_ERROR(error_isr_general_protection_fault_impl);
}

typedef struct
{
    uint32_t present : 1;                   // When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
    uint32_t write : 1;                     // When set, the page fault was caused by a write access. When not set, it was caused by a read access.
    uint32_t user : 1;                      // When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
    uint32_t reserved_write : 1;            // When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
    uint32_t instruction_fetch : 1;         // When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.
    uint32_t protection_key : 1;            // When set, the page fault was caused by a protection-key violation. The PKRU register (for user-mode accesses) or PKRS MSR (for supervisor-mode accesses) specifies the protection key rights.
    uint32_t shadow_stack : 1;              // When set, the page fault was caused by a shadow stack access.
    uint32_t reserved8 : 8;
    uint32_t software_guard_extensions : 1; // When set, the fault was due to an SGX violation. The fault is unrelated to ordinary paging.
    uint32_t reserved16 : 16;
    uint32_t pad; // We pad it to 64 bits for x86-64 ISR error code size requirements
} PageFaultErrorCode;

void __attribute__((used, sysv_abi))
error_isr_page_fault_impl(isr_InterruptFrame *frame, PageFaultErrorCode error)
{
    if (vga_current_mode() != VGA_MODE_TYPE_TEXT)
    {
        vga_mode_text();
        io_clear_vga();
    }

    uint64_t cr2;
    asm volatile ("mov %0, cr2" : "=r"(cr2));
    printf("\nPage Fault: RIP=0x%llx; Virtual address cause: %llx\n", frame->rip, cr2);
    printf("present = %d\n"
           "write = %d\n"
           "user = %d\n"
           "reserved_write = %d\n"
           "instruction_fetch = %d\n"
           "protection_key = %d\n"
           "shadow_stack = %d\n"
           "software_guard_extensions = %d\n",
           error.present, error.write, error.user, error.reserved_write,
           error.instruction_fetch, error.protection_key, error.shadow_stack,
           error.software_guard_extensions);
    printf("Press any key to continue\n");
    io_wait_key_raw();
}

void __attribute__((naked)) error_isr_page_fault()
{
    isr_IMPL_INTERRUPT_ERROR(error_isr_page_fault_impl);
}
