MEMORY
{
        /* Devo7e has 128K, and bootloader takes up 12K, but we allow an extra 8KB
           for the checksum because we will run a 2nd pass that is optimized
            around the crc 'hole'.  This is optimistic by the size of the vector table */
        /* The RAM length is shortened by 4kB to allow code to be loaded at the end of RAM */
	rom (rx) : ORIGIN = 0x08003000, LENGTH = 124K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
_crc_offset = 0x2000; /* CRC is located at 0x08005000 */
INCLUDE target/common_devo/devo.ld
