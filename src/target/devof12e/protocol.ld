MEMORY
{
	ram (rwx) : ORIGIN = 0x20009800, LENGTH = 10K
}
SECTIONS
{
        .text : {
            KEEP(*(.rodata.protocol_type))
            KEEP(*(.text.PROTO_Cmds))
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
