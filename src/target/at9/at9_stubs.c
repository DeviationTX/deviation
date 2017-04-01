/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
//void USB_Enable(unsigned type, unsigned use_interrupt) {
//    (void) type;
//    (void)use_interrupt;
//}
//void USB_Disable() {}
void init_err_handler() {}

void SOUND_Init() {}
void SOUND_SetFrequency(unsigned frequency, unsigned volume) {
    (void)frequency;
    (void)volume;
}
void SOUND_Start(unsigned msec, u16(*next_note_cb)(), u8 vibrate) {
    (void)msec;
    (void)next_note_cb;
    (void)vibrate;
}
void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)()) {
    (void)msec;
    (void)next_note_cb;
}
void SOUND_Stop() {}
u32 SOUND_Callback() { return 0;}

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

volatile u8 ppmSync = 0;     //  the ppmSync for mixer.c,  0:ppm-Not-Sync , 1:ppm-Got-Sync
volatile s32 ppmChannels[MAX_PPM_IN_CHANNELS];    //  [0...ppmin_num_channels-1] for each channels width, [ppmin_num_channels] for sync-signal width
volatile u8 ppmin_num_channels;     //  the ppmin_num_channels for mixer.c 
void PPMin_Init() {}
void PPMin_Stop() {}
void PPMin_Start() {}

void SPITouch_Init() {}
void PPMin_TIM_Init() {}
int  SPITouch_IRQ() { return 0; }

const char *MCU_GetPinName(char *str, struct mcu_pin *port) {
    (void)str;
    (void)port;
    return "None";
}

void PWM_Initialize() {}
void PWM_Stop() {}
void PWM_Set(int val) {
    (void)val;
}
