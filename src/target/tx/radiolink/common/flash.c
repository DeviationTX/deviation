#include "common.h"

#include <libopencm3/stm32/flash.h>

#define FLASH_PAGE_SIZE 2048
#define FLASH_ADDRESS (void *)0x08040000

extern uint32_t Mass_Block_Size[2];

#if !defined HAS_4IN1_FLASH || !HAS_4IN1_FLASH
void SPIFlash_Init() {
    Mass_Block_Size[0] = FLASH_PAGE_SIZE;
}

void SPI_FlashBlockWriteEnable(unsigned enable) {
    if (enable)
        flash_unlock();
    else
        flash_lock();
}

void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer) {
    u8 *address = FLASH_ADDRESS + readAddress;
    for (unsigned i = 0; i < length; i++)
    {
        buffer[i] = ~address[i];
    }
}

int SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer) {
    unsigned i;
    u8 *address = FLASH_ADDRESS + readAddress;
    for (i = 0; i < length; i++)
    {
        buffer[i] = ~address[i];
        if (buffer[i] == '\n') {
            i++;
            break;
        }
    }
    return i;
}

void SPIFlash_WriteByte(u32 writeAddress, const unsigned data) {
    u16 temp;
    if (writeAddress & 0x01) {
        // write high part of u16
        temp = (data << 8) | *(u8*)writeAddress;
    } else {
        temp = (*(u8*)writeAddress) << 8 | data;
    }

    flash_program_half_word(writeAddress & ~0x01, temp);
}

void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer) {
    flash_unlock();

    // if write address is not aligned
    if (writeAddress & 0x01) {
        SPIFlash_WriteByte(writeAddress, buffer[0]);
        buffer++;
        writeAddress++;
        length--;
    }

    // write 16bit left out
    for (unsigned iter = 0; iter < (length & ~0x02); iter += 2)
    {
        flash_program_half_word(writeAddress + iter, *((uint16_t*)(buffer + iter)));
    }

    // if length is not 2bytes aligned
    if (length & 0x01) {
        SPIFlash_WriteByte(writeAddress + length, buffer[length]);
    }

    flash_lock();
}

void SPIFlash_EraseSector(u32 sectorAddress) {
    flash_erase_page(sectorAddress);
}
#endif  // !defined HAS_4IN1_FLASH || !HAS_4IN1_FLASH
