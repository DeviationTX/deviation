MEMORY
{
	ram (rwx) : ORIGIN = 0x20004000, LENGTH = 4K
}
SECTIONS
{
        .text : {
            KEEP(*(.rodata.protocol_type))
            KEEP(*(.text.DSM2_Cmds))
            *(.text*)
            . = ALIGN(4);
            *(.rodata*)     /* Read-only data */
            . = ALIGN(4);
        } >ram
	.data : {
                _data_start = .;
		*(.data*)	   /* Read-write initialized data */
                . = _data_start;   /* Ensure there is no data section, since initialization won't happen */
	} >ram
	.bss : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
        } >ram
}
