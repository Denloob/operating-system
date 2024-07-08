org 0x7C00
bits 16

_start:
    ; Setup all the segments to be `0`
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov sp, 0x7C00 ; The stack will grow down from where we are

    mov ah, 0x0E
    xor bh, bh
    mov si, msg

.loop:
    lodsb
    test al, al
    jz .halt
    int 0x10
    jmp .loop

.halt:
    hlt
    jmp .halt

msg db "Hello, World!"

times 510-($-$$) db 0
db 0x55
db 0xAA
