MEMORY
{
  /* Devo7e-256 has 256K, and bootloader takes up 12K */
  rom (rx) : ORIGIN = 0x08006000, LENGTH = 232K
  ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}
INCLUDE target/tx/devo/t8sg/t8sg.common.ld
