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

#include "common.h"
#include "protocol/interface.h"
#include "config/model.h"

#include <stdlib.h>

#if HAS_A7105
void A7105_Initialize()
{
}

void A7105_WriteReg(u8 addr, u8 value)
{
    (void)addr;
    (void)value;
    //send(addr);
    //send(value);
}

void A7105_WriteData(u8 *data, u8 len, u8 channel)
{
    (void)data;
    (void)len;
    (void)channel;
    int i;
    printf("%02x:", channel);
    for(i = 0; i < len; i++)
        printf(" %02x", data[i]);
    printf("\n");
}
void A7105_ReadData(u8 *data, u8 len)
{
    (void)data;
    (void)len;
    return;
}

void A7105_SetPower(int power)
{
    printf("Set Tx Power to %s\n", RADIO_TX_POWER_VAL[power]);
}

u8 A7105_ReadReg(u8 addr)
{
    (void)addr;
    //send(0x40 | addr)
    //return read();
    return 0x00;
}

void A7105_Reset()
{
    //send(0x00);
    //send(0x00);
}

void A7105_WriteID(u32 id)
{
    (void)id;
    //send(0x06);
    //send(id >> 24);
    //send(0xff & (id >> 16));
    //send(0xff & (id >> 8));
    //send(0xff & id);
}

void A7105_Strobe(enum A7105_State state)
{
    (void)state;
    //send(state)//
}
#endif

/* CYRF */
void CYRF_Initialize() {}
void CYRF_Reset() {}
void CYRF_GetMfgData(u8 data[]) { 
    u8 d[] = { 0xf8, 0xa4, 0x79, 0x00, 0x00, 0x00};
    //u8 d[] = { 0xd4, 0x62, 0xd6, 0xad, 0xd3, 0xff};
    memcpy(data, d, 6);
}

void CYRF_StartReceive() {}
void CYRF_ConfigCRCSeed(u16 crc) {
    printf("CRC: LSB=%02x MSB=%02x\n", crc & 0xff, crc >> 8);
}
void CYRF_ConfigSOPCode(const u8 *sopcodes) {
    int i;
    printf("SOPCode:");
    for(i = 7; i >= 0; i--) {
        printf(" %02x", sopcodes[i]);
    }
    printf("\n");
    return;
}
void CYRF_ConfigDataCode(const u8 *datacodes, u8 len) {
    int i;
    printf("DATACode:");
    for(i = len - 1; i >= 0; i--) {
        printf(" %02x", datacodes[i]);
    }
    printf("\n");
    return;
}
void CYRF_WritePreamble(u32 preamble) {
    printf("Preamble: %02x %02x %02x\n", preamble & 0xff, (preamble >> 8) & 0xff, (preamble >> 16) & 0xff);
}
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
void CYRF_SetPower(u8 power) {
    printf("Set Tx Power to %s\n", RADIO_TX_POWER_VAL[power]);
}
void CYRF_WriteDataPacketLen(u8 data[], u8 len) {
    int i;
    for(i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");
}
void CYRF_WriteDataPacket(u8 data[]) {
    CYRF_WriteDataPacketLen(data, 16);
}
u8 CYRF_MaxPower() { return CYRF_PWR_100MW; }

void CYRF_FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 min, u8 max)
{
    int i;
    if (min < 4)
        min = 4;
    if (max > 80)
        max = 80;
    memset(channels, 0, sizeof(u8) * len);
    for (i = 0; i < len; i++) {
        channels[i] = min;
        min = min + minspace;
        if (min > max)
            break;
    }
}

void PWM_Initialize() {}
void PWM_Stop() {}
void PPM_Enable(u16 low_time, volatile u16 *pulses) {
    int i;
    printf("PPM: low=%d ", (int)low_time);
    for(i = 0; pulses[i]; i++)
        printf("%04d ", pulses[i]);
    printf("\n");
}
