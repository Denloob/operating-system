extern int __attribute__((sysv_abi, used)) main(int argc, char **argv);

void __attribute__((naked)) _start()
{
    asm(
        "pop rdi\n"       // `argc` is on the top of the stack
        "mov rsi, rsp\n"  // `argv` comes right after it

        // Align the stack to 16
        "sub rsp, 0x8\n"
        "and rsp, ~0x10\n"

        "call main\n" // call main(argc, argv)

        ".l: jmp .l\n" // TODO: when implemented, call exit syscall
    );
}
