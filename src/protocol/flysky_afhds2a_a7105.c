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
  #define AFHDS2A_Cmds PROTO_Cmds
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

static const u8 AFHDS2A_TXID[4] = {0x0f, 0x6b, 0xd0, 0xb2};
static const u8 AFHDS2A_RXID[4] = {0x25, 0x27, 0x9B, 0x3B};

static const u8 AFHDS2A_CH[16] = {0x86,0x19,0x28,0x63,0x92,0x5d,0x4c,0x8b,0x2e,0x24,0x1f,0x45,0x14,0x37,0x96,0x0f};

static const u8 AFHDS2A_regs[] = {
    -1  , 0x42 | (1<<5), 0x00, 0x25, 0x00,   -1,   -1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3c, 0x05, 0x00, 0x50, // 00 - 0f
    0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x4f, 0x62, 0x80,   -1,   -1, 0x2a, 0x32, 0xc3, 0x1f, // 10 - 1f
    0x1e,   -1, 0x00,   -1, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00, // 20 - 2f
    0x01, 0x0f                                                                                      // 30 - 31
};

enum{
    AFHDS2A_DATA1,  
    AFHDS2A_DATA2,
    AFHDS2A_DATA3,
};

static u8 AFHDS2A_state;
static u8 AFHDS2A_channel;
static u32 id;
static u8 packet[38];

static const char * const afhds2a_opts[] = {
    _tr_noop("PPM"), _tr_noop("Off"), _tr_noop("On"), NULL,
    _tr_noop("Servo Hz"), "50", "400", "5", NULL,
    NULL
};

enum {
    PROTOOPTS_PPM = 0,
    PROTOOPTS_SERVO_HZ,
    LAST_PROTO_OPT,
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

static int afhds2a_init()
{
    int i;
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;

    A7105_WriteID(0x5475c52a);
    for (i = 0; i < 0x33; i++)
        if((s8)AFHDS2A_regs[i] != -1)
            A7105_WriteReg(i, AFHDS2A_regs[i]);

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
    A7105_WriteReg(0x25, 0x0A);

    A7105_SetTxRxMode(TX_EN);
    A7105_SetPower(Model.tx_power);

    A7105_Strobe(A7105_STANDBY);
    return 1;
}

void afhds2a_build_packet()
{
    packet[0] = 0x58;
    memcpy( &packet[1], AFHDS2A_TXID, 4);
    memcpy( &packet[5], AFHDS2A_RXID, 4);
    for(u8 i=0; i<14; i++)
    {
        if (i >= Model.num_channels) {
            packet[9 + i*2] = 0xdc;
            packet[10 + i*2] = 0x05;
            continue;
        }
        s32 value = (s32)Channels[i] * 0x1f1 / CHAN_MAX_VALUE + 0x5d9;
        if (value < 950 || value > 2050)
            value = 1500;
        packet[9 +  i*2] = value & 0xff;
        packet[10 + i*2] = (value >> 8) & 0xff;
    }
    packet[37] = 0x00;
}

void afhds2a_build_option_packet()
{
    packet[0] = 0xaa;
    memcpy( &packet[1], AFHDS2A_TXID, 4);
    memcpy( &packet[5], AFHDS2A_RXID, 4);
    packet[9] = 0xfd;
    packet[10]= 0xff;
    packet[11]= Model.proto_opts[PROTOOPTS_SERVO_HZ] & 0xff;
    packet[12]= (Model.proto_opts[PROTOOPTS_SERVO_HZ] >> 8) & 0xff;
    packet[13]= Model.proto_opts[PROTOOPTS_PPM];
    packet[14]= 0x00;
    for(u8 i=15; i<37; i++)
        packet[i] = 0xff;
    packet[37] = 0x00;
}

static void afhds2a_update_telemetry()
{
    const u8 *update = NULL;
    static const u8 telempkt[] = { TELEM_FRSKY_VOLT1, TELEM_FRSKY_RSSI, 0 };
    
    // todo: check txid & rxid
    // todo: check sensor type and ID
    Telemetry.value[TELEM_FRSKY_VOLT1] = ((u16)(packet[12]<<8) | (packet[11] & 0xff)); // RX voltage
    Telemetry.value[TELEM_FRSKY_RSSI] = 100 - packet[15]; // % droped / missed packets
    update = telempkt;
    
    if (update) {
        while(*update) {
            TELEMETRY_SetUpdated(*update++);
        }
    }
}

u16 afhds2a_cb()
{
    static u8 state=0;
    switch(AFHDS2A_state) {
        case AFHDS2A_DATA1:
            if(state==1)
                afhds2a_build_option_packet();
            else
                afhds2a_build_packet();
            A7105_Strobe(A7105_STANDBY);
            A7105_SetTxRxMode(TX_EN);
            A7105_WriteData(packet, 38, AFHDS2A_CH[AFHDS2A_channel++]);
            if(AFHDS2A_channel >= 16)
                AFHDS2A_channel = 0;
            
            state = 0;
            
            // got some data from RX ?
            if(A7105_ReadReg(0) == 0x1b) {
                A7105_Strobe(A7105_RST_RDPTR);
                A7105_ReadData(packet, 37);
                if(packet[0] == 0xaa && packet[9] == 0xfc) { // rx is asking for options ?
                    state=1;
                    printf("setup\n");
                }
                else if(packet[0] == 0xaa) {
                    afhds2a_update_telemetry();
                    //printf("%02x %02x\n", packet[11], packet[12]);
                }
            }
            
            AFHDS2A_state = AFHDS2A_DATA2;
            return 1700;
            
        case AFHDS2A_DATA2:
            A7105_SetTxRxMode(RX_EN);
            A7105_Strobe(A7105_RX);
            AFHDS2A_state = AFHDS2A_DATA1;
            return 2150;
    }
    return 1000; // just to please the compiler
}

void initialize(u8 bind)
{
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (afhds2a_init())
            break;
    }
    AFHDS2A_state = AFHDS2A_DATA1;
    AFHDS2A_channel = 0;
    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_FRSKY);
    printf("\n\nAFHDS2A initialize OK\n");
    CLOCK_StartTimer(2400, afhds2a_cb);
}

const void *AFHDS2A_Cmds(enum ProtoCmds cmd)
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
            if( Model.proto_opts[PROTOOPTS_SERVO_HZ] < 50 || Model.proto_opts[PROTOOPTS_SERVO_HZ] > 400)
                Model.proto_opts[PROTOOPTS_SERVO_HZ] = 50;
            return afhds2a_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_ON;
        default: break;
    }
    return 0;
}

#endif