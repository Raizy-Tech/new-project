.section .text.boot
.global _start
.type _start, %function

_start:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    cbnz    x0, .secondary_hang

    ldr     x1, =__stack_top
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
    mov     x3, #0
1:
    cmp     x1, x2
    b.hs    2f
    str     x3, [x1], #8
    b       1b
2:
    bl      kernel_main

.hang:
    wfe
    b       .hang

.secondary_hang:
    wfe
    b       .secondary_hang
