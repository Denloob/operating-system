ENTRY(kernel_main)
OUTPUT_FORMAT("binary")
phys = 0xffffffff80000000;
SECTIONS
{
    . = phys;

    .entry       : { __entry_start = .;     *(.entry)   ; __entry_end = .;  }
    .text        : { __text_start = .;      *(.text)    ; __text_end = .;   }
    .rodata      : ALIGN(0x1000) { __rodata_start = .;    *(.rodata)  ; __rodata_end = .; }
    .data        : ALIGN(0x1000) { __data_start = .;      *(.data)    ; __data_end = .;   }
    .bss         : ALIGN(0x1000) { __bss_start = .;       *(.bss)}
    __bss_end = .;
    

    __end = .;
}
