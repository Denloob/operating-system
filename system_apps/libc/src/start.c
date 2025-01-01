#include <stdint.h>
#include <stdio.h>

extern int __attribute__((sysv_abi, used)) main(int argc, char **argv);

static void __attribute__((used)) init_stdio_streams()
{
    stdin  = fopen("tty.dev");
    stdout = fopen("tty.dev");
    stderr = fopen("tty.dev");
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

        ".l: jmp .l\n" // TODO: when implemented, call exit syscall
    );
}
