#pragma once
#include <stdint.h>

typedef struct
{
    uint64_t rip;
    uint64_t cs;
    uint64_t flags;
    uint64_t rsp;
    uint64_t ss;
} isr_InterruptFrame;

// TODO: store xmm registers
#define PUSH_CALLER_STORED()                                                   \
        asm volatile("                                                         \
            push rax;                                                          \
            push rcx;                                                          \
            push rdx;                                                          \
            push rdi;                                                          \
            push rsi;                                                          \
            push r8;                                                           \
            push r9;                                                           \
            push r10;                                                          \
            push r11;                                                          \
        " : : : "memory")

// TODO: restore xmm registers
#define POP_CALLER_STORED()                                                    \
        asm volatile("                                                         \
            pop r11;                                                           \
            pop r10;                                                           \
            pop r9;                                                            \
            pop r8;                                                            \
            pop rsi;                                                           \
            pop rdi;                                                           \
            pop rdx;                                                           \
            pop rcx;                                                           \
            pop rax;                                                           \
        " : : : "memory")

