global main_gdt_64bit
main_gdt_64bit:
    .Null:
    dq 0
    .Code:
    dq 0
    .Data:
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
    %define .stack_base [ebp + 16]
    %define .param1 [ebp + 24]
    %define .param2 [ebp + 28]
    %define .param3 [ebp + 32]

    lgdt [main_gdt_descriptor]         ; Load the 64-bit global descriptor table.
    jmp 0x8:.long_init       ; Set the code segment and enter 64-bit long mode.
.long_init:
    [bits 64]

    mov rax, .addr
    mov edi, .param1
    mov esi, .param2
    mov edx, .param3

    ; There's no need to execute `leave`, we do not care what will happen to the old stack,
    ; as we will not return here and we overwrite rsp and rbp
    mov rsp, .stack_base
    sub rsp, 8 ; "Align" the stack to 16 bytes - we basically simulate what would happen if this was a call and not a jump, so from function's perspective, after the prologue, the stack is aligned
    mov rbp, rsp

    jmp rax
