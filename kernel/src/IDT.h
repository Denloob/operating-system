#pragma once
#include <stdint.h>

typedef struct
{
    uint16_t offset_low;    // Holds the lower 16 bits of the address of the interrupt handler function
    uint16_t segment;       // Represent the segment in which the function located
    uint32_t ist : 3;       // An offset into the Interrupt Stack Table, which is stored in the Task State Segment. If the bits are all set to zero, the Interrupt Stack Table is not used.
    uint32_t reserved_5 : 5;
    uint32_t gate_type : 4; // @see IDT_gate_type
    uint32_t zero : 1;      // Shall be zero
    uint32_t dpl : 2;       // Descriptor privilege level - CPU privilege level AKA the minumum required ring to access the interrupt via IDT
    uint32_t present : 1;   // Holds the flags of the interrupt (check out the IDT_FLAGS enum)
    uint16_t offset_mid;    // Holds the next 16 bits of the function's address after the low offset
    uint32_t offset_high;   // Holds the last 32 bits of the function's address
    uint32_t reserved;      // Must be set to 0 its reserved
} IDTEntry;

typedef enum
{
    IDT_gate_type_INTERRUPT = 0xE,
    IDT_gate_type_TRAP_GATE = 0xF,
} IDT_gate_type;

typedef struct 
{
    uint16_t limit; // Specifies the maximum size of the IDT (IDT_LENGTH * Entry size)
    uint64_t base;  // The base address of the idt
} __attribute__((packed)) IDTDescriptor;

#define IDT_LENGTH 256
extern IDTEntry *g_idt;

/**
 * @brief - Register an IDT entry
 *
 * @param interrupt - The number of the interrupt you wish to register.
 * @param gate_type - The type of the gate (interrupt/trap gate).
 * @param callback - The function to call on the interrupt.
 */
void idt_register(int interrupt, IDT_gate_type gate_type, void *callback);
