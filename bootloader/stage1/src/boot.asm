org 0x7C00
bits 16

_start:
    ; Setup all the segments to be `0`
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov sp, 0x7c00 ; The stack will grow down from where we are

    mov byte [drive_number], dl ; The BIOS sets the drive number in dl
    call get_drive_params       ; Query the BIOS for number_of_heads and sectors_per_track

    mov ax, 1                   ; LBA=1, that is the second sector
    mov cl, 12                  ; Read twelve sectors
    mov bx, 0x500               ; Load into the higher half of the address space
    call read_drive
    jmp 0x500                   ; Transfer execution to the loaded module

; Completely halt execution
halt:
    cli
    hlt
    jmp halt

; A function to call when the drive fails
; Will print a message and reboot after a key press
drive_fail:
    mov si, drive_fail_msg
    call puts

    mov ah, 0
    int 16h                       ; wait for a key press

    jmp 0xFFFF:0                ; jump to beginning of BIOS, should reboot


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

; Gets the drive params
;
; @params   drive_number
; @returns  number_of_heads, sectors_per_track
get_drive_params:
    push di
    push ax
    push bx
    push di
    push cx
    push dx
    push es

    mov dl, [drive_number]
    mov ah, 0x8 ; Get drive parameters interrupt
    xor di, di  ; es:di = 0x0000:0x0000
    stc
    int 0x13
    jc drive_fail

    xor dl, dl
    inc dh
    xchg dh, dl
    mov [number_of_heads], dx

    xor ch, ch
    and cl, 0x3f
    mov [sectors_per_track], cx

    pop es
    pop dx
    pop cx
    pop di
    pop bx
    pop ax
    pop di
    ret

; Converts an LBA address to CHS
;
; @params:
;   ax              - LBA address
;
; @return:
;   cx [bits 0-5]   - sector number
;   cx [bits 6-15]  - cylinder
;   dh              - head
;
; @see https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h)#CHS
lba_to_chs:
    push ax              ; Store registers
    push dx

    xor cx, cx           ; cx = 0 (for return)
    xor dx, dx           ; dx = 0 (for div)

    div word [sectors_per_track] ; ax = LBA / sectors_per_track (= temp)
    inc dx               ; dx = (LBA % sectors_per_track) + 1

    and dx, 0b111_11     ; Get only the 5 bits (Better safe than sorry)
    mov cx, dx           ; sector = (LBA % (Sectors per Track)) + 1

    xor dx, dx           ; dx = 0 (for div)
    div word [number_of_heads]  ; ax = temp / number_of_heads (= cylinder)
                         ; dx = temp % number_of_heads (= head)

    mov dh, dl           ; head = (LBA / sectors_per_track) % number_of_heads

    ;
    ; CX =       ---CH--- ---CL---
    ; cylinder : 76543210 98
    ; sector   :            543210
    ;
    ; @source https://en.wikipedia.org/wiki/INT_13H#INT_13h_AH=02h:_Read_Sectors_From_Drive

    mov ch, al           ; ch = cylinder & 0xff
    shl ah, 6
    or  cl, ah           ; cl = sector | ((cylinder 0xFF00) << 6)


    pop ax
    mov dl, al           ; Restore dl

    pop ax               ; Restore ax

    ret

; Reset the drive controller
;
; @params:
;  dl: drive number
;
drive_reset:
    pusha

    stc             ; Set the carry flag (will be used to check for errors)
    xor ah, ah
    int 13h         ; Reset the drive
    jc drive_fail  ; If failed, jump to drive_fail

    popa
    ret

; Reads an address using an LBA address
;
; @params:
;   ax      - LBA address
;   cl      - number of sectors to read (up to 120)
;   dl      - the number of the drive
;   es:[bx] - where to store the read data
;
read_drive:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx         ; Save `cl` (number of sectors to read)
    call lba_to_chs ; call lba_to_chs(ax)
    pop  ax         ; al = number of sectors to read

    mov ah, 0x2     ; interrupt

    mov di, 3       ; Retry the read 3 times

.retry:
    pusha           ; Store all registers before call to BIOS
    stc             ; Set the carry flag (indicates if the read succeeded)
    int 13h

    popa            ; Restore
    jnc .done       ; Break if the interrupt succeeded (by checking the carry flag)

                    ; Read failed, so
    call drive_reset ; Reset the drive

    dec di
    test di, di
    jnz .retry      ; if (--di != 0) goto .retry
                    ; else goto .fail
.fail:
    ; All the attempts failed
    jmp drive_fail

.done:
    pop di
    pop dx
    pop cx
    pop bx
    pop ax

    ret

number_of_heads   dw 0
sectors_per_track dw 0
drive_number      db 0

%define ENDL 0x0D, 0x0A

drive_fail_msg db "Drive failed. Press any key to reboot", ENDL, 0

times 510-($-$$) db 0
db 0x55
db 0xAA
