#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_

// Various SPI flash memories use different commands to
// block-protect memory and to write more than 1 byte
// Possible variants:
// Microchip SST25VF032B - original Devo 10
// Microchip SST25VF512A - original Devo F7
// Microchip SST26VF032B
// ISSI IS25CQ032, IS25LQ032B, IS25LP032D
// Winbond W25Q32BV
// Macronix MX25L3233F
// Adesto AT25DF321A

#define SST25VFxxxB 0
#define SST25VFxxxA 1
#define SST26VFxxxB 2
#define IS25CQxxx   3
#define W25QxxBV    4
#define MX25Lxxxxx  5
#define AT25DFxx1A  6

#if SPIFLASH_TYPE == SST25VFxxxB
    #define SPIFLASH_SR_ENABLE    0x50
    #define SPIFLASH_PROTECT_MASK 0x3C
    #define SPIFLASH_WRITE_SIZE   2
    #define SPIFLASH_WRITE_CMD    0xAD
    #define SPIFLASH_FAST_READ    0
    #define SPIFLASH_USE_AAI      1
#elif SPIFLASH_TYPE == SST25VFxxxA
    #define SPIFLASH_SR_ENABLE    0x50
    #define SPIFLASH_PROTECT_MASK 0x0C
    #define SPIFLASH_WRITE_SIZE   1
    #define SPIFLASH_WRITE_CMD    0xAF
    #define SPIFLASH_FAST_READ    0
    #define SPIFLASH_USE_AAI      1
#elif SPIFLASH_TYPE == SST26VFxxxB
    #define SPIFLASH_SR_ENABLE    0x06  //No EWSR, use standard WREN
    #define SPIFLASH_PROTECT_MASK 0
    #define SPIFLASH_WRITE_SIZE   1
    #define SPIFLASH_WRITE_CMD    0x02
    #define SPIFLASH_FAST_READ    1
    #define SPIFLASH_USE_AAI      0
#elif SPIFLASH_TYPE == IS25CQxxx
    #define SPIFLASH_SR_ENABLE    0x06  //No EWSR, use standard WREN
    #define SPIFLASH_PROTECT_MASK 0x3C
    #define SPIFLASH_WRITE_SIZE   1
    #define SPIFLASH_WRITE_CMD    0x02
    #define SPIFLASH_FAST_READ    1
    #define SPIFLASH_USE_AAI      0
#elif SPIFLASH_TYPE == W25QxxBV
    #define SPIFLASH_SR_ENABLE    0x50
    #define SPIFLASH_PROTECT_MASK 0x1C
    #define SPIFLASH_WRITE_SIZE   1
    #define SPIFLASH_WRITE_CMD    0x02
    #define SPIFLASH_FAST_READ    1
    #define SPIFLASH_USE_AAI      0
#elif SPIFLASH_TYPE == MX25Lxxxxx
    #define SPIFLASH_SR_ENABLE    0x06  //No EWSR, use standard WREN
    #define SPIFLASH_PROTECT_MASK 0x3C
    #define SPIFLASH_WRITE_SIZE   1
    #define SPIFLASH_WRITE_CMD    0x02
    #define SPIFLASH_FAST_READ    1
    #define SPIFLASH_USE_AAI      0
#elif SPIFLASH_TYPE == AT25DFxx1A
    #define SPIFLASH_SR_ENABLE    0x06  //No EWSR, use standard WREN
    #define SPIFLASH_PROTECT_MASK 0x3C
    #define SPIFLASH_WRITE_SIZE   1
    #define SPIFLASH_WRITE_CMD    0x02
    #define SPIFLASH_FAST_READ    1
    #define SPIFLASH_USE_AAI      0
#endif
#endif //_SPI_FLASH_H_
