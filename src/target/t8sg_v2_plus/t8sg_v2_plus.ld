MEMORY
{
  /* Devo7e-256 has 256K, and bootloader takes up 12K */
  rom (rx) : ORIGIN = 0x08003000, LENGTH = 244K
  ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}
_crc_offset = 0x2000; /* CRC is located at 0x08005000 */
INCLUDE target/common/devo/devo.ld
