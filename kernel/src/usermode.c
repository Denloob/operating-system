#include "usermode.h"
#include <stdint.h>

void usermode_jump_to(void *address, void *stack)
{
    asm volatile (
        "mov r11, %[rflags]\n"
        "mov rsp, %[stack]\n" // This must be before zeroing, so we don't zero the register with the value
        "mov rbp, rsp\n"

        // Zero out all kernel registers (except the ones set later)
        "xor rax, rax\n"
        "xor rbx, rbx\n"
        // skip rcx (which is the `address`)
        "xor rdx, rdx\n"
        "xor rsi, rsi\n"
        "xor rdi, rdi\n"
        // skip rbp and rsp
        "xor r8, r8\n"
        "xor r9, r9\n"
        "xor r10, r10\n"
        // skip r11
        "xor r12, r12\n"
        "xor r13, r13\n"
        "xor r14, r14\n"
        "xor r15, r15\n"

        "sysretq\n"
        :
        : "c"(address), [rflags] "i"(0x202), [stack] "r"(stack)
        : "memory"); // We don't actually need to specify clobbers because it doesn't return. "memory" to force execution order.

    __builtin_unreachable();
}
