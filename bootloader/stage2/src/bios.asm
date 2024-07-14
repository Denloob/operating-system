; Define a cdecl 32 bit param (assumes a saved ebp)
;
; @params:
;   %1 - The number of the parameter on the stack
;   %2 - The name of the parameter
;
; @example:
;   param32 0, .my_first_param
;   param32 1, .my_second_param
;   mov eax, .my_first_param
%macro param32 2
    %define %2 [ebp + (%1 + 2) * 4]
%endmacro

%macro enter_real_mode 0
    [bits 32]
    jmp word 0x18:.protected_mode16 ; Jump to 16-bit protected mode
.protected_mode16:
    [bits 16]
    ; Unset PE (Protection Enable) bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    jmp word 0x0:.real_mode16 ; Jmp to real mode
.real_mode16:
    ; Setup the segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; Enable interrupts
    sti
%endmacro

%macro enter_protected_mode 0
    [bits 16]
    ; Disable interrupts
    cli

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
%endmacro

; Convert linear address to segment:offset address.
; @params
;    1 - linear address (e.g. 0xabcde)
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - (out) target lower 16-bit half of %3 (e.g. ax)
%macro linear_to_segmented_offset 4
    ; The comments assume the example params

    mov %3, %1      ; eax = 0xabcde
    shr %3, 4       ; eax /= 16
    mov %2, %4      ; es = ax (= 0xabcd)

    mov %3, %1      ; eax = 0xabcde
    and %3, 0xf     ; eax &= 0xf (= 0xe)
%endmacro

global bios_drive_get_drive_params
bios_drive_get_drive_params:
    [bits 32]
    push ebp
    mov ebp, esp

    param32 0, .drive     ; uint8_t
    param32 1, .driveType ; uint8_t *
    param32 2, .cylinders ; uint16_t *
    param32 3, .sectors   ; uint8_t *
    param32 4, .heads     ; uint8_t *

    push ebx    ; Store ebx and edi
    push edi

    mov eax, .drive
    push eax

    enter_real_mode

    pop eax

    mov dl, al  ; Drive number
    mov ah, 0x8 ; Get drive parameters interrupt
    xor di, di  ; es:di = 0x0000:0x0000
    stc
    int 0x13
    jnc .success

    ; Fail
    mov eax, 0  ; return false
    push eax
    jmp .end

.success:
     linear_to_segmented_offset .driveType, es, edi, di ; es:di = .driveType
     mov es:[di], bl ; *.driveType = bl (drive type)

     mov al, cl
     and al, 0x3f ; al = cl [bits 5-0] (number of sectors)

     linear_to_segmented_offset .sectors, es, edi, di ; es:di = .sectors
     mov es:[di], al ; *.sectors = al (number of sectors)

     shr cl, 6   ; cl = high two bits of maximum cylinder number [bits 7-6 of cl]
                 ; ch (set by the interrupt) = low eight bits of maximum cylinder number
     xchg cl, ch ; cx = number of cylinders - 1 (A BIOS thing to return the number-1)
     inc cx      ; cx = number of cylinders
     linear_to_segmented_offset .cylinders, es, edi, di ; es:di = .cylinders
     mov es:[di], cx ; *.cylinders = cx (number of cylinders)

    linear_to_segmented_offset .heads, es, edi, di ; es:di = .heads
    inc dh
    mov es:[di], dh    ; *.heads = dh + 1 (number of heads)

    mov eax, 1  ; return true
    push eax
.end:
    enter_protected_mode

    pop eax     ; get the return value

    pop edi     ; Restore ebx and edi
    pop ebx

    leave
    ret
