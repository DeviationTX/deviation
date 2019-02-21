#include "common.h"

#if !defined HAS_4IN1_FLASH || !HAS_4IN1_FLASH
void SPIFlash_Init() {}
void SPI_FlashBlockWriteEnable(unsigned enable) {
    (void)enable;
}

#define FS_ADDRESS (void *)0x08040000
void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer) {
    u8 *address = FS_ADDRESS + readAddress;
    for(unsigned i=0;i<length;i++)
    {
        buffer[i] = ~address[i];
    }
}
int SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer) {
    unsigned i;
    u8 *address = FS_ADDRESS + readAddress;
    for(i=0;i<length;i++)
    {
        buffer[i] = ~address[i];
        if (buffer[i] == '\n') {
            i++;
            break;
        }
    }
    return i;
}
void SPIFlash_WriteByte(u32 writeAddress, const unsigned byte) {
    (void)writeAddress;
    (void)byte;
}
void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer) {
    (void)writeAddress;
    (void)length;
    (void)buffer;
}
void SPIFlash_EraseSector(u32 sectorAddress) {
    (void)sectorAddress;
}
#endif // !defined HAS_4IN1_FLASH || !HAS_4IN1_FLASH
