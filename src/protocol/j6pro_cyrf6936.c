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
    //Allows the linker to properly relocate
    #define J6PRO_Cmds PROTO_Cmds
    #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "telemetry.h"

#ifdef MODULAR
    #pragma long_calls_off
    extern unsigned _data_loadaddr;
    const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CYRF6936
#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us

//For Debug
//#define NO_SCRAMBLE

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
const u8 bind_sop_code[] = {0x62, 0xdf, 0xc1, 0x49, 0xdf, 0xb1, 0xc0, 0x49};
const u8 data_code[] = {0x02, 0xf9, 0x93, 0x97, 0x02, 0xfa, 0x5c, 0xe3, 0x01, 0x2b, 0xf1, 0xdb, 0x01, 0x32, 0xbe, 0x6f};

static enum PktState state;
static u8 packet[16];
static u8 radio_ch[4];
static u8 num_channels;
#ifdef USE_FIXED_MFGID
    //static const u8 cyrfmfg_id[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
    static const u8 cyrfmfg_id[6] = {0x49, 0xec, 0xa9, 0xc4, 0xc1, 0xff};
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
    packet[0] = 0xaa; //FIXME what is this?
    for (i = 0; i < 12; i++) {
        if (i >= num_channels) {
            packet[i+1] = 0xff;
            continue;
        }
        s32 value = (s32)Channels[i] * 0x200 / CHAN_MAX_VALUE + 0x200;
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
    CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);         //RXF, force receive clock enable
    CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3c);  //AUTO_CAL_TIME = 3Ch, typical configuration
    CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14); //AUTO_CAL_OFFSET = 14h, typical configuration
    CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);  //STRIM MSB = 0x05, typical configuration
    CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);  //STRIM LSB = 0x55, typical configuration
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4a);         //LNA + FAST TURN EN + RXOW EN, enable low noise amplifier, fast turning, overwrite enable
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | 0x07);  //Data Code Length = 64 chip codes + Data Mode = 8DR Mode + max-power(+4 dBm)
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0e);   //TH64 = 0Eh, set pn correlation threshold
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xee);    //SOP EN + SOP LEN = 64 chips + LEN EN + SOP TH = 0Eh
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);    //Reset TX overrides
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x00);    //Reset RX overrides
    CYRF_ConfigDataCode(data_code, 16);
    CYRF_WritePreamble(0x333302);                     //Default preamble
#ifndef USE_FIXED_MFGID
    CYRF_GetMfgData(cyrfmfg_id);
    if (Model.fixed_id) {
        cyrfmfg_id[0] ^= (Model.fixed_id >> 0) & 0xff;
        cyrfmfg_id[1] ^= (Model.fixed_id >> 8) & 0xff;
        cyrfmfg_id[2] ^= (Model.fixed_id >> 16) & 0xff;
        cyrfmfg_id[3] ^= (Model.fixed_id >> 24) & 0xff;
    }
#endif
}
static void cyrf_bindinit()
{
    /* Use when binding */
    CYRF_SetPower(0x07); //Use max power (+4 dBm) for binding in case there is no telem module
    CYRF_ConfigSOPCode(bind_sop_code);
    CYRF_ConfigCRCSeed(0x0000);
    build_bind_packet();
}
static void cyrf_datainit()
{
    /* Use when already bound */
    u8 sop_idx = (0xff & (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + cyrfmfg_id[3] - cyrfmfg_id[5])) % 19;
    u16 crc =  (0xff & (cyrfmfg_id[1] - cyrfmfg_id[4] + cyrfmfg_id[5])) |
              ((0xff & (cyrfmfg_id[2] + cyrfmfg_id[3] - cyrfmfg_id[4] + cyrfmfg_id[5])) << 8);
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

MODULE_CALLTYPE
static u16 j6pro_cb()
{
    switch(state) {
        case J6PRO_BIND:
            cyrf_bindinit();
            state = J6PRO_BIND_01;
            /* FALLTHROUGH */
            //no break because we want to send the 1st bind packet now
        case J6PRO_BIND_01:
            CYRF_ConfigRFChannel(0x52);
            CYRF_SetTxRxMode(TX_EN);
            CYRF_WriteDataPacketLen(packet, 0x09);
            state = J6PRO_BIND_03_START;
            return 3000; //3msec
        case J6PRO_BIND_03_START:
            {
                int i = 0;
                while (! (CYRF_ReadRegister(0x04) & 0x06))
                    if(++i > NUM_WAIT_LOOPS)
                        break;
            }
            CYRF_ConfigRFChannel(0x53);
            CYRF_SetTxRxMode(RX_EN);
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80); //Prepare to receive
            state = J6PRO_BIND_03_CHECK;
            return 30000; //30msec
        case J6PRO_BIND_03_CHECK:
            {
            u8 rx = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
            if((rx & 0x1a) == 0x1a) {
                rx = CYRF_ReadRegister(CYRF_0A_RX_LENGTH);
                if(rx == 0x0f) {
                    rx = CYRF_ReadRegister(CYRF_09_RX_COUNT);
                    if(rx == 0x0f) {
                        //Expected and actual length are both 15
                        CYRF_ReadDataPacketLen(packet, rx);
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
                            CYRF_SetTxRxMode(TX_EN);
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
            CYRF_WriteDataPacketLen(packet, 0x0f);
            state = state + 1;
            return 4600; //4.6msec
        case J6PRO_CHANSEL:
            PROTOCOL_SetBindState(0);
            set_radio_channels();
            cyrf_datainit();
            state = J6PRO_CHAN_1;
            /* FALLTHROUGH */
        case J6PRO_CHAN_1:
            //Keep transmit power updated
            CYRF_SetPower(Model.tx_power);
            build_data_packet();
            /* FALLTHROUGH */
            //return 3400;
        case J6PRO_CHAN_2:
            //return 3500;
        case J6PRO_CHAN_3:
            //return 3750
        case J6PRO_CHAN_4:
            CYRF_ConfigRFChannel(radio_ch[state - J6PRO_CHAN_1]);
            CYRF_SetTxRxMode(TX_EN);
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

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    CYRF_Reset();
    cyrf_init();
    num_channels = 8;
    if (bind) {
        state = J6PRO_BIND;
        PROTOCOL_SetBindState(0xFFFFFFFF);
    } else {
        state = J6PRO_CHANSEL;
    }
    CLOCK_StartTimer(2400, j6pro_cb);
}

const void *J6PRO_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(CYRF_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return 0; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        case PROTOCMD_CURRENT_ID: return 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
