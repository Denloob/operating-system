bits 16

section .entry

extern __bss_start
extern __end

extern start
global entry

entry:
    [bits 16]
    cli
    ; The stack is set up by the stage1 bootloader.

    ; Pass the boot drive as a parameter to cstart
    mov [g_boot_drive], dx

    ; Enable A20
    call a20_enable

    ; Load the GDT
    lgdt [g_GDT_desc]

    ; Set the 3rd video mode (and by this also clear the screen)
    mov ax, 0x3 ; ah = 0x0, al = 0x3
    int 0x10

    ; Set PE (Protection Enable) bit in cr0
    mov eax, cr0
    or  al, 1
    mov cr0, eax

    ; Jump into protected mode
    jmp dword 0x8:.protected_mode
.protected_mode:
    [bits 32]

    ; Setup the stack, data and all the other (non code) segments to be at the 32bit data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Clear .bss
    mov edi, __bss_start    ; start = __bss_start
    mov ecx, __end
    sub ecx, edi
    shr ecx, 2              ; count = (__end - __bss_start) / 4
    xor eax, eax            ; value = 0
    cld
    rep stosd               ; memset(start, 0, count)

    mov al, [g_boot_drive]
    push eax
    call start  ; start(boot_drive)

.halt
    cli
    hlt
    jmp .halt

g_boot_drive db 0

; ===================================
; -=    Global Descriptor Table    =-
; ===================================

g_GDT:
    [bits 16]
    ; NULL Descriptor
    dq 0

    ; 32bit code segment
    dw 0xFFFF           ; limit (bits 0-15) = 0xFFFF_FFFF (full 32 bit range)
    dw 0                ; base (bits 0-15) = 0
    db 0                ; base (bits 16-23) = 0
    db 0b10011011       ; access (present, ring 0, code segment, allow only ring 0 to execute it, readable)
    db 0b11001111       ; flags (4k-sized granuality, 32-bit protected mode) + limit (bits 16-19)
    db 0                ; base (24-31)

    ; 32bit data segment
    dw 0xFFFF           ; limit (bits 0-15) = 0xFFFF_FFFF (full 32 bit range)
    dw 0                ; base (bits 0-15) = 0
    db 0                ; base (bits 16-23) = 0
    db 0b10010011       ; access (present, ring 0, data segment, direction 0, writable)
    db 0b11001111       ; flags (4k-sized granuality, 32-bit protected mode) + limit (bits 16-19)
    db 0                ; base (24-31)

    ; 16bit code segment
    dw 0xFFFF           ; limit (bits 0-15) = 0xFFFF_FFFF (full 32 bit range)
    dw 0                ; base (bits 0-15) = 0
    db 0                ; base (bits 16-23) = 0
    db 0b10011011       ; access (present, ring 0, code, allow only ring 0 to execute it, readable)
    db 0b00001111       ; flags (1-byte granuality, 16-bit protected mode) + limit (bits 16-19)
    db 0                ; base (24-31)

    ; 16bit data segment
    dw 0xFFFF           ; limit (bits 0-15) = 0xFFFF_FFFF (full 32 bit range)
    dw 0                ; base (bits 0-15) = 0
    db 0                ; base (bits 16-23) = 0
    db 0b10010011       ; access (present, ring 0, data segment, direction 0, writable)
    db 0b00001111       ; flags (1-byte granuality, 16-bit protected mode) + limit (bits 16-19)
    db 0                ; base (24-31)

g_GDT_desc:
    dw g_GDT_desc - g_GDT - 1   ; GDT size - 1
    dd g_GDT                    ; address of GDT


; ===================================
; -= Keyboard Controller (A20 bit) =-
; ===================================

KEYBOARD_CONTROLLER_PORT_DATA                       equ 0x60
KEYBOARD_CONTROLLER_PORT_COMMAND                    equ 0x64

KEYBOARD_CONTROLLER_COMMAND_KEYBOARD_DISABLE        equ 0xad
KEYBOARD_CONTROLLER_COMMAND_KEYBOARD_ENABLE         equ 0xAD
KEYBOARD_CONTROLLER_COMMAND_READ_CTRL_OUTPUT_PORT   equ 0xD0
KEYBOARD_CONTROLLER_COMMAND_WRITE_CTRL_OUTPUT_PORT  equ 0xD1
a20_enable:
    [bits 16]
    call a20_check
    jnz .exit       ; if (a20 is enabled) goto .exit

    mov al, KEYBOARD_CONTROLLER_COMMAND_KEYBOARD_DISABLE
    call a20_command

    ; Read the control output port
    mov al, KEYBOARD_CONTROLLER_COMMAND_READ_CTRL_OUTPUT_PORT
    call a20_command
    call a20_read_output
    push eax ; Store the read value

    ; Write control output port
    mov al, KEYBOARD_CONTROLLER_COMMAND_WRITE_CTRL_OUTPUT_PORT
    call a20_command
    pop eax     ; Restore the read value
    or al, 2    ; Enable the A20 bit
    call a20_wait_input
    out KEYBOARD_CONTROLLER_PORT_DATA, al

    ; Enable the keyboard
    mov al, KEYBOARD_CONTROLLER_COMMAND_KEYBOARD_ENABLE
    call a20_command
.exit:
    ret

; Sends the command in `al` to the keyboard port
a20_command:
    [bits 16]
    call a20_wait_input
    out KEYBOARD_CONTROLLER_PORT_COMMAND, al
    ret

; Read output from the keyboard data port.
; @return
;   al - The read byte
a20_read_output:
    [bits 16]
    call a20_wait_output
    in al, KEYBOARD_CONTROLLER_PORT_DATA
    ret

; Wait until input buffer status bit (bit 2) is 0
a20_wait_input:
    [bits 16]
    push ax

.continue_waiting:
    in al, KEYBOARD_CONTROLLER_PORT_COMMAND

    test al, 2
    jnz .continue_waiting

    pop ax
    ret


; Wait until output buffer status bit (bit 1) is 1 (aka data is ready for reading)
a20_wait_output:
    [bits 16]
    push ax

.continue_waiting:
    in al, KEYBOARD_CONTROLLER_PORT_COMMAND

    test al, 1
    jnz .continue_waiting

    pop ax
    ret

; Check if the a20 pin is enabled
;
; @return ZF (zero flag) is set if the a20 line is disabled and is not set otherwise

a20_check:
    [bits 16]
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax          ; ax = 0
    mov es, ax

    not ax              ; ax = 0xFFFF
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    ; Store the old values at 0x500 and 0x510
    mov al, byte [es:di]
    push ax

    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    ; Restore the stored values
    pop ax
    mov byte [ds:si], al

    pop ax
    mov byte [es:di], al

.exit:
    pop si
    pop di
    pop es
    pop ds

    ret
