global main_gdt_64bit
main_gdt_64bit:
    .Null:
    dq 0
    .Code:
    dq 0
    .Data:
    dq 0
    .TSS:
    dq 0
    dq 0

global main_gdt_descriptor
main_gdt_descriptor:
    dw $ - main_gdt_64bit - 1 ; -1 because that's what the docs require (gdt_size - 1)
    dq 0

global main_long_mode_jump_to
main_long_mode_jump_to:
    [bits 32]
    push ebp
    mov ebp, esp

    %define .addr [ebp + 8]

    lgdt [main_gdt_descriptor]         ; Load the 64-bit global descriptor table.
    jmp 0x8:.long_init       ; Set the code segment and enter 64-bit long mode.
.long_init:
    [bits 64]

    mov eax, .addr

    leave
    jmp rax