#include "FAT16.h"
#include "string.h"
#include "io.h"
#include "smartptr.h"
#include "kernel_memory_info.h"
#include "math.h"
#include "kmalloc.h"
#include "mmap.h"
#include "mmu.h"
#include "regs.h"
#include "syscall.h"
#include "brk.h"
#include "assert.h"
#include <stdint.h>
#include "program.h"
#include "file.h"
#include "fs.h"
#include "res.h"
#include "scheduler.h"
#include "usermode.h"
#include "execve.h"
#include "shell.h"

#define MSR_STAR   0xC0000081
#define MSR_LSTAR  0xC0000082
#define MSR_CSTAR  0xC0000083
#define MSR_SFMASK 0xC0000084
#define MSR_EFER   0xC0000080

#define MAX_ARGV_LEN     128
#define MAX_ARGV_ELEMENT_LEN 512
// Here the RSP of the syscall caller will be stored.
static uint64_t g_ring3_rsp;

static void syscall_exit(Regs *regs)
{
    scheduler_process_dequeue_current_and_context_switch();
    assert(false && "Unreachable");
}

static void syscall_chdir(Regs *regs)
{
    usermode_mem *new_cwd = (usermode_mem *)regs->rdi;

    // TODO: Parse the dir and check if it actually exists

    uint64_t path_length;
    bool valid = usermode_strlen(new_cwd, FS_MAX_FILEPATH_LEN - 1, &path_length);
    if (!valid)
    {
        regs->rax = false; // Return: failed
        return;
    }

    assert(path_length < FS_MAX_FILEPATH_LEN);

    if (path_length == 0)
    {
        regs->rax = false; // Fail
        return;
    }

    char first_char;
    res rs = usermode_copy_from_user(&first_char, new_cwd, 1);
    assert(IS_OK(rs) && "checked before, should be good");

    if (first_char != '/')
    {
        assert(false && "Relative chdir is unsupported"); // TODO: implement relative paths
    }

    PCB *pcb = scheduler_current_pcb();
    rs = usermode_copy_from_user(pcb->cwd, new_cwd, path_length);
    assert(IS_OK(rs) && "checked before, should be good");
    pcb->cwd[path_length] = '\0';
}

static void syscall_getcwd(Regs *regs)
{
    usermode_mem *buf = (usermode_mem *)regs->rdi; 
    size_t size = regs->rsi;

    PCB *pcb = scheduler_current_pcb();
    size_t cwd_length = strlen(pcb->cwd);
    assert(cwd_length < FS_MAX_FILEPATH_LEN);

    size_t bytes_to_copy = MIN(size, cwd_length);
    res rs = usermode_copy_to_user(buf, pcb->cwd, bytes_to_copy);
    if (!IS_OK(rs))
    {
        regs->rax = -1; // Fail
        return;
    }

    regs->rax = bytes_to_copy;
}

static void syscall_reboot(Regs *regs)
{
    syscall_RebootCode code = regs->rdi;
    uint64_t magic1 = regs->rsi;
    uint64_t magic2 = regs->rdx;
    regs->rax = -1; // Return: failed.

    if (magic1 != SYSCALL_REBOOT_MAGIC1 || magic2 != SYSCALL_REBOOT_MAGIC2)
    {
        return;
    }

    switch (code)
    {
        default:
            return;

        case SYSCALL_REBOOT_HALT:
            puts("System halted.");
            cli_hlt();
        case SYSCALL_REBOOT_POWER_OFF:
            puts("Power down.");
            shutdown();
        case SYSCALL_REBOOT_RESTART:
            puts("Restarting system.");
            reboot();
    }

    cli_hlt(); // For good measure.
}

static void syscall_execute_program(Regs *regs)
{
    regs->rax = false; // Return: failed.

    //Args:
    usermode_mem *path_to_file = (usermode_mem *)regs->rdi;
    usermode_mem *usermode_argv = (usermode_mem *)regs->rsi;

    uint64_t path_length;
    bool valid = usermode_strlen(path_to_file, FS_MAX_FILEPATH_LEN - 1, &path_length);
    if (!valid)
    {
        return;
    }

    char filepath[FS_MAX_FILEPATH_LEN] = {0};
    res rs = usermode_copy_from_user(filepath, path_to_file, path_length);
    assert(IS_OK(rs) && "checked before, should be good");

    uint64_t argv_length = 0;
    (void)argv_length; // It's used, but clang doesn't know that.

    char **argv = NULL;
    defer({
        for (int i = 0; i < argv_length; i++)
        {
            kfree(argv[i]);
        }

        kfree(argv);
    });

    if (usermode_argv != NULL)
    {
        uint64_t usermode_argv_length;
        valid = usermode_len(usermode_argv, sizeof(char *), MAX_ARGV_LEN - 1, &usermode_argv_length);
        if (!valid)
        {
            return;
        }

        argv = kcalloc(usermode_argv_length + 1, sizeof(char *));
        if (argv == NULL)
        {
            return;
        }
        argv[usermode_argv_length] = NULL;

        for (int i = 0; i < usermode_argv_length; i++)
        {
            usermode_mem *current_argv_ptr = (usermode_mem *)((uint64_t)usermode_argv + i * sizeof(char *));

            usermode_mem *current_argv = NULL;
            rs = usermode_copy_from_user(&current_argv, current_argv_ptr, sizeof(char *));
            assert(IS_OK(rs) && "checked before, should be good");

            uint64_t current_length;
            valid = usermode_strlen(current_argv, MAX_ARGV_ELEMENT_LEN - 1, &current_length);
            if (!valid)
            {
                return;
            }

            argv[i] = kcalloc(current_length + 1, 1);
            if (argv[i] == NULL)
            {
                return;
            }
            argv_length++;

            rs = usermode_copy_from_user(argv[i], current_argv, current_length);
            assert(IS_OK(rs) && "checked before, should be good");
        }
    }

    rs = execve(filepath, (const char *const *)argv);
    regs->rax = IS_OK(rs);
}

static void syscall_brk(Regs *regs)
{
    //Args:
    const uint64_t addr = regs->rdi;

    PCB *const pcb = scheduler_current_pcb();
    if (is_usermode_address((void *)addr, 1))
    {
        res rs = brk((void *)addr, &pcb->page_break, MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_RING_3);
        (void)rs; // Unused, see NOTES in man 2 brk
    }

    regs->rax = pcb->page_break;
}

static void syscall_read_write(Regs *regs, typeof(fread) fread_fwrite_func)
{
    regs->rax = 0; // Return: Read/Wrote 0 bytes.

    // Args:
    usermode_mem *filepath_user = (usermode_mem *)regs->rdi;
    usermode_mem *out_buffer = (usermode_mem *)regs->rsi;
    uint64_t buffer_size = regs->rdx;

    // Both calculates the length, and makes sure that filepath_user is, in fact,
    //  a null terminated string in usermode memory, smaller than FS_MAX_FILEPATH_LEN.
    uint64_t filepath_len;
    bool valid = usermode_strlen(filepath_user, FS_MAX_FILEPATH_LEN - 1, &filepath_len);
    if (!valid)
    {
        return;
    }

    if (!usermode_is_mapped((uint64_t)out_buffer, (uint64_t)out_buffer + buffer_size))
    {
        return;
    }

    char filepath[FS_MAX_FILEPATH_LEN] = {0};
    res rs = usermode_copy_from_user(filepath, filepath_user, filepath_len);
    assert(IS_OK(rs) && "checked before, should be good");

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
    syscall_read_write(regs, process_fread);
}

static void syscall_write(Regs *regs)
{
    syscall_read_write(regs, process_fwrite);
}

static void __attribute__((used, sysv_abi)) syscall_handler(Regs *user_regs)
{
    user_regs->rsp = g_ring3_rsp; // Doesn't affect the process, just a nice thing for us.

    // NOTE: caller_regs->r11 and caller_regs->rcx contain $RFLAGS and $rip respectively
    //          hence, the kernel probably should not modify them.
    uint64_t original_rip = user_regs->rcx;
    uint64_t original_rflags = user_regs->r11;

    // If we block, we won't use `user_regs`, instead we will context switch to
    //  the pcb. If it happens, you better make sure that there was no data lost.
    PCB *pcb = scheduler_current_pcb();
    pcb->rip = original_rip;
    pcb->regs = *user_regs;
    pcb->regs.rflags = original_rflags;

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
        case SYSCALL_BRK:
            syscall_brk(user_regs);
            break;
        case SYSCALL_EXECUTE:
            syscall_execute_program(user_regs);
            break;
        case SYSCALL_EXIT:
            syscall_exit(user_regs);
            break;
        case SYSCALL_REBOOT:
            syscall_reboot(user_regs);
            break;
        case SYSCALL_GETCWD:
            syscall_getcwd(user_regs);
            break;
        case SYSCALL_CHDIR:
            syscall_chdir(user_regs);
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
