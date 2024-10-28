#pragma once
#include <stdint.h>

#define IDT_MAX_ENTRIES 256

typedef struct
{
    uint16_t offset_low; // Holds the lower 16 bits of the address of the interrupt handler function
    uint16_t selector;   // Represent the segment in which the function located
    uint8_t zero;        // It has 0 meaning but it must be 0
    uint8_t flags;       // Holds the flags of the interrupt (check out the IDT_FLAGS enum)
    uint16_t offset_mid; // Holds the next 16 bits of the function's address after the low offset
    uint32_t offset_high;// Holds the last 32 bits of the function's address
    uint32_t reserved;   // Must be set to 0 its reserved
} __attribute__((packed)) IDTEntry;


typedef struct 
{
    uint16_t limit; // Specifies the maximum size of the IDT (IDT_MAX_ENTRIES * Entry size)
    uint64_t base;  // The base address of the idt
} __attribute__((packed)) IDTDescriptor;

// Represent the IDTEntry flags 
typedef enum
{
    IDT_FLAG_GATE_TASK   = 0x5,  // A task gate that is used to perform context switching
    IDT_FLAG_GATE_INT16  = 0x5,  // A 16-bit interrupt gate , for handling 16-bit mode interrupts 
    IDT_FLAG_GATE_TRAP16 = 0x7,  // A 16-bit trap gate , trap gate is used for exceptions when triggered it does not disable further interrupts
    IDT_FLAG_GATE_INT32  = 0xE,  // same thing as the 16 one just for protected mode
    IDT_FLAG_GATE_TRAP32 = 0xF,  // same thing as the 16 one just for protected mode 
    IDT_FLAG_RING0       = 0x00, // sets the privilege level to 0 meaning it can only be accessed from kernel mode
    IDT_FLAG_RING3       = 0x60, // sets the privilege level to 3 meaning it can also be accessed by user mode
    IDT_FLAGS_PRESENT    = 0x80, // indicates whether or not the interrupt gate is in the memory , if not it will result in a general protection fault

} IDT_FLAGS;

extern IDTEntry idt[IDT_MAX_ENTRIES]; // will be in kernel main.c
extern IDTDescriptor idt_descriptor;  // will be in kernel main.c

void IDT_set_gate(int num, uint64_t base, uint16_t selector, uint8_t flags);
void IDT_load(void);
void IDT_enable_gate(int num);
void IDT_disable_gate(int num);
void IDT_initialize(void);

extern void idt_flush(uint64_t idt_ptr_address); // Needs to be an ass implementation
