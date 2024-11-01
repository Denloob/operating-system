#pragma once

void __attribute__((naked)) error_isr_general_protection_fault();
void __attribute__((naked)) error_isr_page_fault();
