#include "sse.h"

#define CR0_MP         (1 << 1)
#define CR0_EM         (1 << 2)
#define CR0_TS         (1 << 3)
#define CR0_ET         (1 << 4)
#define CR0_NE         (1 << 5)
#define CR4_OSFXSR     (1 << 9)  // 128bit SSE support enabled
#define CR4_OSXMMEXCPT (1 << 10) // XF SSE exception enabled
#define CR4_OSXSAVE    (1 << 18) // SSE state saving (we won't support it)

void sse_init()
{
    asm("mov eax, cr0\n"
        "and eax, %0\n"
        "or eax, %1\n"
        "mov cr0, eax"
        :
        : "i"(~CR0_EM & ~CR0_TS), "i"(CR0_MP | CR0_NE | CR0_ET)
        : "eax", "cc");

    asm("mov eax, cr4\n"
        "or eax, %0\n"
        "mov cr4, eax"
        :
        : "i"(CR4_OSFXSR | CR4_OSXMMEXCPT)
        : "eax", "cc");

    asm("fninit\n");
}
