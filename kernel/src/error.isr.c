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
