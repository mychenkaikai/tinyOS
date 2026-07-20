#include "boot_layout.inc"

.intel_syntax noprefix
.section .text
.code16
.global _start

.set KERNEL_LOAD_SEGMENT, 0x1000
.set PROTECTED_CODE_SEL, 0x08
.set DATA_SEL, 0x10
.set LONG_MODE_CODE_SEL, 0x18
.set PAGE_TABLE_FLAGS, 0x003
.set PAGE_2M_FLAGS, 0x083
.set STACK_TOP, 0x70000
.set IA32_EFER, 0xc0000080
.set EFER_LME, 0x00000100
.set CR0_PE, 0x00000001
.set CR0_PG, 0x80000000
.set CR4_PAE, 0x00000020

_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    mov [boot_drive], dl

    mov si, load_msg
    call print_string

    mov si, kernel_dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc kernel_load_error

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, CR0_PE
    mov cr0, eax
    ljmp PROTECTED_CODE_SEL, protected_mode_entry

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

kernel_load_error:
    mov si, error_msg
    call print_string
    cli
.hang:
    hlt
    jmp .hang

.code32
protected_mode_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, STACK_TOP

    mov edi, 0x9000
    mov ecx, 0x3000 / 4
    xor eax, eax
    rep stosd

    mov dword ptr [0x9000], 0xA000 | PAGE_TABLE_FLAGS
    mov dword ptr [0x9004], 0x00000000

    mov dword ptr [0xA000], 0xB000 | PAGE_TABLE_FLAGS
    mov dword ptr [0xA004], 0x00000000

    mov dword ptr [0xB000], 0x00000000 | PAGE_2M_FLAGS
    mov dword ptr [0xB004], 0x00000000

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    mov eax, 0x9000
    mov cr3, eax

    mov ecx, IA32_EFER
    rdmsr
    or eax, EFER_LME
    wrmsr

    mov eax, cr0
    or eax, CR0_PG
    mov cr0, eax

    ljmp LONG_MODE_CODE_SEL, long_mode_entry

.code64
long_mode_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, STACK_TOP
    movabs rax, KERNEL_ENTRY_ADDR
    jmp rax

.align 8
gdt_start:
    .quad 0x0000000000000000
    .quad 0x00cf9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x00af9a000000ffff
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

load_msg:
    .asciz "tinyOS stage2 loading kernel...\r\n"
error_msg:
    .asciz "tinyOS stage2 failed to load kernel.\r\n"

boot_drive:
    .byte 0
    .byte 0

.align 8
kernel_dap:
    .byte 0x10
    .byte 0x00
    .word KERNEL_SECTORS
    .word 0x0000
    .word KERNEL_LOAD_SEGMENT
    .quad KERNEL_LBA
