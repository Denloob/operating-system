#include "usermode.h"
#include <stdint.h>

void usermode_jump_to(void *address, void *stack)
{
    asm volatile (
        "mov r11, %[rflags]\n"
        "mov rsp, %[stack]\n"
        "mov rbp, rsp\n"

        "sysretq\n"
        :
        : "c"(address), [rflags] "i"(0x202), [stack] "r"(stack)
        : "memory"); // We don't actually need to specify clobbers because it doesn't return. "memory" to force execution order.

    __builtin_unreachable();
}
