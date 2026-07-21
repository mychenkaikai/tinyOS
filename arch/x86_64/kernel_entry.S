.intel_syntax noprefix
.section .text.boot, "ax"
.global _start
.extern kernel_main
.extern __bss_start
.extern __bss_end

.macro debugcon_mark value
    mov dx, 0x402
    mov al, \value
    out dx, al
.endm

_start:
    .byte 0xf3, 0x0f, 0x1e, 0xfa
    # #region debug-point A:kernel-entry
    debugcon_mark 'A'
    mov rbx, rdi
    lea rsp, [kernel_stack_top]
    and rsp, -16
    debugcon_mark 'B'
    lea rdi, [__bss_start]
    lea rcx, [__bss_end]
    sub rcx, rdi
    xor eax, eax
    shr rcx, 3
    rep stosq
    debugcon_mark 'C'
    mov rdi, rbx
    debugcon_mark 'D'
    call kernel_main
    # #endregion

.hang:
    cli
    hlt
    jmp .hang

.section .bss
.align 16
kernel_stack:
    .skip 16384
kernel_stack_top:
