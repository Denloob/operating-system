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

    %macro un_param 0
        %undef .drive
        %undef .driveType
        %undef .cylinders
        %undef .sectors
        %undef .heads
    %endmacro

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

    un_param
    %undef un_param

    leave
    ret

; Loads the drive_CHS struct into the correct registers
; @params:
;   es:di - The pointer to struct drive_CHS
; @return
;   cx [bits 0-5]   - sector number
;   cx [bits 6-15]  - cylinder
;   dh              - head
load_chs_into_registers:
    [bits 16]
%define .drive_CHS.cylinder es:[di]
%define .drive_CHS.head es:[di + 2]
%define .drive_CHS.sector es:[di + 3]
    push ax

    mov dh, .drive_CHS.head
    mov cl, .drive_CHS.sector

    ;
    ; CX =       ---CH--- ---CL---
    ; cylinder : 76543210 98
    ; sector   :            543210
    ;
    ; @source https://en.wikipedia.org/wiki/INT_13H#INT_13h_AH=02h:_Read_Sectors_From_Drive

    mov ax, .drive_CHS.cylinder
    mov ch, al
    shl ah, 6
    or  cl, ah

    pop ax
    ret

; Reset the drive controller
;
; @params:
;  dl: drive number
;
drive_reset:
    [bits 16]
    pusha

    stc             ; Set the carry flag (will be used to check for errors)
    xor ah, ah
    int 13h         ; Reset the drive

    popa
    ret

global bios_drive_read
bios_drive_read:
    [bits 32]
    push ebp
    mov ebp, esp

    param32 0, .drive           ; uint8_t
    param32 1, .chs             ; drive_CHS *
    param32 2, .buffer          ; uint8_t *
    param32 3, .sector_count    ; uint8_t

    %macro un_param 0
        %undef .drive
        %undef .chs
        %undef .buffer
        %undef .sector_count
    %endmacro

    push edi                ; Store registers
    push ebx

    mov al, .drive
    push eax

    enter_real_mode

    pop edx                 ; dl = .drive

    linear_to_segmented_offset .chs, es, edi, di  ; es:di = .chs
    call load_chs_into_registers    ; Load the .chs struct into cx and dh for the interrupt

    lea eax, .sector_count
    linear_to_segmented_offset eax, es, edi, di ; es:di = &.sector_count
    mov al, es:[di] ; al = .sector_count

    linear_to_segmented_offset .buffer, es, ebx, bx ; es:di = .buffer

    mov ah, 0x2     ; interrupt code


    mov di, 3       ; Retry the read 3 times

.retry:
    pusha           ; Store all registers before call to BIOS
    stc             ; Set the carry flag (indicates if the read succeeded)
    int 0x13
    popa            ; Restore

    jnc .success

    call drive_reset
    jc .fail        ; if carry is set, drive_reset failed, and then there's no point in retrying

    dec di
    test di, di
    jnz .retry      ; if (--di != 0) goto .retry
                    ; else goto .fail
.fail:
    xor eax, eax  ; return false
    push eax
    jmp .end

.success:

    mov eax, 1  ; return true
    push eax
.end:
    enter_protected_mode

    pop eax     ; get the return value

    pop ebx     ; Restore registers
    pop edi

    un_param
    %undef un_param

    leave
    ret

global bios_memory_get_mem_map
bios_memory_get_mem_map:
    [bits 32]
    push ebp
    mov ebp, esp

    param32 0, .res_ptr         ; bios_memory_MapEntry *
    param32 1, .state_ptr       ; bios_memory_MapRequestState  *

    %macro un_param 0
        %undef .res_ptr
        %undef .state_ptr
    %endmacro

    push ebx            ; Store registers
    push edi
    push esi

    enter_real_mode

.state.entry_id            equ 0x0
.state.done                equ 0x4
.state.is_extended         equ 0x5
.res.attr_bitfield       equ 0x14
    linear_to_segmented_offset .state_ptr, es, esi, si ; es:si = .state_ptr

    mov ebx, es:[si + .state.entry_id] ; ebx = .state_ptr->entry_id;

.magic_bios_canary equ 0x534D4150
    mov edx, .magic_bios_canary
    mov eax, 0xe820
    mov ecx, 24

    linear_to_segmented_offset .res_ptr, es, edi, di    ; es:di = .res_ptr
    mov dword es:[di+.res.attr_bitfield], 1             ; .res_ptr->attr_bitfield = 1 // For ACPI 3.X compatibility

    int 0x15
    xchg bx,bx

    setc dl                                             ; Store the cary flag in dl

    cmp eax, .magic_bios_canary                         ; if eax doesn't match the canary, interrupt failed
    jnz .fail

    linear_to_segmented_offset .state_ptr, es, esi, si  ; es:si = .state_ptr
    mov es:[si+.state.done], dl                         ; .state_ptr->done = cary_bit (if it's 1 - we are done)

    sub cl, 20
    mov es:[si+.state.is_extended], cl                  ; .state_ptr->is_extended = (bios-bytes-written - 20)
    
    mov es:[si+.state.entry_id], ebx                    ; .state_ptr->entry_id = (next-entry-id)

    test ebx, ebx                                        ; if (entry_id == 0) {
    setz es:[si+.state.done]                            ;    state_ptr->done = true; 
                                                        ; }

    push 1                                              ; return_value = true;
    jmp .end

.fail:
    push 0                                              ; return_value = false;

.end:
    enter_protected_mode

    pop eax                                             ; eax = return_value

    pop esi                                             ; Restore registers
    pop edi
    pop ebx

    un_param
    %undef un_param

    leave
    ret
