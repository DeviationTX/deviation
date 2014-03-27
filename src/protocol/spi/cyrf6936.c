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
#ifdef MODULAR
  #pragma long_calls
#endif
#include "common.h"
#include "config/tx.h"
//Some versions of gcc applythis to definitions, others to calls
//So just use long_calls everywhere
//#pragma long_calls_off
#include "protocol/interface.h"
#include "protospi.h"

//GPIOB.11
#define RS_HI() PROTOSPI_pin_set(CYRF_RESET_PIN)
#define RS_LO() PROTOSPI_pin_clear(CYRF_RESET_PIN)

//Disable AWA24S
#define AWA24S 0

#define Delay usleep
static void  CS_HI() {
#if HAS_MULTIMOD_SUPPORT
    if (Transmitter.module_enable[MULTIMOD].port) {
        //We need to set the multimodule CSN even if we don't use it
        //for this protocol so that it doesn't interpret commands
        PROTOSPI_pin_set(Transmitter.module_enable[MULTIMOD]);
        if(Transmitter.module_enable[CYRF6936].port == SWITCH_ADDRESS) {
            for(int i = 0; i < 20; i++)
                _NOP();
            return;
        }
    }
#endif
    PROTOSPI_pin_set(Transmitter.module_enable[CYRF6936]);
}

static void CS_LO() {
#if HAS_MULTIMOD_SUPPORT
    if (Transmitter.module_enable[MULTIMOD].port) {
        //We need to set the multimodule CSN even if we don't use it
        //for this protocol so that it doesn't interpret commands
        PROTOSPI_pin_clear(Transmitter.module_enable[MULTIMOD]);
        if(Transmitter.module_enable[CYRF6936].port == SWITCH_ADDRESS) {
            for(int i = 0; i < 20; i++)
                _NOP();
            return;
        }
    }
#endif
    PROTOSPI_pin_clear(Transmitter.module_enable[CYRF6936]);
}

void CYRF_WriteRegister(u8 address, u8 data)
{
    CS_LO();
    PROTOSPI_xfer(0x80 | address);
    PROTOSPI_xfer(data);
    CS_HI();
}

static void WriteRegisterMulti(u8 address, const u8 data[], u8 length)
{
    unsigned char i;

    CS_LO();
    PROTOSPI_xfer(0x80 | address);
    for(i = 0; i < length; i++)
    {
        PROTOSPI_xfer(data[i]);
    }
    CS_HI();
}

static void ReadRegisterMulti(u8 address, u8 data[], u8 length)
{
    unsigned char i;

    CS_LO();
    PROTOSPI_xfer(address);
    for(i = 0; i < length; i++)
    {
        data[i] = PROTOSPI_xfer(0);
    }
    CS_HI();
}

u8 CYRF_ReadRegister(u8 address)
{
    u8 data;

    CS_LO();
    PROTOSPI_xfer(address);
    data = PROTOSPI_xfer(0);
    CS_HI();
    return data;
}

int CYRF_Reset()
{
#if HAS_MULTIMOD_SUPPORT
        CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x01);
        Delay(200);
        /* Reset the CYRF chip */
#else
        RS_HI();
        Delay(100);
        RS_LO();
        Delay(100);
#endif
        CYRF_SetTxRxMode(TXRX_OFF);
        //Verify the CYRD chip is responding
        return (CYRF_ReadRegister(CYRF_10_FRAMING_CFG) == 0xa5);
}

u8 CYRF_MaxPower()
{
    return (*((u8*)0x08001007) == 0) ? CYRF_PWR_100MW : CYRF_PWR_10MW;
}
/*
 *
 */
void CYRF_GetMfgData(u8 data[])
{
#if defined PROTO_HAS_CYRF6936 && ! defined MODULAR
    if(Transmitter.txid) {
        MCU_SerialNumber(data, 6);
        return;
    }
#endif
    /* Fuses power on */
    CYRF_WriteRegister(0x25, 0xFF);

    ReadRegisterMulti(0x25, data, 6);

    /* Fuses power off */
    CYRF_WriteRegister(0x25, 0x00); 
}
/*
 * 1 - Tx else Rx
 */
void CYRF_SetTxRxMode(enum TXRX_State mode)
{
#if AWA24S
    if (Transmitter.module_enable[CYRF6936].port == 0xFFFFFFFF) {
        if(mode == TX_EN)
        {
            CYRF_WriteRegister(0x0D,0x40); //disable Rx (assume IRQ is set?)
            SPI_ConfigSwitch(0x8f, 0x8e);
        }
        else
        {
            CYRF_WriteRegister(0x0D,0x00); //enable Rx (assume IRQ is set?)
            SPI_ConfigSwitch(0x0f, 0x0e);
        }
    }
    else
#endif
    {
        if(mode == TX_EN)
        {
            CYRF_WriteRegister(0x0E,0x80);
            CYRF_WriteRegister(0x0F,0x2C);
        }
        else
        {
            CYRF_WriteRegister(0x0E,0x20);
            CYRF_WriteRegister(0x0F,0x28);
        }
    }
}
/*
 *
 */
void CYRF_ConfigRFChannel(u8 ch)
{
    CYRF_WriteRegister(0x00,ch);
}

void CYRF_SetPower(u8 power)
{
    u8 val = CYRF_ReadRegister(0x03) & 0xF8;
    CYRF_WriteRegister(0x03, val | (power & 0x07));
}

/*
 *
 */
void CYRF_ConfigCRCSeed(u16 crc)
{
    CYRF_WriteRegister(0x15,crc & 0xff);
    CYRF_WriteRegister(0x16,crc >> 8);
}
/*
 * these are the recommended sop codes from Crpress
 * See "WirelessUSB LP/LPstar and PRoC LP/LPstar Technical Reference Manual"
 */
void CYRF_ConfigSOPCode(const u8 *sopcodes)
{
    //NOTE: This can also be implemented as:
    //for(i = 0; i < 8; i++) WriteRegister)0x23, sopcodes[i];
    WriteRegisterMulti(0x22, sopcodes, 8);
}

void CYRF_ConfigDataCode(const u8 *datacodes, u8 len)
{
    //NOTE: This can also be implemented as:
    //for(i = 0; i < len; i++) WriteRegister)0x23, datacodes[i];
    WriteRegisterMulti(0x23, datacodes, len);
}

void CYRF_WritePreamble(u32 preamble)
{
    CS_LO();
    PROTOSPI_xfer(0x80 | 0x24);
    PROTOSPI_xfer(preamble & 0xff);
    PROTOSPI_xfer((preamble >> 8) & 0xff);
    PROTOSPI_xfer((preamble >> 16) & 0xff);
    CS_HI();
}
/*
 *
 */
void CYRF_StartReceive()
{
    CYRF_WriteRegister(0x05,0x87);
}

void CYRF_ReadDataPacket(u8 dpbuffer[])
{
    ReadRegisterMulti(0x21, dpbuffer, 0x10);
}

void CYRF_WriteDataPacketLen(const u8 dpbuffer[], u8 len)
{
    CYRF_WriteRegister(CYRF_01_TX_LENGTH, len);
    CYRF_WriteRegister(0x02, 0x40);
    WriteRegisterMulti(0x20, dpbuffer, len);
    CYRF_WriteRegister(0x02, 0xBF);
}
void CYRF_WriteDataPacket(const u8 dpbuffer[])
{
    CYRF_WriteDataPacketLen(dpbuffer, 16);
}

u8 CYRF_ReadRSSI(u32 dodummyread)
{
    u8 result;
    if(dodummyread)
    {
        CYRF_ReadRegister(0x13);
    }
    result = CYRF_ReadRegister(0x13);
    if(result & 0x80)
    {
        result = CYRF_ReadRegister(0x13);
    }
    return (result & 0x0F);
}

//NOTE: This routine will reset the CRC Seed
void CYRF_FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 min, u8 max)
{
    #define NUM_FREQ 80
    #define FREQ_OFFSET 4
    u8 rssi[NUM_FREQ];

    if (min < FREQ_OFFSET)
        min = FREQ_OFFSET;
    if (max > NUM_FREQ)
        max = NUM_FREQ;

    int i;
    int j;
    memset(channels, 0, sizeof(u8) * len);
    CYRF_ConfigCRCSeed(0x0000);
    CYRF_SetTxRxMode(RX_EN);
    //Wait for pre-amp to switch from send to receive
    Delay(1000);
    for(i = 0; i < NUM_FREQ; i++) {
        CYRF_ConfigRFChannel(i);
        CYRF_ReadRegister(0x13);
        CYRF_StartReceive();
        Delay(10);
        rssi[i] = CYRF_ReadRegister(0x13);
    }

    for (i = 0; i < len; i++) {
        channels[i] = min;
        for (j = min; j < max; j++) {
            if (rssi[j] < rssi[channels[i]]) {
                channels[i] = j;
            }
            
        }
        for (j = channels[i] - minspace; j < channels[i] + minspace; j++) {
            //Ensure we don't reuse any channels within minspace of the selected channel again
            if (j < 0 || j >= NUM_FREQ)
                continue;
            rssi[j] = 0xff;
        }
    }
    CYRF_SetTxRxMode(TX_EN);
}
