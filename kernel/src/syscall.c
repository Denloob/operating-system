#include "FAT16.h"
#include "kernel_memory_info.h"
#include "math.h"
#include "mmu.h"
#include "regs.h"
#include "syscall.h"
#include "assert.h"
#include <filesystem>
#include <stdint.h>
#include "program.h"
#include "file.h"
#include "fs.h"
#include "res.h"
#include "usermode.h"

#define MSR_STAR   0xC0000081
#define MSR_LSTAR  0xC0000082
#define MSR_CSTAR  0xC0000083
#define MSR_SFMASK 0xC0000084
#define MSR_EFER   0xC0000080

#define MAX_FILEPATH_LEN 512
// Here the RSP of the syscall caller will be stored.
static uint64_t g_ring3_rsp;

static void syscall_execute_program(Regs *regs)
{
    //Args:
    uint64_t id = regs->rdi;
    uint64_t parent_id = regs->rsi;
    usermode_mem *path_to_file = (usermode_mem *)regs->rdx;
    uint64_t path_lenght = regs->r10;

    if (!usermode_is_mapped((uint64_t)path_to_file, (uint64_t)path_to_file + path_lenght))
    {
        return;
    }
    
    char filepath[MAX_FILEPATH_LEN] = {0};
    
    usermode_copy_from_user(filepath, path_to_file, MAX_FILEPATH_LEN-1);

    res result = program_setup(id , NULL , g_pml4 , &g_fs_fat16 , filepath);

    regs->rax = IS_OK(result);
}
static void syscall_read_write(Regs *regs, typeof(fread) fread_fwrite_func)
{

    // Args:
    usermode_mem *filepath_user = (usermode_mem *)regs->rdi;
    uint64_t filepath_len = regs->rsi;
    usermode_mem *out_buffer = (usermode_mem *)regs->rdx;
    uint64_t buffer_size = regs->r10;

    regs->rax = 0; // Read/Wrote 0 bytes.

    if (filepath_len >= MAX_FILEPATH_LEN)
    {
        return;
    }

    if (!usermode_is_mapped((uint64_t)filepath_user, (uint64_t)filepath_user + filepath_len))
    {
        return;
    }

    if (!usermode_is_mapped((uint64_t)out_buffer, (uint64_t)out_buffer + buffer_size))
    {
        return;
    }

    char filepath[MAX_FILEPATH_LEN] = {0};
    usermode_copy_from_user(filepath, (void *)regs->rdi, MIN(regs->rsi, sizeof(filepath) - 1));

    FILE file = {0};
    bool success = fat16_open(&g_fs_fat16, filepath, &file.file); // TODO: when we support `fd`s, that's how we will get the file handle. For now, this also works
    if (!success)
    {
        return;
    }

    asm volatile("stac" ::: "memory"); // 
    regs->rax = fread_fwrite_func(out_buffer, 1, buffer_size, &file);
    asm volatile("clac" ::: "memory");
}

static void syscall_read(Regs *regs)
{
    syscall_read_write(regs, fread);
}

static void syscall_write(Regs *regs)
{
    syscall_read_write(regs, fwrite);
}

static void __attribute__((used, sysv_abi)) syscall_handler(Regs *user_regs)
{
    user_regs->rsp = g_ring3_rsp; // Doesn't affect the process, just a nice thing for us.

    // NOTE: caller_regs->r11 and caller_regs->rcx contain $RFLAGS and $rip respectively
    //          hence, the kernel probably should not modify them.
    uint64_t original_rip = user_regs->rcx;
    uint64_t original_rflags = user_regs->r11;

    /* Calling convention:
     *     Syscall Number:  $rax
     *     Return Value:    $rax
     *     Argument 0:      $rdi
     *     Argument 1:      $rsi
     *     Argument 2:      $rdx
     *     Argument 3:      $r10
     *     Argument 4:      $r8
     *     Argument 5:      $r9
     */

    syscall_Number syscall_number = user_regs->rax;
    switch (syscall_number)
    {
        case SYSCALL_READ:
            syscall_read(user_regs);
            break;
        case SYSCALL_WRITE:
            syscall_write(user_regs);
            break;
        case SYSCALL_EXECUTE:
            syscall_execute_program(user_regs);
            break;
    }

    assert(original_rip == user_regs->rcx && original_rflags == user_regs->r11 && "The syscall is not expected to modify process $rip or $RFLAGS");
}

static void __attribute__((naked)) syscall_handler_trampoline()
{
    // Store usermode RSP and replace it with kernel stack
    asm volatile("mov %[ring3_rsp], rsp\n"
                 "mov rsp, %[stack_begin]\n"
                 : [ring3_rsp] "+m"(g_ring3_rsp)
                 : [stack_begin] "i"(KERNEL_STACK_BASE)
                 : "memory");

    // KERNEL_STACK_BASE is aligned, so we don't need to worry about manually re-aligning the stack :)

    regs_PUSH();

    asm volatile("lea rdi, [rsp]\n"
                 "call syscall_handler\n" ::: "memory");

    regs_POP();

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
