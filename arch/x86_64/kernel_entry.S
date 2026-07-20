.intel_syntax noprefix
.section .text.boot, "ax"
.global _start
.extern kernel_main
.extern __bss_start
.extern __bss_end

_start:
    lea rsp, [kernel_stack_top]
    and rsp, -16
    lea rdi, [__bss_start]
    lea rcx, [__bss_end]
    sub rcx, rdi
    xor eax, eax
    shr rcx, 3
    rep stosq
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
