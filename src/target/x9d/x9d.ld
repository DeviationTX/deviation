MEMORY
{
  rom (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
  ram (rwx)     : ORIGIN = 0x20000000, LENGTH = 128K
}
_crc_offset = 0x1000; /* This is unneccessary but saves duplicating the ld file */
INCLUDE target/common/devo/devo.ld
