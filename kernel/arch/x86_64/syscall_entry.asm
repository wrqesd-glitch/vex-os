bits 64

global vex_syscall_entry
extern syscall_enter_from_user

section .bss
align 16
syscall_stack:
    resb 16384
syscall_stack_top:

section .text
vex_syscall_entry:
    mov r12, rsp
    mov r13, rcx
    mov r14, r11
    mov r15, cr3

    swapgs

    mov rbx, rsi
    mov r11, rdx
    mov rsp, syscall_stack_top
    and rsp, -16

    push r12
    push r13
    push r14
    push r15

    mov rsi, rax
    mov rdx, rbx
    mov rcx, r11
    mov r8, r10
    mov r9, r15
    cld
    call syscall_enter_from_user

    pop r15
    pop r14
    pop r13
    pop r12

    mov cr3, r15
    mov rsp, r12
    mov rcx, r13
    mov r11, r14

    swapgs
    o64 sysret
