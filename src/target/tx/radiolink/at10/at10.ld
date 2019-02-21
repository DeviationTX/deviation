MEMORY
{
	rom (rx) : ORIGIN = 0x0800a000, LENGTH = 472K  /* 20K is allocated to the bootloader, so only 492K is available */
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}
_crc_offset = 0x150; /* This is unneccessary but saves duplicating the ld file */
/*
 * INCLUDE target/tx/devo/common/devo.ld
 */
INCLUDE target/tx/devo/common/devo.ld
