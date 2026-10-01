ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    __kernel_start = .;

    .text : ALIGN(16) { KEEP(*(.text.boot)) *(.text*) }
    .rodata : ALIGN(16) { *(.rodata*) }
    .data : ALIGN(16) { *(.data*) }
    .bss : ALIGN(16) {
        __bss_start = .;
        *(.bss*) *(COMMON)
        __bss_end = .;
    }
    . = ALIGN(16);
    . += 0x4000;
    __stack_top = .;
    __kernel_end = .;
}
