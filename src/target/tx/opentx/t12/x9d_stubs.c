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
void MSC_Enable() {}
void MSC_Disable() {}
void USB_Enable(unsigned use_interrupt) {
    (void)use_interrupt;
}
void USB_Disable() {}
void HID_Enable() {}
void HID_Disable() {}
void HID_Write(s8 *pkt, u8 size) {
    (void)pkt;
    (void)size;
}
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

void SPIFlash_Init() {}
void SPI_FlashBlockWriteEnable(unsigned enable) {
    (void)enable;
}
void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer) {
    (void)readAddress;
    (void)length;
    (void)buffer;
}
int SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer) {
    (void)readAddress;
    (void)length;
    (void)buffer;
    return 0;
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

volatile u8 ppmSync = 0;     //  the ppmSync for mixer.c,  0:ppm-Not-Sync , 1:ppm-Got-Sync
volatile s32 ppmChannels[MAX_PPM_IN_CHANNELS];    //  [0...ppmin_num_channels-1] for each channels width, [ppmin_num_channels] for sync-signal width
volatile u8 ppmin_num_channels;     //  the ppmin_num_channels for mixer.c
void PPMin_Init() {}
void PPMin_Stop() {}
void PPMin_Start() {}

void SPITouch_Init() {}
void PPMin_TIM_Init() {}

const char *MCU_GetPinName(char *str, struct mcu_pin *port) {
    (void)str;
    (void)port;
    return "None";
}

void PWM_Initialize() {}
void PWM_Stop() {}
void PPM_Enable(unsigned active_time, volatile u16 *pulses, u8 num_pulses, u8 polarity) {
    (void)active_time;
    (void)pulses;
    (void)num_pulses;
    (void)polarity;
}

void PXX_Enable(u8 *packet) { (void)packet; }
