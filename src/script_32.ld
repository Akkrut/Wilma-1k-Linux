/* ld glues every useful crumb together and promises not to add furniture. */

SECTIONS {
    . = PAYLOAD_VADDR;

    combined . : AT(PAYLOAD_VADDR) ALIGN(1) SUBALIGN(1) {
        *(.text*)
        *(.data*)
        *(.rodata*)
        *(.bss*)
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.gnu*)
    }
}
