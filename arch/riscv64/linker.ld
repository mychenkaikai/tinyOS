ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80200000, LENGTH = 64M
}

SECTIONS
{
    . = ORIGIN(RAM);
    __kernel_start = .;

    .text : ALIGN(16) {
        *(.text .text.*)
    } > RAM

    .rodata : ALIGN(16) {
        *(.rodata .rodata.*)
    } > RAM

    .data : ALIGN(16) {
        *(.data .data.*)
    } > RAM

    __bss_start = .;
    .bss (NOLOAD) : ALIGN(16) {
        *(.bss .bss.*)
        *(COMMON)
    } > RAM
    __bss_end = .;

    . = ALIGN(16);
    __kernel_end = .;
    __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
}
