org 0xC000
bits 16

_start:
    mov si, msg
    call puts
    jmp halt

; Completely halt execution
halt:
    cli
    hlt
    jmp halt

; Prints a null terminated string in ds:[si] followed by a ENDL
;
; @params
;   si  - The address of the null terminated string to print
puts:
    push si
    push ax
    push bx

    mov ah, 0x0E    ; Prepare for the 0x10/0xE interrupt
    xor bh, bh

.loop:              ; while (1) {
    lodsb           ;   al = ds:[si] ; si++
    test al, al     ;   if (al == '\0')
    jz .end         ;       break
    int 0x10        ;   putc(al)
    jmp .loop       ; }

    mov al, 0x20
    int 0x10        ; putc('\n')

.end:
    pop bx
    pop ax
    pop si

    ret

%define ENDL 0x0D, 0x0A

msg db "Hello, World Stage Two!!!", ENDL, 0
