#include <stdint.h>
#include <stdio.h>

extern int __attribute__((sysv_abi, used)) main(int argc, char **argv);

static void __attribute__((used)) init_stdio_streams()
{
    stdin  = fopen("/dev/tty", "r");
    stdout = fopen("/dev/tty", "w");
    stderr = fopen("/dev/tty", "w");
}

void __attribute__((naked)) _start()
{
    asm(
        "pop r14\n"       // `argc` is on the top of the stack
        "mov r15, rsp\n"  // `argv` comes right after it

        // Align the stack to 16
        "and rsp, ~0xf\n"

        "call init_stdio_streams\n"

        "mov rdi, r14\n"
        "mov rsi, r15\n"
        "call main\n" // call main(argc, argv)

        "mov rdi, rax\n"
        "call exit\n"
    );
}
