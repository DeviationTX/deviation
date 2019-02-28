#include "common.h"

#include <libopencm3/stm32/flash.h>

#define FLASH_PAGE_SIZE 2048
#define FLASH_ADDRESS (void *)0x08040000

static u8 writable;

void MCUFlash_Init() {
}

u32 MCUFlash_ReadID() {
    return 0x09106;
}
void MCUFlash_BlockWriteEnable(unsigned enable) {
    writable = enable;
}

void MCUFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer) {
    u8 *address = FLASH_ADDRESS + readAddress;
    for (unsigned i = 0; i < length; i++)
    {
        buffer[i] = ~address[i];
    }
}

int MCUFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer) {
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

void MCUFlash_WriteByte(u32 writeAddress, const unsigned data) {
    u16 temp;
    if (writeAddress & 0x01) {
        // write high part of u16
        temp = (data << 8) | 0x00;
    } else {
        // write low part of u16
        temp = 0x0000 | data;
    }

    flash_program_half_word(writeAddress & ~0x01, temp);
}

void MCUFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer) {
    if (!writable)
        return;

    flash_unlock();

    // if write address is not aligned
    if (writeAddress & 0x01) {
        MCUFlash_WriteByte(writeAddress, buffer[0]);
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
        MCUFlash_WriteByte(writeAddress + length, buffer[length]);
    }

    flash_lock();
}

void MCUFlash_EraseSector(u32 sectorAddress) {
    if (!writable)
        return;

    // Erase two pages in one call
    // we simulate a 4KB based device
    flash_erase_page(sectorAddress);
    flash_erase_page(sectorAddress + FLASH_PAGE_SIZE);
}
