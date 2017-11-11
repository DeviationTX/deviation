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
  #define FLYSKY_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_A7105

//Fewer bind packets in the emulator so we can get right to the important bits
#ifdef EMULATOR
#define BIND_COUNT 3
#else
#define BIND_COUNT 2500
#endif

#define PACKET_PERIOD_FLYSKY 1510UL
#define PACKET_PERIOD_CX20 3984UL

static const char * const flysky_opts[] = {
  "WLToys ext.",  _tr_noop("Off"), "V9x9", "V6x6", "V912", "CX20", NULL,
  "Freq Tune", "-300", "300", "655361", NULL, // big step 10, little step 1
  NULL
};
enum {
    PROTOOPTS_WLTOYS = 0,
    PROTOOPTS_FREQTUNE,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define WLTOYS_EXT_OFF 0
#define WLTOYS_EXT_V9X9 1
#define WLTOYS_EXT_V6X6 2
#define WLTOYS_EXT_V912 3
#define WLTOYS_EXT_CX20 4

FLASHBYTETABLE A7105_regs[] = {
    0xFF, 0x42, 0x00, 0x14, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x00, 0x50,
    0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,
    0x13, 0xc3, 0x00, 0xFF, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,
    0x01, 0x0f, 0xFF,
};
FLASHBYTETABLE tx_channels[8][4] = {
    { 0x12, 0x34, 0x56, 0x78},
    { 0x18, 0x27, 0x36, 0x45},
    { 0x41, 0x82, 0x36, 0x57},
    { 0x84, 0x13, 0x65, 0x72},
    { 0x87, 0x64, 0x15, 0x32},
    { 0x76, 0x84, 0x13, 0x52},
    { 0x71, 0x62, 0x84, 0x35},
    { 0x71, 0x86, 0x43, 0x52}
};

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
    CHANNEL7,
    CHANNEL8,
    CHANNEL9,
    CHANNEL10,
    CHANNEL11,
    CHANNEL12,
};

enum {
    // flags going to byte 10
    FLAG_V9X9_VIDEO = 0x40,
    FLAG_V9X9_CAMERA= 0x80,
    // flags going to byte 12
    FLAG_V9X9_UNK   = 0x10, // undocumented ?
    FLAG_V9X9_LED   = 0x20,
};

enum {
    // flags going to byte 13
    FLAG_V6X6_HLESS1= 0x80,
    // flags going to byte 14
    FLAG_V6X6_VIDEO = 0x01,
    FLAG_V6X6_YCAL  = 0x02,
    FLAG_V6X6_XCAL  = 0x04,
    FLAG_V6X6_RTH   = 0x08,
    FLAG_V6X6_CAMERA= 0x10,
    FLAG_V6X6_HLESS2= 0x20,
    FLAG_V6X6_LED   = 0x40,
    FLAG_V6X6_FLIP  = 0x80,
};

enum {
    // flags going to byte 14
    FLAG_V912_TOPBTN= 0x40,
    FLAG_V912_BTMBTN= 0x80,
};

static u32 id;
static u8 packet[21];
static u16 counter;
static u16 packet_period;
static u8 hopping_frequency[16];
static u8 hopping_frequency_no;
static u8 tx_power;
static s16 freq_offset;

static int flysky_init()
{
    int i;
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;
    u8 reg;

    A7105_WriteID(0x5475c52a);
    for (i = 0; i < 0x33; i++)
    {
        reg = pgm_read_byte(&A7105_regs[i]);
        if(Model.proto_opts[PROTOOPTS_WLTOYS] == WLTOYS_EXT_CX20) {
            if(i==0x0E) reg=0x01;
            if(i==0x1F) reg=0x1F;
            if(i==0x20) reg=0x1E;
        }
        if( reg != 0xFF)
            A7105_WriteReg(i, reg);
    }
    A7105_Strobe(A7105_STANDBY);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    A7105_ReadReg(0x02);
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    if_calibration1 = A7105_ReadReg(0x22);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //VCO Current Calibration
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet

    //VCO Bank Calibration
    A7105_WriteReg(0x26, 0x3b); //Recomended limits from A7105 Datasheet

    //VCO Bank Calibrate channel 0?
    //Set Channel
    A7105_WriteReg(0x0f, 0); //Should we choose a different channel?
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration0 = A7105_ReadReg(0x25);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Calibrate channel 0xa0?
    //Set Channel
    A7105_WriteReg(0x0f, 0xa0); //Should we choose a different channel?
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(A7105_02_CALC))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration1 = A7105_ReadReg(0x25);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Reset VCO Band calibration
    A7105_WriteReg(0x25, 0x08);

    A7105_SetTxRxMode(TX_EN);
    A7105_SetPower(Model.tx_power);

    A7105_Strobe(A7105_STANDBY);
    return 1;
}

static void flysky_apply_extension_flags()
{
    const u8 V912_X17_SEQ[10] = { 0x14, 0x31, 0x40, 0x49, 0x49,    // sometime first byte is 0x15 ?
                                  0x49, 0x49, 0x49, 0x49, 0x49, }; 
    static u8 seq_counter;
    switch(Model.proto_opts[PROTOOPTS_WLTOYS]) {
        case WLTOYS_EXT_V9X9:
            if(Channels[CHANNEL5] > 0)
                packet[12] |= FLAG_V9X9_LED;
            if(Channels[CHANNEL6] > 0)
                packet[10] |= FLAG_V9X9_VIDEO;
            if(Channels[CHANNEL7] > 0)
                packet[10] |= FLAG_V9X9_CAMERA;
            if(Channels[CHANNEL8] > 0)
                packet[12] |= FLAG_V9X9_UNK;
            break;
            
        case WLTOYS_EXT_V6X6:
            packet[13] = 0x03; // 3 = 100% rate (0=40%, 1=60%, 2=80%)
            packet[14] = 0x00;
            if(Channels[CHANNEL5] > 0) 
                packet[14] |= FLAG_V6X6_LED;
            if(Channels[CHANNEL6] > 0) 
                packet[14] |= FLAG_V6X6_FLIP;
            if(Channels[CHANNEL7] > 0) 
                packet[14] |= FLAG_V6X6_CAMERA;
            if(Channels[CHANNEL8] > 0) 
                packet[14] |= FLAG_V6X6_VIDEO;
            if(Model.num_channels >= 9 && Channels[CHANNEL9] > 0) { 
                packet[13] |= FLAG_V6X6_HLESS1;
                packet[14] |= FLAG_V6X6_HLESS2;
            }
            if(Model.num_channels >= 10 && Channels[CHANNEL10] > 0) 
                packet[14] |= FLAG_V6X6_RTH;
            if(Model.num_channels >= 11 && Channels[CHANNEL11] > 0) 
                packet[14] |= FLAG_V6X6_XCAL;
            if(Model.num_channels >= 12 && Channels[CHANNEL12] > 0) 
                packet[14] |= FLAG_V6X6_YCAL;
            packet[15] = 0x10; // unknown
            packet[16] = 0x10; // unknown
            packet[17] = 0xAA; // unknown
            packet[18] = 0xAA; // unknown
            packet[19] = 0x60; // unknown, changes at irregular interval in stock TX
            packet[20] = 0x02; // unknown
            break;
            
        case WLTOYS_EXT_V912:
            seq_counter++;
            if( seq_counter > 9)
                seq_counter = 0;
            packet[12] |= 0x20; // bit 6 is always set ?
            packet[13] = 0x00;  // unknown
            packet[14] = 0x00;
            if(Channels[CHANNEL5] > 0)
                packet[14] |= FLAG_V912_BTMBTN;
            if(Channels[CHANNEL6] > 0)
                packet[14] |= FLAG_V912_TOPBTN;
            packet[15] = 0x27; // [15] and [16] apparently hold an analog channel with a value lower than 1000
            packet[16] = 0x03; // maybe it's there for a pitch channel for a CP copter ?
            packet[17] = V912_X17_SEQ[seq_counter]; // not sure what [17] & [18] are for
            if(seq_counter == 0)                    // V912 Rx does not even read those bytes... [17-20]
                packet[18] = 0x02;
            else
                packet[18] = 0x00;
            packet[19] = 0x00; // unknown
            packet[20] = 0x00; // unknown
            break;
            
        case WLTOYS_EXT_CX20:
            packet[19] = 00; // unknown
            packet[20] = (hopping_frequency_no<<4)|0x0A;
            break;
            
        default:
            break; 
    }
}

static void flysky_build_packet(u8 init)
{
    int i;
    //-100% =~ 0x03e8
    //+100% =~ 0x07ca
    //Calculate:
    //Center = 0x5d9
    //1 %    = 5
    packet[0] = init ? 0xaa : 0x55; 
    packet[1] = (id >>  0) & 0xff;
    packet[2] = (id >>  8) & 0xff;
    packet[3] = (id >> 16) & 0xff;
    packet[4] = (id >> 24) & 0xff;
    for (i = 0; i < 8; i++) {
        if (i > Model.num_channels) {
            packet[5 + i*2] = 0;
            packet[6 + i*2] = 0;
            continue;
        }
        s32 value = (s32)Channels[i] * 0x1f1 / CHAN_MAX_VALUE + 0x5d9;
        if(Model.proto_opts[PROTOOPTS_WLTOYS] == WLTOYS_EXT_CX20 && i == 1) // reverse elevator
            value = 3000 - value;
        if (value < 0)
            value = 0;
        packet[5 + i*2] = value & 0xff;
        packet[6 + i*2] = (value >> 8) & 0xff;
    }
    flysky_apply_extension_flags();
}

MODULE_CALLTYPE
static u16 flysky_cb()
{
    if (counter) {
        flysky_build_packet(1);
        A7105_WriteData(packet, 21, 1);
        counter--;
        if (! counter)
            PROTOCOL_SetBindState(0);
    } else {
        flysky_build_packet(0);
        A7105_WriteData(packet, 21, hopping_frequency[hopping_frequency_no & 0x0F]);
        //Keep transmit power updated
        if(tx_power != Model.tx_power) {
            A7105_SetPower(Model.tx_power);
            tx_power = Model.tx_power;
        }
        // keep frequency tuning updated
        if(freq_offset != Model.proto_opts[PROTOOPTS_FREQTUNE]) {
                freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
                A7105_AdjustLOBaseFreq(freq_offset);
        }
    }
    hopping_frequency_no++;
    return packet_period;
}

static void initialize(u8 bind) {
    uint8_t chanrow;
    uint8_t chanoffset;
    uint8_t temp;

    CLOCK_StopTimer();
    if(Model.proto_opts[PROTOOPTS_WLTOYS] == WLTOYS_EXT_CX20) {
        packet_period = PACKET_PERIOD_CX20;
    } else {
        packet_period = PACKET_PERIOD_FLYSKY;
    }
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (flysky_init())
            break;
    }
    if (Model.fixed_id) {
        id = Model.fixed_id;
    } else {
        id = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) % 999999;
    }
    if ((id & 0xf0) > 0x90) // limit offset to 9 as higher values don't work with some RX (ie V912)
        id = id - 0x70;
    chanrow = id % 16;
    chanoffset = (id & 0xff) / 16;
    
    for(uint8_t i=0;i<16;i++)
    {
        temp=pgm_read_byte(&tx_channels[chanrow>>1][i>>2]);
        if(i&0x02)
            temp&=0x0F;
        else
            temp>>=4;
        temp*=0x0A;
        if(i&0x01)
            temp+=0x50;
        if(Model.proto_opts[PROTOOPTS_WLTOYS] == WLTOYS_EXT_CX20)
        {
            if(temp==0x0A)
                temp+=0x37;
            if(temp==0xA0)
            {
                if (chanoffset<4)
                    temp=0x37;
                else if (chanoffset<9)
                    temp=0x2D;
                else
                    temp=0x29;
            }
        }
        hopping_frequency[((chanrow&1)?15-i:i)]=temp-chanoffset;
    }
    
    tx_power = Model.tx_power;
    A7105_SetPower(Model.tx_power);
    freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
    A7105_AdjustLOBaseFreq(freq_offset);
    hopping_frequency_no = 0;
    if (bind || ! Model.fixed_id) {
        counter = BIND_COUNT;
        PROTOCOL_SetBindState(2500 * packet_period / 1000); //msec
    } else {
        counter = 0;
    }
    
    CLOCK_StartTimer(2400, flysky_cb);
}

const void *FLYSKY_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(A7105_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return Model.fixed_id ? 0 : (void *)1L;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return (void *)((unsigned long)id);
        case PROTOCMD_GETOPTIONS:
            return flysky_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
