ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x4000000;
    __kernel_start = .;

    .text : ALIGN(0x1000)
    {
        *(.text*)
        *(.rodata*)
    } :text

    .data : ALIGN(0x1000)
    {
        *(.data*)
    } :data

    .bss : ALIGN(0x1000)
    {
        *(COMMON)
        *(.bss*)
    } :data

    __kernel_end = .;
}
