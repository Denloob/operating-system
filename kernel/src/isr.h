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


// Frame for the registers pushed/poped by PUSH_CALLER_STORED/POP_CALLER_STORED
typedef struct {
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
} isr_CallerRegsFrame;

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

/**
 * @brief - Trampolines to the provided function.
 *
 * @param func - pointer to the function to trampoline to. Passed on the stack (first on the stack, aka at [rsp]).
 * @param frame - isr_InterruptFrame from the interrupt. Passed on the stack (second on the stack, aka [rsp+8])
 *
 * @return it will return, and you will have to pop the function pointer from it before you can iretq.
 */
void __attribute__((naked)) isr_trampoline();

/**
 * @brief - Trampolines to the provided function.
 *
 * @param func - pointer to the function to trampoline to. Passed on the stack (first on the stack, aka at [rsp]).
 * @param error - uint64_t of the error code. Passed on the stack (second on the stack, aka [rsp+8])
 * @param frame - isr_InterruptFrame from the interrupt. Passed on the stack (third on the stack, aka [rsp+16])
 *
 * @return it will return, and you will have to pop the function pointer from it, and the error code too before you can iretq.
 */
void __attribute__((naked)) isr_trampoline_error();

#define isr_IMPL_INTERRUPT(c_function)                                         \
    asm volatile ("push offset " # c_function  "\n"                                   \
                  "call isr_trampoline\n"                                      \
                  "add rsp, 0x8\n"                                             \
                  "iretq" : : : "memory", "cc");

#define isr_IMPL_INTERRUPT_ERROR(c_function)                                   \
    asm volatile ("push offset " # c_function  "\n"                                   \
                  "call isr_trampoline_error\n"                                      \
                  "add rsp, 0x10\n"                                            \
                  "iretq" : : : "memory", "cc");
