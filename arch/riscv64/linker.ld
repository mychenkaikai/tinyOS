ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80200000, LENGTH = 64M
}

SECTIONS
{
    . = ORIGIN(RAM);
    __kernel_start = .;

    .text : ALIGN(4096) {
        *(.text.entry)
        *(.text .text.*)
    } > RAM :text

    .rodata : ALIGN(16) {
        *(.rodata .rodata.*)
        *(.eh_frame .eh_frame.*)
        *(.eh_frame_hdr)
        *(.note.gnu.build-id)
    } > RAM :text

    . = ALIGN(4096);

    .data : ALIGN(16) {
        *(.data .data.*)
        *(.got .got.*)
        *(.got.plt)
        *(.sdata .sdata.*)
    } > RAM :data

    __bss_start = .;
    .bss (NOLOAD) : ALIGN(16) {
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
    } > RAM :data
    __bss_end = .;

    . = ALIGN(16);
    __kernel_end = .;
    __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;

    /DISCARD/ : {
        *(.comment)
        *(.note.GNU-stack)
    }
}
