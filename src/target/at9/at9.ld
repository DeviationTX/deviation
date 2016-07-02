MEMORY
{
	rom (rx) : ORIGIN = 0x08003000, LENGTH = 500K  /* 12K is allocated to the bootloader, so only 500K is available */
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}
_crc_offset = 0x1000; /* This is unneccessary but saves duplicating the ld file */
/*
 * INCLUDE ../target/common/devo/devo.ld
 */
INCLUDE target/common/devo/devo.ld
