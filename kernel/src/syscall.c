#include "io.h"
#include "isr.h"
#include "syscall.h"
#include <stdint.h>

#define MSR_STAR   0xC0000081
#define MSR_LSTAR  0xC0000082
#define MSR_CSTAR  0xC0000083
#define MSR_SFMASK 0xC0000084
#define MSR_EFER   0xC0000080

static void __attribute__((used, sysv_abi)) syscall_handler(isr_CallerRegsFrame *caller_regs)
{
    printf("Hello, Syscall!\n");
}

// Here the RSP of the syscall caller will be stored.
static uint64_t g_ring3_rsp;

static void __attribute__((naked)) syscall_handler_trampoline()
{
    // Store usermode RSP and replace it with kernel stack
    asm volatile("mov %[ring3_rsp], rsp\n"
                 "mov rsp, %[stack_begin]\n"
                 : [ring3_rsp] "+m"(g_ring3_rsp)
                 : [stack_begin] "i"(0x7ffffffff000)
                 : "memory");

    PUSH_CALLER_STORED();

    asm volatile("lea rdi, [rsp]\n"
                 "call syscall_handler\n" ::: "memory");

    POP_CALLER_STORED();

    // Restore RSP
    asm volatile("mov rsp, %[ring3_rsp]\n"
                 : [ring3_rsp] "+m"(g_ring3_rsp)
                 :
                 : "memory");

    asm volatile("sysretq\n");
}

/**
 * @brief Write a 64bit `value` into the MSR at the given `msr_address`
 */
static inline void write_to_64bit_msr(uint64_t msr_address, uint64_t value)
{
    // 32 MSB go into `edx` and 32 LSB go into `eax`.
    asm volatile("wrmsr\n"
                 :
                 : "c"(msr_address),
                   "d"((uint32_t)(value >> 32)), "a"((uint32_t)value));
}

/**
 * @brief Enable syscalls by setting the EFER syscall extension bit.
 */
static inline void enable_syscalls()
{
    asm volatile("rdmsr\n"
                 "or eax, 1\n" // Set System Call Extensions bit (bit 0)
                 "wrmsr\n"
                 :
                 : "c"(MSR_EFER)
                 : "eax", "edx");
}

/**
 * @brief Set the syscall handler in the appropriate MSR.
 */
static void set_syscall_handler()
{
    write_to_64bit_msr(MSR_LSTAR, (uint64_t)syscall_handler_trampoline);
}

/**
 * @brief Set the kernel/usermode Code and Stack segments in the appropriate MSR.
 */
static void set_syscall_segments()
{
    // Set the kernel CS segment register (the SS is derived from CS: SS=CS+8)
    //  and the usermode CS and SS in the similar way.
#define KERNEL_CS 0x8
#define STAR_KERNEL_CS_BIT_START 32
    uint64_t star_cs_value = ((uint64_t)KERNEL_CS << STAR_KERNEL_CS_BIT_START);

    // Userland CS will be loaded from `IA32_STAR[63:48] + 16` and userland SS from `IA32_STAR[63:48] + 8`
    // @see (5.8.8 in the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3A)
#define USERMODE_SS 0x18
#define STAR_USERMODE_SS_BIT_START 48
    star_cs_value |= (uint64_t)(USERMODE_SS - 8) << STAR_USERMODE_SS_BIT_START; // -8 to offset the SS value as specified by the CPU manual

    write_to_64bit_msr(MSR_STAR, star_cs_value);
}

/**
 * @brief Set the syscall FLAGS register mask in the appropriate MSR.
 */
static void set_syscall_flags_mask()
{
    #define INTERRUPT_FLAG 0x200
    #define RESUME_FLAG    0x10000
    write_to_64bit_msr(MSR_SFMASK, ~(INTERRUPT_FLAG | RESUME_FLAG)); // Mask out the specified flags
}

void syscall_initialize()
{
    set_syscall_handler();
    set_syscall_segments();
    set_syscall_flags_mask();

    enable_syscalls();
}
