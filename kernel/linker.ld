ENTRY(kernel_main)
OUTPUT_FORMAT("binary")
phys = 0x8000000;
SECTIONS
{
    . = phys;

    .entry       : { __entry_start = .;     *(.entry)   ; __entry_end = .;  }
    .text        : { __text_start = .;      *(.text)    ; __text_end = .;   }
    .data        : { __data_start = .;      *(.data)    ; __data_end = .;   }
    .rodata      : { __rodata_start = .;    *(.rodata)  ; __rodata_end = .; }
    .bss         : { __bss_start = .;       *(.bss)}
    __bss_end = .;
    

    __end = .;
}
