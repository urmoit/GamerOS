ENTRY(_start)

SECTIONS
{
    . = 0x100000; /* Kernel loaded at 1MB */

        .multiboot_header : ALIGN(4)
    {
        *(.multiboot_header)
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}

