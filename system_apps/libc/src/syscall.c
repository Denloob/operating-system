#include <stdarg.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <stdbool.h>

static long __attribute__((sysv_abi, naked)) do_syscall(long syscall_number, long a0, long a1, long a2, long a3, long a4, long a5)
{
    asm(

        "mov rax, rdi\n" // rax = syscall_number
        "mov rdi, rsi\n" // Put the params in the syscall regs
        "mov rsi, rdx\n"
        "mov rdx, rcx\n"
        "mov r10, r8 \n"
        "mov r8,  r9 \n"
        "mov r9,  [rsp - 8]\n"

        "syscall\n"

        "ret\n"
    );
}

long syscall(long syscall_number, ...)
{
    va_list args;
    va_start(args, syscall_number);
    long int a0 = va_arg(args, long int); // It's ok if we read more than given, if they shouldn't have, the syscall won't use them.
    long int a1 = va_arg(args, long int);
    long int a2 = va_arg(args, long int);
    long int a3 = va_arg(args, long int);
    long int a4 = va_arg(args, long int);
    long int a5 = va_arg(args, long int);
    va_end(args);

    return do_syscall(syscall_number, a0, a1, a2, a3, a4, a5);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    return syscall(SYS_write, fd, buf, count);
}

ssize_t read(int fd, void *buf, size_t count)
{
    return syscall(SYS_read, fd, buf, count);
}

int brk(void *addr)
{
    void *new_page_break = (void *)syscall(SYS_brk, addr);

    return new_page_break == addr ? 0 : -1;
}

void *sbrk(intptr_t increment)
{
    void *prev_brk = (void *)syscall(SYS_brk, 0);

    uintptr_t new_break;
    if (__builtin_add_overflow((uintptr_t)prev_brk, increment, &new_break))
    {
        return (void *)-1;
    }

    if (brk((void *)new_break) < 0)
    {
        return (void *)-1;
    }

    return prev_brk;
}

int execve_new(const char *path, char *const *argv)
{
    return syscall(SYS_execve, path, argv) ? 0 : -1;
}

void exit(int status)
{
    syscall(SYS_exit, status);
    __builtin_unreachable();
}

int reboot(int op)
{
    return syscall(SYS_reboot, op, RB_SYS_MAGIC1, RB_SYS_MAGIC2);
}

char *getcwd(char *buf, size_t size)
{
    long ret = syscall(SYS_getcwd, buf, size);
    if (ret < 0)
    {
        return NULL;
    }

    return buf;
}

int chdir(const char *path)
{
    return syscall(SYS_chdir, path) ? 0 : -1;
}

pid_t waitpid(pid_t pid, int *wstatus, int options)
{
    return syscall(SYS_waitpid, pid, wstatus, options);
}

int open(const char *pathname, int flags)
{
    return syscall(SYS_open, pathname, flags);
}
