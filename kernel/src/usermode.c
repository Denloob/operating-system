#include "usermode.h"
#include "macro_utils.h"
#include "range.h"
#include "memory.h"
#include "regs.h"
#include <stdint.h>
#include <cpuid.h>
#include <stdbool.h>
#include <stddef.h>
#include "res.h"

void usermode_jump_to(void *address, const Regs *regs)
{
#define USERMODE_CS 0x20 | 3
#define USERMODE_DS 0x18 | 3
    asm volatile (
        // Prepare the iretq stack frame
        "push " STR(USERMODE_DS) "\n"
        "push %[stack]\n"
        "push %[rflags]\n"
        "push " STR(USERMODE_CS) "\n"
        "push %[address]\n"

        // Set the regs
        "mov rax, [ %[regs] + 0   ]\n"
        "mov rcx, [ %[regs] + 8   ]\n"
        "mov rdx, [ %[regs] + 16  ]\n"
        "mov rbx, [ %[regs] + 24  ]\n"
        // Skip $rsp, we set it before (%[stack])
        "mov rbp, [ %[regs] + 40  ]\n"
        "mov rsi, [ %[regs] + 48  ]\n"
        // Skip $rdi because %[regs] is stored in $rdi, so we will load it last
        "mov r8,  [ %[regs] + 64  ]\n"
        "mov r9,  [ %[regs] + 72  ]\n"
        "mov r10, [ %[regs] + 80  ]\n"
        "mov r11, [ %[regs] + 88  ]\n"
        "mov r12, [ %[regs] + 96  ]\n"
        "mov r13, [ %[regs] + 104 ]\n"
        "mov r14, [ %[regs] + 112 ]\n"
        "mov r15, [ %[regs] + 120 ]\n"

        "fxrstor  [ %[regs] + 144 ]\n" // SSE

        "mov rdi, [ %[regs] + 56  ]\n"

        // Can not use %[regs] any longer, we overwrote its value.

        // Setup the segments - must be after we read `regs`, as we change the `ds`.
        "push rax\n" // Store the user rax
        "mov ax, " STR(USERMODE_DS) "\n"
        "mov ds, ax\n"
        "mov es, ax\n"
        "mov fs, ax\n"
        "mov gs, ax\n"
        "pop rax\n" // Restore the user rax

        "iretq\n"
        :
        : [address] "r"(address), [rflags] "r"(regs->rflags), [stack] "r"(regs->rsp),
          [regs] "D"(regs)
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

void usermode_init_smp()
{
#define CPUID_SMP_LEAF 7
#define CPUID_SMEP (1 << 7)
#define CPUID_SMAP (1 << 20)

#define CPU_CR4_SMEP (1 << 20)
#define CPU_CR4_SMAP (1 << 21)

    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx = 0;
    uint32_t edx;

    __cpuid(CPUID_SMP_LEAF, eax, ebx, ecx, edx);

    uint64_t new_flags = 0;

    if (ebx & CPUID_SMEP)
    {
        new_flags |= CPU_CR4_SMEP;
    }

    if (ebx & CPUID_SMAP)
    {
        new_flags |= CPU_CR4_SMAP;
    }

    asm("mov rax, cr4\n"
        "or rax, %0\n"
        "mov cr4, rax"
        :
        : "m"(new_flags)
        : "rax", "cc");
}

static inline void stac()
{
    asm volatile("stac" ::: "memory");
}

static inline void clac()
{
    asm volatile("clac" ::: "memory");
}

// This struct is intentionally defined in the end of the file, to prevent any unintended use of usermode_mem directly
struct usermode_mem {
    char ch;
};

res usermode_copy_from_user(void *to, const usermode_mem *from, size_t len)
{
    // TODO: check that the pages actually exist
    if (!is_usermode_address((void *)from, len))
    {
        return res_usermode_NOT_USERMODE_ADDRESS; 
    }

    stac();

    memmove(to, from, len);

    clac();

    return res_OK;
}
