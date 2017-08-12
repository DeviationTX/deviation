MEMORY
{
        /* devof7-512 has 512k, and bootloader takes up 12K */
	rom (rx) : ORIGIN = 0x08003000, LENGTH = 116K /*not shure if this is correct for teh 512k*/
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 16K /*not shure if this is correct*/
}
_crc_offset = 0x2000; /* CRC is located at 0x08005000 */
INCLUDE target/common/devo/devo.ld
