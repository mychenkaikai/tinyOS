.section .text
.align 2
.globl _start
.extern kernel_main
.extern g_riscv64_boot_info
.extern __bss_start
.extern __bss_end
.extern __stack_top

_start:
    la sp, __stack_top
    la t0, __bss_start
    la t1, __bss_end

1:
    bgeu t0, t1, 2f
    sd zero, 0(t0)
    addi t0, t0, 8
    j 1b

2:
    la a0, g_riscv64_boot_info
    call kernel_main

3:
    wfi
    j 3b
