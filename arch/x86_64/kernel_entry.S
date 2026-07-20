.intel_syntax noprefix
.section .text
.global _start
.extern kernel_main

_start:
    lea rsp, [kernel_stack_top]
    and rsp, -16
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

.section .bss
.align 16
kernel_stack:
    .skip 16384
kernel_stack_top:
