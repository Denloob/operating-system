#pragma once

#include <stdint.h>

typedef struct {
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rflags;
    __attribute__((aligned(16))) uint8_t sse[512];
} Regs; // NOTE: change this struct very carefully, as its offsets are hardcoded in inline assembly for usermode jumps and possibly more.

// Push current cpu registers onto the stack to get the full `Regs`. Stack must
// be aligned before executing.
// In the end, `$rsp` will contain the pointer `Regs`.
#define regs_PUSH() asm volatile(                                              \
                "sub rsp, 512\n"                                               \
                "fxsave [rsp]\n"                                               \
                "push 0\n" /* Padding before the SSE (stack aligned) */        \
                                                                               \
                "pushf\n"                                                      \
                "push r15\n"                                                   \
                "push r14\n"                                                   \
                "push r13\n"                                                   \
                "push r12\n"                                                   \
                "push r11\n"                                                   \
                "push r10\n"                                                   \
                "push r9\n"                                                    \
                "push r8\n"                                                    \
                "push rdi\n"                                                   \
                "push rsi\n"                                                   \
                "push rbp\n"                                                   \
                "push 0\n" /* Skip rsp, meaningless*/                          \
                "push rbx\n"                                                   \
                "push rdx\n"                                                   \
                "push rcx\n"                                                   \
                "push rax\n"                                                   \
        ::: "memory")

// pop the `Regs` struct at `$rsp` into the CPU registers.
#define regs_POP() asm volatile(                                               \
                "pop rax\n"                                                    \
                "pop rcx\n"                                                    \
                "pop rdx\n"                                                    \
                "pop rbx\n"                                                    \
                "add rsp, 8\n" /* Skip popping rsp, meaningless */             \
                "pop rbp\n"                                                    \
                "pop rsi\n"                                                    \
                "pop rdi\n"                                                    \
                "pop r8\n"                                                     \
                "pop r9\n"                                                     \
                "pop r10\n"                                                    \
                "pop r11\n"                                                    \
                "pop r12\n"                                                    \
                "pop r13\n"                                                    \
                "pop r14\n"                                                    \
                "pop r15\n"                                                    \
                "popf\n"                                                       \
                                                                               \
                "add rsp, 8\n" /* Padding before the SSE (stack aligned) */    \
                "fxrstor [rsp]\n"                                              \
                "add rsp, 512\n"                                               \
                                                                               \
            ::: "memory")
