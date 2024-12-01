#include "isr.h"

void __attribute__((naked)) isr_trampoline()
{
 
    asm volatile ("push rbp\n" 
                  "mov rbp, rsp" : : : "memory"); 
    PUSH_CALLER_STORED(); 
 
    asm volatile ( 
        "lea rdi, [rbp + 24]\n" 
        "call [rbp + 16]" : : : "memory"); 
 
    POP_CALLER_STORED(); 
    asm volatile ("leave\n" 
                  "ret" : : : "memory"); 
}

void __attribute__((naked)) isr_trampoline_error()
{
 
    asm volatile ("push rbp\n" 
                  "mov rbp, rsp" : : : "memory"); 
    PUSH_CALLER_STORED(); 
 
    asm volatile ( 
        "lea rdi, [rbp + 32]\n" 
        "mov rsi, [rbp + 24]\n" 
        "call [rbp + 16]" : : : "memory"); 
 
    POP_CALLER_STORED(); 
    asm volatile ("leave\n" 
                  "ret" : : : "memory"); 
}
