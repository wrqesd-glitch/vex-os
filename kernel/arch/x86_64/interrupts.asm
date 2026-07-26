bits 64

global vex_isr_ts
global vex_isr_np
global vex_isr_ss
global vex_isr_gp
global vex_isr_pf
global vex_isr_ud
global vex_isr_df

extern vex_interrupt_fault

%macro FAULT_STUB 2
section .text
%1:
    push qword %2
    mov rdi, rsp
    cld
    call vex_interrupt_fault
.halt:
    cli
    hlt
    jmp .halt
%endmacro

%macro FAULT_STUB_NOERR 2
section .text
%1:
    push qword 0
    push qword %2
    mov rdi, rsp
    cld
    call vex_interrupt_fault
.halt:
    cli
    hlt
    jmp .halt
%endmacro

FAULT_STUB_NOERR vex_isr_ud, 6
FAULT_STUB vex_isr_df, 8
FAULT_STUB vex_isr_ts, 10
FAULT_STUB vex_isr_np, 11
FAULT_STUB vex_isr_ss, 12
FAULT_STUB vex_isr_gp, 13
FAULT_STUB vex_isr_pf, 14
