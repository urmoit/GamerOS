ENTRY(_start)

SECTIONS
{
        . = 0x100000; /* Kernel loaded at 1MB */

        /* The Multiboot 2 header must be 8-byte aligned; the Multiboot 1
           header that follows it only needs 4. Both live in this section. */
        .multiboot_header : ALIGN(8)
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

