#include "isr.h"

void __attribute__((naked)) isr_trampoline()
{
 
    asm volatile ("push rbp\n" 
                  "mov rbp, rsp\n"
                  // 16 byte align the stack
                  "and rsp, ~0xf\n" : : : "memory"); 
    PUSH_CALLER_STORED(); 
    STORE_SSE();
 
    asm volatile ( 
        "lea rdi, [rbp + 24]\n" 
        "call [rbp + 16]" : : : "memory"); 

    RESTORE_SSE();
    POP_CALLER_STORED(); 
    asm volatile ("leave\n" 
                  "ret" : : : "memory"); 
}

void __attribute__((naked)) isr_trampoline_error()
{
 
    asm volatile ("push rbp\n" 
                  "mov rbp, rsp\n"
                  // 16 byte align the stack
                  "and rsp, ~0xf\n" : : : "memory"); 
    PUSH_CALLER_STORED(); 
    STORE_SSE(); // Modifies RAX, must be after the push
 
    asm volatile ( 
        "lea rdi, [rbp + 32]\n" 
        "mov rsi, [rbp + 24]\n" 
        "call [rbp + 16]" : : : "memory"); 
 
    RESTORE_SSE();
    POP_CALLER_STORED(); 
    asm volatile ("leave\n" 
                  "ret" : : : "memory"); 
}
