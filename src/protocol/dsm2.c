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
#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#ifdef PROTO_HAS_CYRF6936
#define BIND_CHANNEL 0x0d
#define USE_FIXED_MFGID
#define MODEL 0

#ifdef EMULATOR
#define BIND_COUNT 2
#else
#define BIND_COUNT 200
#endif

enum {
    DSM2_BIND = 0,
    DSM2_CHANSEL = BIND_COUNT,
    DSM2_CH1_WRITE_A = 1000,
    DSM2_CH1_CHECK_A = 1001,
    DSM2_CH2_WRITE_A = 1002,
    DSM2_CH2_CHECK_A = 1003,
    DSM2_CH1_WRITE_B = 1004,
    DSM2_CH1_CHECK_B = 1005,
    DSM2_CH2_WRITE_B = 1006,
    DSM2_CH2_CHECK_B = 1007,
};
   
static const u8 pncodes[5][9][8] = {
    /* Note these are in order transmitted (LSB 1st) */
{ /* Row 0 */
  /* Col 0 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8},
  /* Col 1 */ {0x88, 0x17, 0x13, 0x3B, 0x2D, 0xBF, 0x06, 0xD6},
  /* Col 2 */ {0xF1, 0x94, 0x30, 0x21, 0xA1, 0x1C, 0x88, 0xA9},
  /* Col 3 */ {0xD0, 0xD2, 0x8E, 0xBC, 0x82, 0x2F, 0xE3, 0xB4},
  /* Col 4 */ {0x8C, 0xFA, 0x47, 0x9B, 0x83, 0xA5, 0x66, 0xD0},
  /* Col 5 */ {0x07, 0xBD, 0x9F, 0x26, 0xC8, 0x31, 0x0F, 0xB8},
  /* Col 6 */ {0xEF, 0x03, 0x95, 0x89, 0xB4, 0x71, 0x61, 0x9D},
  /* Col 7 */ {0x40, 0xBA, 0x97, 0xD5, 0x86, 0x4F, 0xCC, 0xD1},
  /* Col 8 */ {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86}
},
{ /* Row 1 */
  /* Col 0 */ {0x83, 0xF7, 0xA8, 0x2D, 0x7A, 0x44, 0x64, 0xD3},
  /* Col 1 */ {0x3F, 0x2C, 0x4E, 0xAA, 0x71, 0x48, 0x7A, 0xC9},
  /* Col 2 */ {0x17, 0xFF, 0x9E, 0x21, 0x36, 0x90, 0xC7, 0x82},
  /* Col 3 */ {0xBC, 0x5D, 0x9A, 0x5B, 0xEE, 0x7F, 0x42, 0xEB},
  /* Col 4 */ {0x24, 0xF5, 0xDD, 0xF8, 0x7A, 0x77, 0x74, 0xE7},
  /* Col 5 */ {0x3D, 0x70, 0x7C, 0x94, 0xDC, 0x84, 0xAD, 0x95},
  /* Col 6 */ {0x1E, 0x6A, 0xF0, 0x37, 0x52, 0x7B, 0x11, 0xD4},
  /* Col 7 */ {0x62, 0xF5, 0x2B, 0xAA, 0xFC, 0x33, 0xBF, 0xAF},
  /* Col 8 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97}
},
{ /* Row 2 */
  /* Col 0 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97},
  /* Col 1 */ {0x8E, 0x4A, 0xD0, 0xA9, 0xA7, 0xFF, 0x20, 0xCA},
  /* Col 2 */ {0x4C, 0x97, 0x9D, 0xBF, 0xB8, 0x3D, 0xB5, 0xBE},
  /* Col 3 */ {0x0C, 0x5D, 0x24, 0x30, 0x9F, 0xCA, 0x6D, 0xBD},
  /* Col 4 */ {0x50, 0x14, 0x33, 0xDE, 0xF1, 0x78, 0x95, 0xAD},
  /* Col 5 */ {0x0C, 0x3C, 0xFA, 0xF9, 0xF0, 0xF2, 0x10, 0xC9},
  /* Col 6 */ {0xF4, 0xDA, 0x06, 0xDB, 0xBF, 0x4E, 0x6F, 0xB3},
  /* Col 7 */ {0x9E, 0x08, 0xD1, 0xAE, 0x59, 0x5E, 0xE8, 0xF0},
  /* Col 8 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E}
},
{ /* Row 3 */
  /* Col 0 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E},
  /* Col 1 */ {0x80, 0x69, 0x26, 0x80, 0x08, 0xF8, 0x49, 0xE7},
  /* Col 2 */ {0x7D, 0x2D, 0x49, 0x54, 0xD0, 0x80, 0x40, 0xC1},
  /* Col 3 */ {0xB6, 0xF2, 0xE6, 0x1B, 0x80, 0x5A, 0x36, 0xB4},
  /* Col 4 */ {0x42, 0xAE, 0x9C, 0x1C, 0xDA, 0x67, 0x05, 0xF6},
  /* Col 5 */ {0x9B, 0x75, 0xF7, 0xE0, 0x14, 0x8D, 0xB5, 0x80},
  /* Col 6 */ {0xBF, 0x54, 0x98, 0xB9, 0xB7, 0x30, 0x5A, 0x88},
  /* Col 7 */ {0x35, 0xD1, 0xFC, 0x97, 0x23, 0xD4, 0xC9, 0x88},
  /* Col 8 */ {0x88, 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40}
},
{ /* Row 4 */
  /* Col 0 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93},
  /* Col 1 */ {0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C},
  /* Col 2 */ {0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA},
  /* Col 3 */ {0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC},
  /* Col 4 */ {0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84},
  /* Col 5 */ {0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7},
  /* Col 6 */ {0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0},
  /* Col 7 */ {0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1},
  /* Col 8 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8}
},
};
static const u8 pn_bind[] = { 0xc6,0x94,0x22,0xfe,0x48,0xe6,0x57,0x4e };
u8 packet[16];
u8 ch[2];
u8 chidx;
u8 sop_col;
u8 data_col;
u16 state;
#ifdef USE_FIXED_MFGID
static const u8 cyrfmfg_id[6] = {0xd4,0x62,0xd6,0xad,0xd3,0xff};
#else
static u8 cyrfmfg_id[6];
#endif
u8 num_channels;
u16 crc;
u8 model;

static void build_bind_packet()
{
    u8 i;
    u16 sum = 384 - 0x10;
    packet[0] = crc >> 8;
    packet[1] = crc & 0xff;
    packet[2] = 0xff ^ cyrfmfg_id[2];
    packet[3] = (0xff ^ cyrfmfg_id[3]) + model;
    packet[4] = packet[0];
    packet[5] = packet[1];
    packet[6] = packet[2];
    packet[7] = packet[3];
    for(i = 0; i < 8; i++)
        sum += packet[i];
    packet[8] = sum >> 8;
    packet[9] = sum & 0xff;
    packet[10] = 0x01; //???
    packet[11] = num_channels;
    packet[12] = 0xa2; //???
    packet[13] = 0x00; //???
    for(i = 8; i < 14; i++)
        sum += packet[i];
    packet[14] = sum >> 8;
    packet[15] = sum & 0xff;
}
static void build_data_packet(u8 upper)
{
    u8 i;
    static const u8 chmap[] = {6, 0, 2, 4, 3, 1, 5};
    packet[0] = 0xff ^ cyrfmfg_id[2];
    packet[1] = (0xff ^ cyrfmfg_id[3]) + model;
    for (i = 0; i < 7; i++) {
       s32 value = (s32)Channels[upper * 7 + i] * 0x200 / CHAN_MAX_VALUE + 0x200;
       if (value > 0x3ff)
           value = 0x3ff;
       if (value < 0)
           value = 0;
       value = value | (i << 10);
       packet[chmap[i]*2+2] = (value >> 8) & 0xff;
       packet[chmap[i]*2+3] = (value >> 0) & 0xff;
    }

}

static u8 get_pn_row(u8 channel)
{
    return channel % 5;
}

static void cyrf_config()
{
    u8 data_code[32];
    CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x01);
    CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
    CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3c);
    CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
    //0d
    //CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x40);
    CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04); //From Devo - Enable PACTL as GPIO
    CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20); //From Devo
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x48);
    CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
    CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x38);
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0a);
    //CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0x80);
    CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0); //From Devo - Enable XOUT as GPIO
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x04);
    CYRF_WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);
    CYRF_WritePreamble(0x333304);
    CYRF_ConfigRFChannel(0x61);
/*
    CYRF_ConfigRxTx(0);
    CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83); //setup read
    u8 rssi = CYRF_ReadRegister(CYRF_13_RSSI); //poll RSSI valyue = 0x20
    printf("Rssi: %02x\n", rssi);
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x3f); //set pn correlation threshold
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0x7f); //disable sop
    CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83); //setupo read
    rssi = CYRF_ReadRegister(CYRF_13_RSSI);  //poll RSSI value = 0x20
    printf("Rssi: %02x\n", rssi);
    Delay(30000);
    u8 bytes = CYRF_ReadRegister(CYRF_09_RX_COUNT); //0x09, 0x0f //15 bytes in queue?
    u8 data[32];
    printf("count: %d", bytes);
    CYRF_ReadDataPacket(data);
    for(bytes = 0; bytes < 16; bytes++)
        printf(" %02x", data[bytes]);
    printf("\n");
    //0x21, 0xf7 ee af f9 f6 a5 57 28 74 6b 84 64 c4 bb 84 //read 15 bytes
    CYRF_ConfigRxTx(0);
*/
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24); //Force IDLE
    //0x0f, 0x04 //Read state (Idle)
    CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00); //Clear RX abort
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0a); //set pn correlation threshold
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0x4a); //set sop len and threshold
    CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x0f); //Clear RX abort?
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x38); //Set 64chip, SDE mode
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0x4a); //set sop len and threshold
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x04); //disable tx CRC
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x14); //disable rx crc
    CYRF_WriteRegister(CYRF_14_EOP_CTRL, 0x02); //set EOP sync == 2
    CYRF_ConfigRFChannel(BIND_CHANNEL); //This seems to be random?
    u8 pn_row = get_pn_row(BIND_CHANNEL);
    //printf("Ch: %d Row: %d SOP: %d Data: %d\n", BIND_CHANNEL, pn_row, sop_col, data_col);
    CYRF_ConfigCRCSeed(crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    memcpy(data_code, pncodes[pn_row][data_col], 16);
    memcpy(data_code + 16, pncodes[0][8], 8);
    memcpy(data_code + 24, pn_bind, 8);
    CYRF_ConfigDataCode(data_code, 32);
    CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10); //16byte packet
    build_bind_packet();
}

static void cyrf_configdata()
{
//Initialize for reading RSSI
    CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);
//0x13, 0xa6
    CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);
//0x13, 0x20
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
//0x0f, 0x04
    CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xea);
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x00);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | Model.tx_power);
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x3f);
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xff);
//Switch from reading RSSI to Writing
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
//0x0f, 0x04
    CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0a);
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xea);
}

static void set_sop_data_crc()
{
    u8 pn_row = get_pn_row(ch[chidx]);
    //printf("Ch: %d Row: %d SOP: %d Data: %d\n", ch[chidx], pn_row, sop_col, data_col);
    CYRF_ConfigRFChannel(ch[chidx]);
    CYRF_ConfigCRCSeed(chidx ? crc : ~crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    CYRF_ConfigDataCode(pncodes[pn_row][data_col], 16);
}

static u16 dsm2_cb()
{
    if(state < DSM2_CHANSEL) {
        state += 1;
        if(state & 1) {
            CYRF_WriteDataPacket(packet);
            return 8500;
        } else {
            CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);
            return 1500;
        }
    } else if(state < DSM2_CH1_WRITE_A) {
        //CYRF_WriteDataPacket(packet);
        //state = DSM2_BIND;
        //return 10000;
        CYRF_FindBestChannels(ch, 2, 5, 4, 80);
        cyrf_configdata();
        CYRF_ConfigRxTx(1);
        chidx = 0;
        set_sop_data_crc();
        state = DSM2_CH1_WRITE_A;
        PROTOCOL_SetBindState(0);
        return 10000;
    } else if(state == DSM2_CH1_WRITE_A || state == DSM2_CH1_WRITE_B) {
        build_data_packet(state == DSM2_CH1_WRITE_B);
        CYRF_WriteDataPacket(packet);
        state++;
        return 2000;
    } else if(state == DSM2_CH2_WRITE_A || state == DSM2_CH2_WRITE_B) {
        CYRF_WriteDataPacket(packet);
        state++;
        return 2000;
    } else if(state == DSM2_CH1_CHECK_A || state == DSM2_CH1_CHECK_B) {
        while(! (CYRF_ReadRegister(0x04) & 0x02))
            ;
        chidx = !chidx;
        set_sop_data_crc();
        state++;
        return 2000;
    } else if(state == DSM2_CH2_CHECK_A || state == DSM2_CH2_CHECK_B) {
        while(! (CYRF_ReadRegister(0x04) & 0x02))
            ;
        chidx = !chidx;
        set_sop_data_crc();
        if (state == DSM2_CH2_CHECK_A) {
            if (num_channels < 8) {
                state = DSM2_CH1_WRITE_A;
                return 22000 - 6000;
            }
            state = DSM2_CH1_WRITE_B;
            return 11000 - 6000;
        }
        state = DSM2_CH1_WRITE_A;
        return 11000 - 6000;
    } 
    return 0;
}

void DSM2_Initialize()
{
    CLOCK_StopTimer();
    CYRF_Reset();
#ifndef USE_FIXED_MFGID
    CYRF_GetMfgData(cyrfmfg_id);
#endif
    crc = ~((cyrfmfg_id[0] << 8) + cyrfmfg_id[1]); 
    sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
    data_col = 7 - sop_col;
    num_channels = 6;
    model = MODEL;

    cyrf_config();
    CYRF_ConfigRxTx(1);
    state = DSM2_BIND;
    //state = DSM2_CHANSEL;
    if (state == DSM2_BIND)
        PROTOCOL_SetBindState(200 * 10); //msecs
    CLOCK_StartTimer(10000, dsm2_cb);
}
#endif
