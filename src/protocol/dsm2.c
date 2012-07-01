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

#ifdef PROTO_HAS_DSM2
#define BIND_CHANNEL 0x09
#define MODEL 0
enum {
    DSM2_BIND = 0,
    DSM2_CHANSEL = 100,
    DSM2_CH1_WRITE_A = 200,
    DSM2_CH1_CHECK_A = 201,
    DSM2_CH2_WRITE_A = 202,
    DSM2_CH2_CHECK_A = 203,
    DSM2_CH1_WRITE_B = 204,
    DSM2_CH1_CHECK_B = 205,
    DSM2_CH2_WRITE_B = 206,
    DSM2_CH2_CHECK_B = 207,
};
   
static const u8 pncodes[5][9][8] = {
    /* Note these are in order transmitted (LSB 1st) */
{ /* Row 0 */
  /* Col 8 */ { 0x86,0xAE,0x89,0x5E,0xB1,0x54,0xA1,0xD7 },
  /* Col 7 */ { 0xD1,0xCC,0x4F,0x86,0xD5,0x97,0xBA,0x40 },
  /* Col 6 */ { 0x9D,0x61,0x71,0xB4,0x89,0x95,0x03,0xEF },
  /* Col 5 */ { 0xB8,0x0F,0x31,0xC8,0x26,0x9F,0xBD,0x07 },
  /* Col 4 */ { 0xD0,0x66,0xA5,0x83,0x9B,0x47,0xFA,0x8C },
  /* Col 3 */ { 0xB4,0xE3,0x2F,0x82,0xBC,0x8E,0xD2,0xD0 },
  /* Col 2 */ { 0xA9,0x88,0x1C,0xA1,0x21,0x30,0x94,0xF1 },
  /* Col 1 */ { 0xD6,0x06,0xBF,0x2D,0x3B,0x13,0x17,0x88 },
  /* Col 0 */ { 0xF8,0xFE,0xBD,0xEF,0x8A,0x6E,0xBC,0x03 },
},
{ /* Row 1 */
  /* Col 8 */ { 0x97,0x5D,0xD9,0x0F,0xD9,0x32,0x56,0x40 },
  /* Col 7 */ { 0xAF,0xBF,0x33,0xFC,0xAA,0x2B,0xF5,0x62 },
  /* Col 6 */ { 0xD4,0x11,0x7B,0x52,0x37,0xF0,0x6A,0x1E },
  /* Col 5 */ { 0x95,0xAD,0x84,0xDC,0x94,0x7C,0x70,0x3D },
  /* Col 4 */ { 0xE7,0x74,0x77,0x7A,0xF8,0xDD,0xF5,0x24 },
  /* Col 3 */ { 0xEB,0x42,0x7F,0xEE,0x5B,0x9A,0x5D,0xBC },
  /* Col 2 */ { 0x82,0xC7,0x90,0x36,0x21,0x9E,0xFF,0x17 },
  /* Col 1 */ { 0xC9,0x7A,0x48,0x71,0xAA,0x4E,0x2C,0x3F },
  /* Col 0 */ { 0xD3,0x64,0x44,0x7A,0x2D,0xA8,0xF7,0x83 },
},
{ /* Row 2 */
  /* Col 8 */ { 0x8E,0x2B,0x8E,0x7C,0xBB,0x8F,0x90,0xC0 },
  /* Col 7 */ { 0xF0,0xE8,0x5E,0x59,0xAE,0xD1,0x08,0x9E },
  /* Col 6 */ { 0xB3,0x6F,0x4E,0xBF,0xDB,0x06,0xDA,0xF4 },
  /* Col 5 */ { 0xC9,0x10,0xF2,0xF0,0xF9,0xFA,0x3C,0x0C },
  /* Col 4 */ { 0xAD,0x95,0x78,0xF1,0xDE,0x33,0x14,0x50 },
  /* Col 3 */ { 0xBD,0x6D,0xCA,0x9F,0x30,0x24,0x5D,0x0C },
  /* Col 2 */ { 0xBE,0xB5,0x3D,0xB8,0xBF,0x9D,0x97,0x4C },
  /* Col 1 */ { 0xCA,0x20,0xFF,0xA7,0xA9,0xD0,0x4A,0x8E },
  /* Col 0 */ { 0x97,0x5D,0xD9,0x0F,0xD9,0x32,0x56,0x40 },
},
{ /* Row 3 */
  /* Col 8 */ { 0x40,0xBD,0x5F,0x26,0x31,0xD6,0xE1,0x88 },
  /* Col 7 */ { 0x88,0xC9,0xD4,0x23,0x97,0xFC,0xD1,0x35 },
  /* Col 6 */ { 0x88,0x5A,0x30,0xB7,0xB9,0x98,0x54,0xBF },
  /* Col 5 */ { 0x80,0xB5,0x8D,0x14,0xE0,0xF7,0x75,0x9B },
  /* Col 4 */ { 0xF6,0x05,0x67,0xDA,0x1C,0x9C,0xAE,0x42 },
  /* Col 3 */ { 0xB4,0x36,0x5A,0x80,0x1B,0xE6,0xF2,0xB6 },
  /* Col 2 */ { 0xC1,0x40,0x80,0xD0,0x54,0x49,0x2D,0x7D },
  /* Col 1 */ { 0xE7,0x49,0xF8,0x08,0x80,0x26,0x69,0x80 },
  /* Col 0 */ { 0x8E,0x2B,0x8E,0x7C,0xBB,0x8F,0x90,0xC0 },
},
{ /* Row 4 */
  /* Col 8 */ { 0xF8,0xFE,0xBD,0xEF,0x8A,0x6E,0xBC,0x03 },
  /* Col 7 */ { 0xA1,0xF4,0x45,0x96,0x56,0x3B,0x30,0x5F },
  /* Col 6 */ { 0xB0,0xF1,0x28,0x0E,0xDD,0xB3,0xB5,0x58 },
  /* Col 5 */ { 0xB7,0x4F,0xA5,0x9D,0x5C,0xFE,0xC6,0xF1 },
  /* Col 4 */ { 0x84,0x7D,0x9C,0x46,0xB8,0x9C,0xD5,0x5C },
  /* Col 3 */ { 0xAC,0x8F,0x3E,0xAB,0xFA,0x01,0x83,0xE0 },
  /* Col 2 */ { 0xBA,0x06,0x32,0x0E,0x16,0x01,0x0E,0xC3 },
  /* Col 1 */ { 0x8C,0xAF,0xAE,0x97,0x99,0x08,0x68,0xDC },
  /* Col 0 */ { 0x93,0x40,0xBD,0x5F,0x26,0x31,0xD6,0xE1 },
}};
static const u8 pn_bind[] = { 0x4e,0x57,0xe6,0x48,0xfe,0x22,0x94,0xc6 };
u8 packet[16];
u8 ch[2];
u8 chidx;
u8 sop_col;
u8 data_col;
u8 state;
static u8 cyrfmfg_id[6];
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
    packet[0] = 0xff ^ cyrfmfg_id[2];
    packet[1] = (0xff ^ cyrfmfg_id[3]) + model;
    for (i = 0; i < 7; i++) {
       s32 value = (s32)Channels[upper * 7 + i] * 0x200 / CHAN_MAX_VALUE + 0x200;
       packet[2*i+2] = (value >> 8) & 0xff;
       packet[2*i+3] = (value) & 0xff;
    }
}

static u8 get_pn_row(u8 channel)
{
    return channel % 5;
}

static void cyrf_config()
{
    u8 data_code[32];
    CYRF_WriteRegister(0x1d, 0x01);
    CYRF_WriteRegister(0x28, 0x02);
    CYRF_WriteRegister(0x32, 0x3c);
    CYRF_WriteRegister(0x35, 0x14);
    //0d
    CYRF_WriteRegister(0x0d, 0x40);
    CYRF_WriteRegister(0x06, 0x48);
    CYRF_WriteRegister(0x1b, 0x55);
    CYRF_WriteRegister(0x1c, 0x05);
    CYRF_WriteRegister(0x0f, 0x24);
    CYRF_WriteRegister(0x03, 0x38);
    CYRF_WriteRegister(0x12, 0x0a);
    CYRF_WriteRegister(0x0c, 0x80);
    CYRF_WriteRegister(0x0f, 0x04);
    CYRF_WriteRegister(0x39, 0x01);
    CYRF_WritePreamble(0x333304);
    //CYRF_ConfigRFChannel(0x61);
    //CYRF_WriteRegister(0x85, 0x83); //setup read
    //0x13, 0x20 //poll RSSI
    //CYRF_WriteRegister(0x92, 0x3f); //set pn correlation threshold
    //CYRF_WriteRegister(0x90, 0x7f); //disable sop
    //CYRF_WriteRegister(0x85, 0x83); //setupo read
    //0x13, 0x20 //poll RSSI
    //0x09, 0x0f //15 bytes in queue?
    //0x21, 0xf7 ee af f9 f6 a5 57 28 74 6b 84 64 c4 bb 84 //read 15 bytes
    CYRF_WriteRegister(0x0f, 0x24); //Force IDLE
    //0x0f, 0x04 //Read state (Idle)
    CYRF_WriteRegister(0x29, 0x00); //Clear RX abort
    CYRF_WriteRegister(0x12, 0x0a); //set pn correlation threshold
    CYRF_WriteRegister(0x10, 0x4a); //set sop len and threshold
    CYRF_WriteRegister(0x29, 0x0f); //Clear RX abort?
    CYRF_WriteRegister(0x03, 0x38); //Set 64chip, SDE mode
    CYRF_WriteRegister(0x10, 0x4a); //set sop len and threshold
    CYRF_WriteRegister(0x1f, 0x04); //disable tx CRC
    CYRF_WriteRegister(0x1e, 0x14); //disable rx crc
    CYRF_WriteRegister(0x14, 0x02); //set EOP sync == 2
    CYRF_ConfigRFChannel(BIND_CHANNEL); //This seems to be random?
    u8 pn_row = get_pn_row(BIND_CHANNEL);
    printf("Ch: %d Row: %d SOP: %d Data: %d\n", BIND_CHANNEL, pn_row, sop_col, data_col);
    CYRF_ConfigCRCSeed(crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    memcpy(data_code, pn_bind, 8);
    memcpy(data_code + 8, pncodes[0][8], 8);
    memcpy(data_code + 16, pncodes[pn_row][data_col], 16);
    CYRF_ConfigDataCode(data_code, 32);
    CYRF_WriteRegister(0x01, 0x10); //16byte packet
    build_bind_packet();
}

static void cyrf_configdata()
{
//Initialize for reading RSSI
    CYRF_WriteRegister(0x05, 0x83);
//0x13, 0xa6
    CYRF_WriteRegister(0x29, 0x20);
//0x13, 0x20
    CYRF_WriteRegister(0x0f, 0x24);
//0x0f, 0x04
    CYRF_WriteRegister(0x29, 0x00);
    CYRF_WriteRegister(0x03, 0x0d);
    CYRF_WriteRegister(0x10, 0xea);
    CYRF_WriteRegister(0x1f, 0x00);
    CYRF_WriteRegister(0x1e, 0x00);
    CYRF_WriteRegister(0x03, 0x2d);
    CYRF_WriteRegister(0x12, 0x3f);
    CYRF_WriteRegister(0x10, 0xff);
//Switch from reading RSSI to Writing
    CYRF_WriteRegister(0x0f, 0x24);
//0x0f, 0x04
    CYRF_WriteRegister(0x29, 0x00);
    CYRF_WriteRegister(0x12, 0x0a);
    CYRF_WriteRegister(0x10, 0xea);
}

static void set_sop_data_crc()
{
    u8 pn_row = get_pn_row(ch[chidx]);
    printf("Ch: %d Row: %d SOP: %d Data: %d\n", ch[chidx], pn_row, sop_col, data_col);
    CYRF_ConfigCRCSeed(chidx ? ~crc : crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    CYRF_ConfigDataCode(pncodes[pn_row][data_col], 16);
}

static u16 dsm2_cb()
{
    if(state < DSM2_CHANSEL) {
        CYRF_WriteDataPacket(packet);
        state += 1;
        return 10000;
    } else if(state < DSM2_CHANSEL) {
        //FIXME: Select channels here
        ch[0] = 0x35;
        ch[1] = 0x0e;
        cyrf_configdata();
        chidx = 0;
        set_sop_data_crc();
        state = DSM2_CH1_WRITE_A;
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
    CYRF_GetMfgData(cyrfmfg_id);
    crc = ~((cyrfmfg_id[0] << 8) + cyrfmfg_id[1]); 
    data_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
    sop_col = 8 - data_col;
    num_channels = 6;
    model = MODEL;

    cyrf_config();
    state = DSM2_BIND;
    CLOCK_StartTimer(10000, dsm2_cb);
}
#endif
