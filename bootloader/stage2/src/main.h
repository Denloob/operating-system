#pragma once

#include "drive.h"
#include "gdt.h"

typedef void (*kernel_main)(Drive drive);

void main_long_mode_jump_to(kernel_main addr);

extern gdt_entry *main_gdt_64bit;
extern gdt_descriptor *main_gdt_descriptor;
