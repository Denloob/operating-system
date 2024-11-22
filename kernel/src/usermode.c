#include "usermode.h"

void usermode_jump_to(void *address, void *stack)
{
    asm(
        // Set the segment registers to segment 0x20 as usermode (data usermode segment)
        "mov rax, 0x20 | 3 \n"
        "mov ds, ax\n"
        "mov es, ax\n"
        "mov fs, ax\n"
        "mov gs, ax\n"

        // Build in iretq frame
        "push rax\n"        // SS
        "push %[stack]\n"   // rsp
        "pushf\n"           // FLAGS
        "push 0x18 | 3\n"   // CS
        "push %[address]\n" // rip
        "iretq\n"
        :
        : [address] "D"(address), [stack] "S"(stack) // Put them in rdi and rsi
        : "memory", "rax");

    __builtin_unreachable();
}
