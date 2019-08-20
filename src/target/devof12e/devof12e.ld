MEMORY
{
        /* DevoF12e has 256K, and bootloader takes up 16K, but we allow an extra 4KB
           for the checksum because we will run a 2nd pass that is optimized
           around the crc 'hole'. This is optimistic by the size of the vector table */
        /* Added an extra 4kB so an elf file is always generated
           this makes it easier to run analysis and choose functions to
           reduce.  The 2nd pass will properly check the actual memory usage */
	rom (rx) : ORIGIN = 0x08004000, LENGTH = 248K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}
_crc_offset = 0x1000; /* CRC is located at 0x08005000 */
INCLUDE target/common/devo/devo.ld
