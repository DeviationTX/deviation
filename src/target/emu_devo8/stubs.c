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

#include "target.h"


void USB_Enable(u8 use_interrupt) {(void)use_interrupt;}
void USB_Disable() {}
void Initialize_ButtonMatrix() {}
void PWR_Init(void) {}
u16  PWR_ReadVoltage() { return ((5 << 12) | 500); }
void CHAN_Init() {}
void SOUND_Init() {}

void SPIFlash_Init() {}
u32  SPIFlash_ReadID() { return 0x12345678; }
void SPI_FlashBlockWriteEnable(u8 enable) {(void)enable;}
void SPITouch_Init() {}

u8 *BOOTLOADER_Read(int idx) {
    static u8 str[3][80] = {
        "",
        "Devo8 Emu"
        };
    u8 ret = 0;
    switch(idx) {
        case BL_ID: ret = 1; break;
    }
    return str[ret];
}
    
void UART_Initialize() {}
void CYRF_Initialize() {}
void CYRF_GetMfgData(u8 data[]) { 
    u8 d[] = { 0xf8, 0xa4, 0x79, 0x00, 0x00, 0x00};
    memcpy(data, d, 6);
}
void SignOn() {}

int FS_Mount() {return 1;}

void CYRF_ConfigCRCSeed(u8 crc) {(void)crc;}
void CYRF_StartReceive() {}
void CYRF_ConfigSOPCode(u32 idx) {(void)idx;}
void CYRF_ReadDataPacket(u8 dpbuffer[]) {(void)dpbuffer;}
u8 CYRF_ReadRSSI(u32 dodummyread)
{
    (void)dodummyread;
    return rand();
}
u8 CYRF_ReadRegister(u8 addr) {
    if(addr == 0x04) {
        printf("Transmit successful\n");
        return 0x9a;
    };
    return 0;
}
void CYRF_WriteRegister(u8 addr, u8 data) {(void)addr; (void)data;}
void CYRF_ConfigRxTx(u32 TxRx) {
    printf("Switching to %s mode\n", TxRx ? "transmit" : "receive");
}
void CYRF_ConfigRFChannel(u8 ch) {
    printf("Changed channel to %02x\n", ch);
}
void CYRF_WriteDataPacket(u8 data[]) {
    int i;
    for(i = 0; i < 16; i++)
        printf("%02x ", data[i]);
    printf("\n");
}
