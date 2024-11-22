#include "usermode.h"
#include "range.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void usermode_jump_to(void *address, void *stack)
{
    asm volatile (
        "mov r11, %[rflags]\n"
        "mov rsp, %[stack]\n" // This must be before zeroing, so we don't zero the register with the value
        "mov rbp, rsp\n"

        // Zero out all kernel registers (except the ones set later)
        "xor rax, rax\n"
        "xor rbx, rbx\n"
        // skip rcx (which is the `address`)
        "xor rdx, rdx\n"
        "xor rsi, rsi\n"
        "xor rdi, rdi\n"
        // skip rbp and rsp
        "xor r8, r8\n"
        "xor r9, r9\n"
        "xor r10, r10\n"
        // skip r11
        "xor r12, r12\n"
        "xor r13, r13\n"
        "xor r14, r14\n"
        "xor r15, r15\n"

        "sysretq\n"
        :
        : "c"(address), [rflags] "i"(0x202), [stack] "r"(stack)
        : "memory"); // We don't actually need to specify clobbers because it doesn't return. "memory" to force execution order.

    __builtin_unreachable();
}

// The mmu structures are located in the lower address space, so we need to manually
// check if the given address touches the mmu.
static range_Range mmu_address;

bool is_usermode_address(void *address, size_t size)
{
    void *begin = address;
    uint64_t end_u64;
    bool overflow = __builtin_add_overflow((uint64_t)address, size, &end_u64);
    if (overflow) return false;

    void *end = (void *)end_u64;

    // Non canonical or invalid address
    #define CANONICAL_ADDRESS_LIMIT 0x0000800000000000
    if ((uint64_t)address > CANONICAL_ADDRESS_LIMIT || end_u64 > CANONICAL_ADDRESS_LIMIT) return false;

    void *mmu_begin = (void *)mmu_address.begin;
    void *mmu_end = mmu_begin + mmu_address.size;
    if (begin < mmu_end && mmu_begin < end) return false; // overlaps with the MMU

    return true;
}

void usermode_init_address_check(uint64_t mmu_map_base_address, uint64_t mmu_map_size)
{
    mmu_address.begin = mmu_map_base_address;
    mmu_address.size = mmu_map_size;
}
