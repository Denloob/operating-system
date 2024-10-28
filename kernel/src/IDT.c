#include "IDT.h"

IDTEntry *g_idt;

#define GDT_SEGMENT_CODE 0x8

void idt_register(int interrupt, IDT_gate_type gate_type, void *callback)
{
    uint64_t callback_addr = (uint64_t)callback;
    g_idt[interrupt] = (IDTEntry){
        .gate_type = gate_type,

        .offset_high = callback_addr >> 32,
        .offset_mid = (callback_addr >> 16) & 0xffff,
        .offset_low = callback_addr & 0xffff,

        .present = 1,
        .segment = GDT_SEGMENT_CODE,
    };
}

