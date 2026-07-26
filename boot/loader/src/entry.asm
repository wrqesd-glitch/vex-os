bits 64
default rel

global efi_entry
extern efi_main

section .text
efi_entry:
    sub rsp, 40
    call efi_main
    add rsp, 40
    ret
