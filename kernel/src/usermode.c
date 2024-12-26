#include "usermode.h"
#include "io.h"
#include "macro_utils.h"
#include "mmu.h"
#include "assert.h"
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

bool usermode_is_mapped(uint64_t begin, uint64_t end)
{
    if (end < begin)
    {
         return false;
    }

    if (!is_usermode_address((void *)begin, end - begin))
    {
        return false;
    }

    for (uint64_t it = PAGE_ALIGN_DOWN(begin); it < end; it += PAGE_SIZE)
    {
        mmu_PageTableEntry *page = mmu_page(it);
        if (!page->present || !page->user_supervisor)
        {
            return false;
        }
    }

    return true;
}

bool is_usermode_address(const void *address, const size_t size)
{
    const uint64_t begin = (uint64_t)address;

    uint64_t end;
    const bool overflow = __builtin_add_overflow(begin, size, &end);
    if (overflow) return false;


// Non canonical or invalid address
#define CANONICAL_ADDRESS_LIMIT 0x0000800000000000

    return begin < CANONICAL_ADDRESS_LIMIT && end < CANONICAL_ADDRESS_LIMIT;
}

void usermode_init_smp()
{
#define CPUID_SMP_LEAF 7
#define CPUID_SMP_SUB_LEAF 0
#define CPUID_SMEP (1 << 7)
#define CPUID_SMAP (1 << 20)

#define CPU_CR4_SMEP (1 << 20)
#define CPU_CR4_SMAP (1 << 21)

    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    __cpuid_count(CPUID_SMP_LEAF, CPUID_SMP_SUB_LEAF, eax, ebx, ecx, edx);

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

bool usermode_strlen(const usermode_mem *str, uint64_t max_length, uint64_t *out_len)
{
    *out_len = 0;

    if (max_length == 0 || !usermode_is_mapped((uint64_t)str, (uint64_t)str + 1)) // No need to check for overflow, it's ok if it happens, the function will return false.
    {
        return false;
    }

    const char *it = &str->ch;
    const char *end_of_safe_region = (const char *)PAGE_ALIGN_UP((uint64_t)it + 1); // We know that the memory between `str` and to this address is all usermode and mapped.

    while (max_length > 0)
    {
        assert(it <= end_of_safe_region && "should always be true");

        if (it == end_of_safe_region) // end_of_safe_region is always page aligned
        {
            end_of_safe_region += PAGE_SIZE;
            if (!usermode_is_mapped((uint64_t)it, (uint64_t)end_of_safe_region))
            {
                return false;
            }
        }

        stac();
        bool found = *it == '\0';
        clac();

        if (found)
        {
            return true;
        }

        (*out_len)++;
        it++;

        max_length--;
    }

    return false;
}
