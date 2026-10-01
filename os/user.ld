ENTRY(user_program_start)
SECTIONS
{
    . = 0x81000000;
    .text : ALIGN(0x1000) { *(.text.user*) }
    .rodata : ALIGN(0x1000) { *(.rodata.user*) }
    .data : ALIGN(0x1000) { *(.data.user*) }
    .bss : ALIGN(0x1000) { *(.bss.user*) *(COMMON) }
}
