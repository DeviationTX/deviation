MEMORY
{
        /* Devo7e has 128K, and bootloader takes up 12K */
	rom (rx) : ORIGIN = 0x08003000, LENGTH = 116K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
_crc_offset = 0x2000; /* CRC is located at 0x08005000 */
INCLUDE target/common_devo/devo.ld
