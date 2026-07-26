ENTRY(_start)

SECTIONS
{
    . = 0x40000000;

    .text : ALIGN(16)
    {
        *(.text*)
        *(.rodata*)
    }

    .data : ALIGN(16)
    {
        *(.data*)
        *(.bss*)
        *(COMMON)
    }
}
