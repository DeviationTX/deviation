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
#include "config/tx.h"

#include <stdlib.h>

#define GPIOA 0xAAAAAAAA
#define GPIOB 0xBBBBBBBB
#define GPIOC 0xCCCCCCCC
#define GPIOD 0xDDDDDDDD
#define GPIOE 0xEEEEEEEE
#define GPIOF 0xFFFFFFFF
#define GPIOG 0xABCDEFAB
void SPI_ProtoInit() {}
void MCU_InitModules() {
    Transmitter.module_enable[CYRF6936].port = GPIOB;
    Transmitter.module_enable[CYRF6936].pin  = 1 << 12;
    Transmitter.module_poweramp = 1;
};
int MCU_SetPin(struct mcu_pin *port, const char *name) {
    switch(name[0]) {
        case 'A':
        case 'a':
            port->port = GPIOA; break;
        case 'B':
        case 'b':
            port->port = GPIOB; break;
        case 'C':
        case 'c':
            port->port = GPIOC; break;
        case 'D':
        case 'd':
            port->port = GPIOD; break;
        case 'E':
        case 'e':
            port->port = GPIOE; break;
        case 'F':
        case 'f':
            port->port = GPIOF; break;
        case 'G':
        case 'g':
            port->port = GPIOG; break;
        default:
            if(strcasecmp(name, "None") == 0) {
                port->port = 0;
                port->pin = 0;
                return 1;
            }
            return 0;
    }
    int x = atoi(name+1);
    if (x > 15)
        return 0;
    port->pin = 1 << x;
    if(port == &Transmitter.module_enable[0]) {
        printf("Set CYRF6936 Enable: %s ", name);
    } else if(port == &Transmitter.module_enable[1]) {
        printf("Set A7105 Enable: %s ", name);
    } else if(port == &Transmitter.module_enable[2]) {
        printf("Set CC2500 Enable: %s ", name);
    } else if(port == &Transmitter.module_enable[3]) {
        printf("Set NRF24L01 Enable: %s ", name);
    } else {
        return 0;
    }
    printf("port: %08x pin: %04x\n", port->port, port->pin);
    return 1;
}

const char *MCU_GetPinName(char *str, struct mcu_pin *port)
{
    switch(port->port) {
        case GPIOA: str[0] = 'A'; break;
        case GPIOB: str[0] = 'B'; break;
        case GPIOC: str[0] = 'C'; break;
        case GPIOD: str[0] = 'D'; break;
        case GPIOE: str[0] = 'E'; break;
        case GPIOF: str[0] = 'F'; break;
        case GPIOG: str[0] = 'G'; break;
        default: return "None"; break;
    }
    for(int i = 0; i < 16; i++) {
        if(port->pin == (1 << i)) {
            sprintf(str+1, "%d", i);
            return str;
        }
    }
    return "None";
}
#ifdef PROTO_HAS_A7105
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

#ifdef PROTO_HAS_CC2500
void CC2500_WriteReg(u8 address, u8 data) {
    (void)address;
    (void)data;
}
void CC2500_Strobe(u8 state) {
    (void)state;
}
void CC2500_Reset() {}
void CC2500_WriteData(u8 *dpbuffer, u8 len)
{
    int i;
    for(i = 0; i < len; i++)
        printf(" %02x", dpbuffer[i]);
    printf("\n");
}
u8 CC2500_ReadReg(u8 address)
{
    (void)address;
    return 0;
}
void CC2500_ReadData(u8 *dpbuffer, int len)
{
    memset(dpbuffer, 0, len);
}

#endif //PROTO_HAS_CC2500
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
void CYRF_WriteDataPacketLen(const u8 data[], u8 len) {
    int i;
    for(i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");
}
void CYRF_WriteDataPacket(const u8 data[]) {
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

void NRF24L01_Initialize() {}
u8 NRF24L01_WriteReg(u8 reg, u8 data)
{
    (void)reg;
    (void)data;
    return 0;
}

u8 NRF24L01_WriteRegisterMulti(u8 reg, const u8 data[], u8 length)
{
    (void)reg;
    (void)data;
    (void)length;
    return 0;
}

u8 NRF24L01_WritePayload(u8 *data, u8 length)
{
    (void)data;
    (void)length;
    return 0;
}

u8 NRF24L01_ReadReg(u8 reg)
{
    (void)reg;
    return 0;
}

u8 NRF24L01_ReadRegisterMulti(u8 reg, u8 data[], u8 length)
{
    (void)reg;
    (void)data;
    (void)length;
    return 0;
}

u8 NRF24L01_ReadPayload(u8 *data, u8 length)
{
    (void)data;
    (void)length;
    return 0;
}

u8 NRF24L01_FlushTx() {
    return 0;
}

u8 NRF24L01_FlushRx() {
    return 0;
}

u8 NRF24L01_Activate(u8 code)
{
    (void)code;
    return 0;
}

u8 NRF24L01_SetBitrate(u8 bitrate)
{
    (void)bitrate;
    return 0;
}

u8 NRF24L01_SetPower(u8 power)
{
    (void)power;
    return 0;
}

void NRF24L01_PulseCE() {}

void PWM_Initialize() {}
void PWM_Stop() {}
void PPM_Enable(u16 low_time, volatile u16 *pulses) {
    int i;
    printf("PPM: low=%d ", (int)low_time);
    for(i = 0; pulses[i]; i++)
        printf("%04d ", pulses[i]);
    printf("\n");
}
