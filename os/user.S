.section .text.user,"ax"
.global user_program_start
.global user_program_end
.type user_program_start, %function
.type user_program_end, %function

user_program_start:
    mov x0, #0x2a
    mov x8, #1
    svc #0
    mov x8, #2
    svc #0
1:
    wfe
    b 1b

user_program_end:
