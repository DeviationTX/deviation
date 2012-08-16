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

#include "target.h"
#include "interface.h"
#include "mixer.h"

#ifdef PROTO_HAS_CYRF6936

//For Debug
//#define NO_SCRAMBLE

#define USE_FIXED_MFGID

enum PktState {
    J6PRO_BIND,
    J6PRO_BIND_01,
    J6PRO_BIND_03_START,
    J6PRO_BIND_03_CHECK,
    J6PRO_BIND_05_1,
    J6PRO_BIND_05_2,
    J6PRO_BIND_05_3,
    J6PRO_BIND_05_4,
    J6PRO_BIND_05_5,
    J6PRO_BIND_05_6,
    J6PRO_CHANSEL,
    J6PRO_CHAN_1,
    J6PRO_CHAN_2,
    J6PRO_CHAN_3,
    J6PRO_CHAN_4,
};

static const u8 sopcodes[][8] = {
    /* Note these are in order transmitted (LSB 1st) */
    {0x3C, 0x37, 0xCC, 0x91, 0xE2, 0xF8, 0xCC, 0x91},
    {0x9B, 0xC5, 0xA1, 0x0F, 0xAD, 0x39, 0xA2, 0x0F},
    {0xEF, 0x64, 0xB0, 0x2A, 0xD2, 0x8F, 0xB1, 0x2A},
    {0x66, 0xCD, 0x7C, 0x50, 0xDD, 0x26, 0x7C, 0x50},
    {0x5C, 0xE1, 0xF6, 0x44, 0xAD, 0x16, 0xF6, 0x44},
    {0x5A, 0xCC, 0xAE, 0x46, 0xB6, 0x31, 0xAE, 0x46},
    {0xA1, 0x78, 0xDC, 0x3C, 0x9E, 0x82, 0xDC, 0x3C},
    {0xB9, 0x8E, 0x19, 0x74, 0x6F, 0x65, 0x18, 0x74},
    {0xDF, 0xB1, 0xC0, 0x49, 0x62, 0xDF, 0xC1, 0x49},
    {0x97, 0xE5, 0x14, 0x72, 0x7F, 0x1A, 0x14, 0x72},
    {0x82, 0xC7, 0x90, 0x36, 0x21, 0x03, 0xFF, 0x17},
    {0xE2, 0xF8, 0xCC, 0x91, 0x3C, 0x37, 0xCC, 0x91}, //Note: the '03' was '9E' in the Cypress recommended table
    {0xAD, 0x39, 0xA2, 0x0F, 0x9B, 0xC5, 0xA1, 0x0F}, //The following are the same as the 1st 8 above,
    {0xD2, 0x8F, 0xB1, 0x2A, 0xEF, 0x64, 0xB0, 0x2A}, //but with the upper and lower word swapped
    {0xDD, 0x26, 0x7C, 0x50, 0x66, 0xCD, 0x7C, 0x50},
    {0xAD, 0x16, 0xF6, 0x44, 0x5C, 0xE1, 0xF6, 0x44},
    {0xB6, 0x31, 0xAE, 0x46, 0x5A, 0xCC, 0xAE, 0x46},
    {0x9E, 0x82, 0xDC, 0x3C, 0xA1, 0x78, 0xDC, 0x3C},
    {0x6F, 0x65, 0x18, 0x74, 0xB9, 0x8E, 0x19, 0x74},
};

static enum PktState state;
static u8 packet[16];
static u8 radio_ch[4];
static u8 num_channels;
#ifdef USE_FIXED_MFGID
static const u8 cyrfmfg_id[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
#else
static u8 cyrfmfg_id[6];
#endif

void build_bind_packet()
{
    packet[0] = 0x01;  //Packet type
    packet[1] = 0x01;  //FIXME: What is this? Model number maybe?
    packet[2] = 0x56;  //FIXME: What is this?
    packet[3] = cyrfmfg_id[0];
    packet[4] = cyrfmfg_id[1];
    packet[5] = cyrfmfg_id[2];
    packet[6] = cyrfmfg_id[3];
    packet[7] = cyrfmfg_id[4];
    packet[8] = cyrfmfg_id[5];
}
void build_data_packet()
{
    u8 i;
    u32 upperbits = 0;
    s8 channel_map[] = {0, 1, 2, 3, 4, 5, 6, 7};
    packet[0] = 0xaa; //FIXME what is this?
    for (i = 0; i < 12; i++) {
        if (channel_map[i] < 0 || channel_map[i] >= num_channels) {
            packet[i+1] = 0xff;
            continue;
        }
        s32 value = (s32)Channels[channel_map[i]] * 0x400 / CHAN_MAX_VALUE + 0x200;
        if (value < 0)
            value = 0;
        if (value > 0x3ff)
            value = 0x3ff;
        packet[i+1] = value & 0xff;
        upperbits |= (value >> 8) << (i * 2);
    }
    packet[13] = upperbits & 0xff;
    packet[14] = (upperbits >> 8) & 0xff;
    packet[15] = (upperbits >> 16) & 0xff;
}

static void cyrf_init()
{
    /* Initialise CYRF chip */
       CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
       CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3c);
       CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
       CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
       CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
       //CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x00);
       CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);    //From Devo - Enable PACTL as GPIO
       CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);  //From Devo
       CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0); //From Devo - Enable XOUT as GPIO
       CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x25);
       CYRF_WriteRegister(CYRF_03_TX_CFG, 0x05);
       CYRF_WriteRegister(CYRF_06_RX_CFG, 0x8a);
       CYRF_WriteRegister(CYRF_03_TX_CFG, 0x2f);
       CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0e);
       CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xee);
       CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
       CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x00);
       const u8 data_code[] = {0x02, 0xf9, 0x93, 0x97, 0x02, 0xfa, 0x5c, 0xe3, 0x01, 0x2b, 0xf1, 0xdb, 0x01, 0x32, 0xbe, 0x6f};
       CYRF_ConfigDataCode(data_code, 16);
       CYRF_WritePreamble(0x023333);
#ifndef USE_FIXED_MFGID
       CYRF_GetMfgData(cyrfmfg_id);
#endif
}
static void cyrf_bindinit()
{
/* Use when binding */
       //0.060470# 03 2f
       u8 sop_code[] = {0x62, 0xdf, 0xc1, 0x49, 0xdf, 0xb1, 0xc0, 0x49};
       CYRF_WriteRegister(CYRF_03_TX_CFG, 0x29);

       CYRF_ConfigRFChannel(0x52);
       CYRF_ConfigSOPCode(sop_code);
       CYRF_ConfigCRCSeed(0x0000);
       CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4a);
       CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);
       //0.061511# 13 20

       CYRF_ConfigRFChannel(0x52);
       //0.062684# 0f 05
       CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x25);
       //0.062792# 0f 05
       CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x40);
       build_bind_packet(); //01 01 e9 49 ec a9 c4 c1 ff
       //CYRF_WriteDataPacketLen(packet, 0x09);
}
static void cyrf_datainit()
{
/* Use when already bound */
       //0.094007# 0f 05
       u8 sop_idx = (0xff & (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + cyrfmfg_id[3] - cyrfmfg_id[5])) % 19;
       u8 crc =  (0xff & (cyrfmfg_id[1] - cyrfmfg_id[4] + cyrfmfg_id[5])) |
                ((0xff & (cyrfmfg_id[2] + cyrfmfg_id[3] - cyrfmfg_id[4] + cyrfmfg_id[5])) << 8);
       CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x25);
       CYRF_ConfigSOPCode(sopcodes[sop_idx]);
       CYRF_ConfigCRCSeed(crc);
}

static void set_radio_channels()
{
    //FIXME: Query free channels
    //lowest channel is 0x08, upper channel is 0x4d?
    CYRF_FindBestChannels(radio_ch, 3, 5, 8, 77);
    radio_ch[3] = radio_ch[0];
}

static u16 j6pro_cb()
{
    switch(state) {
        case J6PRO_BIND:
            cyrf_bindinit();
            state = J6PRO_BIND_01;
            //no break because we want to send the 1st bind packet now
        case J6PRO_BIND_01:
            CYRF_ConfigRFChannel(0x52);
            CYRF_ConfigRxTx(1);
            //0.062684# 0f 05
            CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x25);
            //0.062684# 0f 05
            CYRF_WriteDataPacketLen(packet, 0x09);
            state = J6PRO_BIND_03_START;
            return 3000; //3msec
        case J6PRO_BIND_03_START:
            while(! (CYRF_ReadRegister(0x04) & 0x06))
                ;
            CYRF_ConfigRFChannel(0x53);
            CYRF_ConfigRxTx(0);
            CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4a);
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);
            state = J6PRO_BIND_03_CHECK;
            return 30000; //30msec
        case J6PRO_BIND_03_CHECK:
            {
            u8 rx = CYRF_ReadRegister(CYRF_07_RX_IRG_STATUS);
            if((rx & 0x1a) == 0x1a) {
                rx = CYRF_ReadRegister(CYRF_0A_RX_LENGTH);
                if(rx == 0x0f) {
                    rx = CYRF_ReadRegister(CYRF_09_RX_COUNT);
                    if(rx == 0x0f) {
                        //Expected and actual length are both 15
                        CYRF_ReadDataPacket(packet);
                        if (packet[0] == 0x03 &&
                            packet[3] == cyrfmfg_id[0] &&
                            packet[4] == cyrfmfg_id[1] &&
                            packet[5] == cyrfmfg_id[2] &&
                            packet[6] == cyrfmfg_id[3] &&
                            packet[7] == cyrfmfg_id[4] &&
                            packet[8] == cyrfmfg_id[5])
                        {
                            //Send back Ack
                            packet[0] = 0x05;
                            CYRF_ConfigRFChannel(0x54);
                            CYRF_ConfigRxTx(1);
                            state = J6PRO_BIND_05_1;
                            return 2000; //2msec
                         }
                    }
                }
            }
            state = J6PRO_BIND_01;
            return 500;
            }
        case J6PRO_BIND_05_1:
        case J6PRO_BIND_05_2:
        case J6PRO_BIND_05_3:
        case J6PRO_BIND_05_4:
        case J6PRO_BIND_05_5:
        case J6PRO_BIND_05_6:
            CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x25);
            CYRF_WriteDataPacketLen(packet, 0x0f);
            state = state + 1;
            return 4600; //4.6msec
        case J6PRO_CHANSEL:
            set_radio_channels();
            cyrf_datainit();
        case J6PRO_CHAN_1:
            build_data_packet();
            //return 3400;
        case J6PRO_CHAN_2:
            //return 3500;
        case J6PRO_CHAN_3:
            //return 3750
        case J6PRO_CHAN_4:
            CYRF_ConfigRFChannel(radio_ch[state - J6PRO_CHAN_1]);
            CYRF_ConfigRxTx(1);
            CYRF_WriteDataPacket(packet);
            if (state == J6PRO_CHAN_4) {
                state = J6PRO_CHAN_1;
                return 13900;
            }
            state = state + 1;
            return 3550;
    }
    return 0;
}

void J6PRO_Initialize()
{
    CLOCK_StopTimer();
    CYRF_Reset();
    cyrf_init();
    num_channels = 8;
    state = J6PRO_BIND;
    //state = J6PRO_CHANSEL; //FIXME: Support skipping bind phase
    CLOCK_StartTimer(2400, j6pro_cb);
}

#endif
