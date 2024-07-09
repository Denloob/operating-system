bits 16

section _ENTRY class=CODE

extern _cstart_
global entry

entry:
    ; The stack is set up by the stage1 bootloader.

    ; Pass the boot drive as a parameter to cstart
    xor dh, dh
    push dx

    call _cstart_
.halt:
    cli
    hlt
    jmp .halt
