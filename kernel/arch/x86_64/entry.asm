bits 64

global _start
extern kernel_main

section .text
_start:
    lea rsp, [rel boot_stack_top]
    and rsp, -16
    call kernel_main
.halt:
    hlt
    jmp .halt

section .bss
align 16
boot_stack:
    resb 65536
boot_stack_top:
