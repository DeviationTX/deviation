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
  #define HUBSAN_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include <string.h>
#include <stdlib.h>
#include "telemetry.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#endif

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif
#ifdef PROTO_HAS_A7105

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Leds
    CHANNEL6,     // Flip (or Altitude Hold on H501S)
    CHANNEL7,     // Still camera
    CHANNEL8,     // Video camera
    CHANNEL9,     // Headless
    CHANNEL10,    // RTH
    CHANNEL11,    // GPS HOLD
};

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_ALT_HOLD    CHANNEL6
#define CHANNEL_SNAPSHOT    CHANNEL7
#define CHANNEL_VIDEO       CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10
#define CHANNEL_GPS_HOLD    CHANNEL11

enum{
    // flags going to packet[9] (H107)
    FLAG_VIDEO= 0x01,   // record video
    FLAG_FLIP = 0x08,   // enable flips
    FLAG_LED  = 0x04    // enable LEDs
};

enum{
    // flags going to packet[9] (H107 Plus series)
    FLAG_HEADLESS = 0x08, // headless mode
};

enum{
    // flags going to packet[13] (H107 Plus series)
    FLAG_SNAPSHOT  = 0x01,
    FLAG_FLIP_PLUS = 0x80,
};

enum{
    // flags going to packet[9] (H501S)
    FLAG_H501_VIDEO     = 0x01,
    FLAG_H501_LED       = 0x04,
    FLAG_H501_RTH       = 0x20,
    FLAG_H501_HEADLESS1 = 0x40,
    FLAG_H501_GPS_HOLD  = 0x80,
};

enum{
    // flags going to packet[13] (H501S)
    FLAG_H501_SNAPSHOT  = 0x01,
    FLAG_H501_HEADLESS2 = 0x02,
    FLAG_H501_ALT_HOLD  = 0x08,
};

#define VTX_STEP_SIZE "5"

static const char * const hubsan4_opts[] = {
    _tr_noop("Format"), "H107", "H501", NULL,
    _tr_noop("vTX MHz"),  "5645", "5945", VTX_STEP_SIZE, NULL,
    _tr_noop("Telemetry"),  _tr_noop("On"), _tr_noop("Off"), NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT,
    PROTOOPTS_VTX_FREQ,
    PROTOOPTS_TELEMETRY,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum {
    FORMAT_H107 = 0,
    FORMAT_H501,
};

enum {
    TELEM_ON = 0,
    TELEM_OFF,
};

#define ID_NORMAL 0x55201041 // H102D, H107/L/C/D, H501S
#define ID_PLUS   0xAA201041 // H107P/C+/D+

static u8 packet[16];
static u8 channel;
static s16 vtx_freq;
static const u8 allowed_ch[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};
static u32 sessionid;
static const u32 txid = 0xdb042679;
static u8 state;
static u8 packet_count;
static u8 bind_count;
static u32 id_data;

enum {
    BIND_1,
    BIND_2,
    BIND_3,
    BIND_4,
    BIND_5,
    BIND_6,
    BIND_7,
    BIND_8,
    DATA_1,
    DATA_2,
    DATA_3,
    DATA_4,
    DATA_5,
};
#define WAIT_WRITE 0x80

static int hubsan_init()
{
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;
    
    A7105_WriteID(ID_NORMAL);
    A7105_WriteReg(A7105_01_MODE_CONTROL, 0x63);
    A7105_WriteReg(A7105_03_FIFOI, 0x0f);
    A7105_WriteReg(A7105_0D_CLOCK, 0x05);
    A7105_WriteReg(A7105_0E_DATA_RATE, 0x04);
    A7105_WriteReg(A7105_15_TX_II, 0x2b);
    A7105_WriteReg(A7105_18_RX, 0x62);
    A7105_WriteReg(A7105_19_RX_GAIN_I, 0x80);
    A7105_WriteReg(A7105_1C_RX_GAIN_IV, 0x0A);
    A7105_WriteReg(A7105_1F_CODE_I, 0x07);
    A7105_WriteReg(A7105_20_CODE_II, 0x17);
    A7105_WriteReg(A7105_29_RX_DEM_TEST_I, 0x47);

    A7105_Strobe(A7105_STANDBY);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    //vco_current =
    A7105_ReadReg(0x02);
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    if_calibration1 = A7105_ReadReg(A7105_22_IF_CALIB_I);
    A7105_ReadReg(A7105_24_VCO_CURCAL);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //VCO Current Calibration
    //A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet

    //VCO Bank Calibration
    //A7105_WriteReg(0x26, 0x3b); //Recomended limits from A7105 Datasheet

    //VCO Bank Calibrate channel 0?
    //Set Channel
    A7105_WriteReg(A7105_0F_CHANNEL, 0);
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
    vco_calibration0 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Calibrate channel 0xa0?
    //Set Channel
    A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
    //VCO Calibration
    A7105_WriteReg(A7105_02_CALC, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(A7105_02_CALC))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration1 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
    }

    //Reset VCO Band calibration
    //A7105_WriteReg(0x25, 0x08);
    A7105_SetTxRxMode(TX_EN);

    A7105_SetPower(Model.tx_power);


    A7105_Strobe(A7105_STANDBY);
    return 1;
}

static void update_crc()
{
    int sum = 0;
    for(int i = 0; i < 15; i++)
        sum += packet[i];
    packet[15] = (256 - (sum % 256)) & 0xff;
}
static void hubsan_build_bind_packet(u8 bind_state)
{
    static u8 handshake_counter;
    if(state < BIND_7)
        handshake_counter = 0;
    memset(packet, 0, 16);
    packet[0] = bind_state;
    packet[1] = channel;
    packet[2] = (sessionid >> 24) & 0xff;
    packet[3] = (sessionid >> 16) & 0xff;
    packet[4] = (sessionid >>  8) & 0xff;
    packet[5] = (sessionid >>  0) & 0xff;
    if(id_data == ID_NORMAL && Model.proto_opts[PROTOOPTS_FORMAT] != FORMAT_H501) {
        packet[6] = 0x08;
        packet[7] = 0xe4; //???
        packet[8] = 0xea;
        packet[9] = 0x9e;
        packet[10] = 0x50;
        packet[11] = (txid >> 24) & 0xff;
        packet[12] = (txid >> 16) & 0xff;
        packet[13] = (txid >>  8) & 0xff;
        packet[14] = (txid >>  0) & 0xff;
    }
    else if(id_data == ID_PLUS || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_H501) {
        if(state >= BIND_3) {
            packet[7] = 0x62;
            packet[8] = 0x16;
        }
        if(state == BIND_7) {
            packet[2] = handshake_counter++;
        }
    }
    update_crc();
}

static s16 get_channel(u8 ch, s32 scale, s32 center, s32 range)
{
    s32 value = (s32)Channels[ch] * scale / CHAN_MAX_VALUE + center;
    if (value < center - range)
        value = center - range;
    if (value >= center + range)
        value = center + range -1;
    return value;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

static void hubsan_build_packet()
{
    static u32 h501_packet = 0;
    memset(packet, 0, 16);
    if(vtx_freq != Model.proto_opts[PROTOOPTS_VTX_FREQ] || packet_count==100) { // set vTX frequency
        vtx_freq = Model.proto_opts[PROTOOPTS_VTX_FREQ];
        packet[0] = 0x40; // vtx data packet
        packet[1] = (vtx_freq >> 8) & 0xff;
        packet[2] = vtx_freq & 0xff;
        packet[3] = 0x82;
        packet_count++;
        h501_packet = 0;
    } else {
        packet[0] = 0x20; // normal data packet
        packet[2] = get_channel(2, 0x80, 0x80, 0x80); //Throttle
    }
    packet[4] = 0xff - get_channel(3, 0x80, 0x80, 0x80); //Rudder is reversed
    packet[6] = 0xff - get_channel(1, 0x80, 0x80, 0x80); //Elevator is reversed
    packet[8] = get_channel(0, 0x80, 0x80, 0x80); //Aileron 
    
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_H501) { // H501S
        packet[9] = 0x02
                  | GET_FLAG(CHANNEL_LED, FLAG_H501_LED)
                  | GET_FLAG(CHANNEL_VIDEO, FLAG_H501_VIDEO)
                  | GET_FLAG(CHANNEL_RTH, FLAG_H501_RTH)
                  | GET_FLAG(CHANNEL_GPS_HOLD, FLAG_H501_GPS_HOLD)
                  | GET_FLAG(CHANNEL_HEADLESS, FLAG_H501_HEADLESS1);
        packet[10] = 0x1a; // 0x19 sometimes, seems to be ignored by rx
        packet[13] = GET_FLAG(CHANNEL_HEADLESS, FLAG_H501_HEADLESS2)
                   | GET_FLAG(CHANNEL_ALT_HOLD, FLAG_H501_ALT_HOLD)
                   | GET_FLAG(CHANNEL_SNAPSHOT, FLAG_H501_SNAPSHOT);
        h501_packet++;
        if(h501_packet == 10) {
            memset(packet, 0, 16);
            packet[0] = 0xe8;
        }
        else if(h501_packet == 20) {
            memset(packet, 0, 16);
            packet[0] = 0xe9;
        }
        if(h501_packet >= 20) h501_packet = 0;
    }
    else if(id_data == ID_NORMAL) { // H107/L/C/D, H102D
        if(packet_count < 100) {
            packet[9] = 0x02 | FLAG_LED | FLAG_FLIP; // sends default value for the 100 first packets
            packet_count++;
        } else {
            packet[9] = 0x02
                      | GET_FLAG(CHANNEL_LED, FLAG_LED)
                      | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP)
                      | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO); // H102D
        }
        packet[10] = 0x64;
        packet[11] = (txid >> 24) & 0xff;
        packet[12] = (txid >> 16) & 0xff;
        packet[13] = (txid >>  8) & 0xff;
        packet[14] = (txid >>  0) & 0xff;
    }
    else if(id_data == ID_PLUS) { // H107P/C+/D+
        packet[3] = 0x64;
        packet[5] = 0x64;
        packet[7] = 0x64;
        packet[9] = 0x06
                  | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO)
                  | GET_FLAG(CHANNEL_HEADLESS, FLAG_HEADLESS);
        packet[10]= 0x19;
        packet[12]= 0x5C; // ghost channel ?
        packet[13] = GET_FLAG(CHANNEL_SNAPSHOT, FLAG_SNAPSHOT)
                   | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP_PLUS);
        packet[14]= 0x49; // ghost channel ?
        if(packet_count < 100) { // set channels to neutral for first 100 packets
            packet[2] = 0x80; // throttle neutral is at mid stick on plus series
            packet[4] = 0x80;
            packet[6] = 0x80;
            packet[8] = 0x80;
            packet[9] = 0x06;
            packet[13]= 0x00;
            packet_count++;
        }
    }
    update_crc();
}

static u8 hubsan_check_integrity() 
{
    int sum = 0;
    for(int i = 0; i < 15; i++)
        sum += packet[i];
    return packet[15] == ((256 - (sum % 256)) & 0xff);
}

static void hubsan_update_telemetry()
{
    const u8 *update = NULL;
    static const u8 telempkt[] = { TELEM_DEVO_VOLT1, 0 };
    if( ((packet[0]==0xe1 || packet[0]==0xe7)) && hubsan_check_integrity()) {
        Telemetry.value[TELEM_DEVO_VOLT1] = packet[13];
        update = telempkt;
    }
    if (update) {
        while(*update) {
            TELEMETRY_SetUpdated(*update++);
        }
    }
}

MODULE_CALLTYPE
static u16 hubsan_cb()
{
    static u8 txState = 0;
    static int delay = 0;
    static u8 rfMode=0;
    int i;
    switch(state) {
    case BIND_1:
        bind_count++;
        if(bind_count == 20 && Model.proto_opts[PROTOOPTS_FORMAT] != FORMAT_H501) {
            if(id_data == ID_NORMAL)
                id_data = ID_PLUS;
            else
                id_data = ID_NORMAL;
            A7105_WriteID(id_data);    
            bind_count = 0;
        }
    case BIND_3:
    case BIND_5:
    case BIND_7:
        hubsan_build_bind_packet(state == BIND_7 ? 9 : (state == BIND_5 ? 1 : state + 1 - BIND_1));
        A7105_Strobe(A7105_STANDBY);
        A7105_WriteData(packet, 16, channel);
        state |= WAIT_WRITE;
        return 3000;
    case BIND_1 | WAIT_WRITE:
    case BIND_3 | WAIT_WRITE:
    case BIND_5 | WAIT_WRITE:
    case BIND_7 | WAIT_WRITE:
        //wait for completion
        for(i = 0; i< 20; i++) {
           if(! (A7105_ReadReg(A7105_00_MODE) & 0x01))
               break;
        }
        A7105_SetTxRxMode(RX_EN);
        A7105_Strobe(A7105_RX);
        state &= ~WAIT_WRITE;
        if(id_data == ID_PLUS) {
            if(state == BIND_7 && packet[2] == 9) {
                state = DATA_1;
                A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
                PROTOCOL_SetBindState(0);
                return 4500;
            }
        }
        state++;
        return 4500; //7.5msec elapsed since last write
    case BIND_2:
    case BIND_4:
    case BIND_6:
        A7105_SetTxRxMode(TX_EN);
        if(A7105_ReadReg(A7105_00_MODE) & 0x01) {
            state = BIND_1;
            return 4500; //No signal, restart binding procedure.  12msec elapsed since last write
        }
        A7105_ReadData(packet, 16);
        state++;
        if (state == BIND_5)
            A7105_WriteID((packet[2] << 24) | (packet[3] << 16) | (packet[4] << 8) | packet[5]);
        
        return 500;  //8msec elapsed time since last write;
    case BIND_8:
        A7105_SetTxRxMode(TX_EN);
        if(A7105_ReadReg(A7105_00_MODE) & 0x01) {
            state = BIND_7;
            return 15000; //22.5msec elapsed since last write
        }
        A7105_ReadData(packet, 16);
        if(packet[1] == 9 && id_data == ID_NORMAL) {
            state = DATA_1;
            A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
            PROTOCOL_SetBindState(0);
            return 28000; //35.5msec elapsed since last write
        } else {
            state = BIND_7;
            return 15000; //22.5 msec elapsed since last write
        }
    case DATA_1:
    case DATA_2:
    case DATA_3:
    case DATA_4:
    case DATA_5:
        if( txState == 0) { // send packet
            rfMode = A7105_TX;
            if( state == DATA_1)
                A7105_SetPower( Model.tx_power); //Keep transmit power in sync
            hubsan_build_packet();
            A7105_Strobe(A7105_STANDBY);
            u8 ch;
            if((state == DATA_5 && id_data == ID_NORMAL) && Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_H107) {
                ch = channel + 0x23;
            }
            else {
                ch = channel;
            }
            A7105_WriteData( packet, 16, ch);
            if (state == DATA_5)
                state = DATA_1;
            else
                state++;
            delay=3000;
        }
        else {
            if( Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON) {
                if( rfMode == A7105_TX) {// switch to rx mode 3ms after packet sent
                    for( i=0; i<10; i++)
                    {
                        if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)) {// wait for tx completion
                            A7105_SetTxRxMode(RX_EN);
                            A7105_Strobe(A7105_RX); 
                            rfMode = A7105_RX;
                            break;
                        }
                    }
                }
                if( rfMode == A7105_RX) { // check for telemetry frame
                    for( i=0; i<10; i++) {
                        if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)) { // data received
                            A7105_ReadData(packet, 16);
                            hubsan_update_telemetry();
                            A7105_Strobe(A7105_RX);
                            break;
                        }
                    }
                }
            }
            delay=1000;
        }
        if (++txState == 8) { // 3ms + 7*1ms
            A7105_SetTxRxMode(TX_EN);
            txState = 0;
        }
        return delay;
    }
    return 0;
}

static void initialize(u8 bind) {
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (hubsan_init())
            break;
    }
    
    u32 lfsr = 0xb2c54a2ful;
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
#endif
    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    
    sessionid = lfsr;
    channel = allowed_ch[sessionid % sizeof(allowed_ch)]; // not optimal, but not worse than stock tx after all ...
    id_data = ID_NORMAL;
    if(bind || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_H107) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
        state = BIND_1;
    }
    else {
        state = DATA_1;
        A7105_WriteID(sessionid);
        A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
        PROTOCOL_SetBindState(0);
    }
    vtx_freq = 0;
    packet_count=0;
    bind_count = 0;
    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_DEVO);
    if( Model.proto_opts[PROTOOPTS_VTX_FREQ] == 0)
        Model.proto_opts[PROTOOPTS_VTX_FREQ] = 5885;
    CLOCK_StartTimer(10000, hubsan_cb);
}

const void *HUBSAN_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT: initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(A7105_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_H107 ? (void*)1L : 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)11L; // A, E, T, R, Leds, Flips(or alt-hold), Snapshot, Video Recording, Headless, RTH, GPS Hold
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)11L;
        case PROTOCMD_CURRENT_ID: return 0;
        case PROTOCMD_GETOPTIONS:
            if( Model.proto_opts[PROTOOPTS_VTX_FREQ] == 0)
                Model.proto_opts[PROTOOPTS_VTX_FREQ] = 5885;
            return hubsan4_opts;
        case PROTOCMD_TELEMETRYSTATE: 
            return (void *)(long)(Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
        default: break;
    }
    return 0;
}
#endif
