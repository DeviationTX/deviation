MEMORY
{
	rom (rx) : ORIGIN = 0x08004000, LENGTH = 240K  /* 16K is allocated to the bootloader, so only 240K is available */
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}
_crc_offset = 0x1000; /* CRC is located at 0x08005000 */
INCLUDE target/common_devo/devo.ld
