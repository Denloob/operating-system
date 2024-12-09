#pragma once

#include "isr.h"
#include "pcb.h"

void schedular_context_switch_to(PCB *pcb);
void schedular_context_switch_from(Regs *regs, isr_InterruptFrame *frame) __attribute__((used, sysv_abi));
