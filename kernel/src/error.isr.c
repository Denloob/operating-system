#include "error.isr.h"
#include "io.h"
#include "isr.h"
#include <stdint.h>

static void __attribute__((used, sysv_abi))
error_isr_general_protection_fault_impl(isr_InterruptFrame *frame, uint64_t error)
{
    printf("\nGeneral protection fault: RIP=0x%llx ; Error Code = 0x%llx\n",
           frame->rip, error);
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
}

void __attribute__((naked)) error_isr_page_fault()
{
    isr_IMPL_INTERRUPT_ERROR(error_isr_page_fault_impl);
}
