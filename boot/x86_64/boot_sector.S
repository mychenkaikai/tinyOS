#include "boot_layout.inc"

.intel_syntax noprefix
.section .text
.code16
.global _start

.set STAGE2_LOAD_ADDR, 0x8000

_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [boot_drive], dl
    mov si, boot_msg
    call print_string

    mov si, stage2_dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    push 0x0000
    push STAGE2_LOAD_ADDR
    lret

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0e
    mov bx, 0x0007
    int 0x10
    jmp print_string
.done:
    ret

disk_error:
    mov si, error_msg
    call print_string
    cli
.hang:
    hlt
    jmp .hang

boot_msg:
    .asciz "tinyOS booting...\r\n"
error_msg:
    .asciz "tinyOS boot failed while loading stage2.\r\n"

boot_drive:
    .byte 0
    .byte 0

stage2_dap:
    .byte 0x10
    .byte 0x00
    .word STAGE2_SECTORS
    .word STAGE2_LOAD_ADDR
    .word 0x0000
    .quad STAGE2_LBA

    .org 510
    .word 0xaa55
