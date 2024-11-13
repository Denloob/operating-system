#pragma once

#include "drive.h"
#include "gdt.h"

void main_long_mode_jump_to(uint64_t addr, uint64_t stack_base, uint32_t param1, uint32_t param2, uint32_t param3);
void main_gdt_long_mode_init();

extern gdt_entry main_gdt_64bit;
extern gdt_descriptor main_gdt_descriptor;
