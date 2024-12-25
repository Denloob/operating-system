#include <stdarg.h>
#include <sys/syscall.h>
#include <unistd.h>

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
