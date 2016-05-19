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

#define TXPACKET_SIZE 38
#define RXPACKET_SIZE 37
#define NUMFREQ       16
#define TXID_SIZE     4
#define RXID_SIZE     4

static u8 packet[TXPACKET_SIZE];
static u8 txid[TXID_SIZE];
static u8 rxid[RXID_SIZE];
static u8 hopping_frequency[NUMFREQ];
static u8 packet_type;
static u8 bind_count;
static u8 bind_reply;
static u8 state;
static u8 channel;

static const u8 AFHDS2A_regs[] = {
    -1  , 0x42 | (1<<5), 0x00, 0x25, 0x00,   -1,   -1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3c, 0x05, 0x00, 0x50, // 00 - 0f
    0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x4f, 0x62, 0x80,   -1,   -1, 0x2a, 0x32, 0xc3, 0x1f, // 10 - 1f
    0x1e,   -1, 0x00,   -1, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00, // 20 - 2f
    0x01, 0x0f // 30 - 31
};

enum{
    PACKET_STICKS,
    PACKET_SETTINGS,
};

enum{
    BIND1,
    BIND2,
    BIND3,
    BIND4,
    DATA1,
};

static const char * const afhds2a_opts[] = {
    _tr_noop("PPM"), _tr_noop("Off"), _tr_noop("On"), NULL,
    _tr_noop("Servo Hz"), "50", "400", "5", NULL,
    "RX ID", "-32768", "32768", "1", NULL, // todo: store that elsewhere
    "RX ID2","-32768", "32768", "1", NULL, // ^^^^^^^^^^^^^^^^^^^^^^^^^^
    NULL
};

enum {
    PROTOOPTS_PPM = 0,
    PROTOOPTS_SERVO_HZ,
    PROTOOPTS_RXID, // todo: store that elsewhere
    PROTOOPTS_RXID2,// ^^^^^^^^^^^^^^^^^^^^^^^^^^
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
    A7105_WriteReg(0x0f, 0xa0);
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

static void build_sticks_packet()
{
    packet[0] = 0x58;
    memcpy( &packet[1], txid, 4);
    memcpy( &packet[5], rxid, 4);
    for(u8 i=0; i<14; i++)
    {
        if (i >= Model.num_channels) {
            packet[9 + i*2] = 0xdc;
            packet[10 + i*2] = 0x05;
            continue;
        }
        s32 value = (s32)Channels[i] * 0x1f1 / CHAN_MAX_VALUE + 0x5d9;
        if (value < 900 || value > 2100)
            value = 1500;
        packet[9 +  i*2] = value & 0xff;
        packet[10 + i*2] = (value >> 8) & 0xff;
    }
    packet[37] = 0x00;
}

static void build_settings_packet()
{
    packet[0] = 0xaa;
    memcpy( &packet[1], txid, 4);
    memcpy( &packet[5], rxid, 4);
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

static void build_packet(u8 type)
{
    switch(type) {
        case PACKET_STICKS:
            build_sticks_packet();
            break;
        case PACKET_SETTINGS:
            build_settings_packet();
            break;
    }
}

// telemetry sensors ID
enum{
    SENSOR_RX_VOLTAGE   = 0x00,
    SENSOR_RX_ERR_RATE  = 0xfe,
    SENSOR_RX_RSSI      = 0xfc,
};

static void update_telemetry()
{
    // AA | TXID | RXID | sensor id | sensor # | value 16 bit big endian | sensor id ......
    // max 7 sensors per packet
    
    for(u8 sensor=0; sensor<7; sensor++) {
        u8 index = 9+(4*sensor);
        switch(packet[index]) {
            case SENSOR_RX_VOLTAGE:
                Telemetry.value[TELEM_FRSKY_VOLT1] = packet[index+3]<<8 | packet[index+2];
                TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
                break;
            case SENSOR_RX_ERR_RATE:
                // packet[index+2];
                break;
            case SENSOR_RX_RSSI:
                Telemetry.value[TELEM_FRSKY_RSSI] = -packet[index+2];
                TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
                break;
            case 0xff:
                return;
            default:
                // unknown sensor ID
                break;
            }
    }
}

static void build_bind_packet()
{
    u8 ch;
    memcpy( &packet[1], txid, 4);
    memset( &packet[5], 0xff, 4);
    packet[10]= 0x00;
    for(ch=0; ch<16; ch++) {
        packet[11+ch] = hopping_frequency[ch];
    }
    memset( &packet[27], 0xff, 10);
    packet[37] = 0x00;
    switch(state) {
        case BIND1:
            packet[0] = 0xbb;
            packet[9] = 0x01;
            break;
        case BIND2:
        case BIND3:
        case BIND4:
            packet[0] = 0xbc;
            if(state == BIND4) {
                memcpy( &packet[5], &rxid, 4);
                memset( &packet[11], 0xff, 16);
            }
            packet[9] = state-1;
            if(packet[9] > 0x02)
                packet[9] = 0x02;
            packet[27]= 0x01;
            packet[28]= 0x80;
            break;
    }
}

static void calc_fh_channels(uint32_t seed)
{
    int idx = 0;
    uint32_t rnd = seed;
    while (idx < NUMFREQ) {
        int i;
        int count_1_42 = 0, count_43_85 = 0, count_86_128 = 0, count_129_168=0;
        rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

        uint8_t next_ch = ((rnd >> (idx%32)) % 0xa8) + 1;
        // Keep the distance 2 between the channels - either odd or even
        if (((next_ch ^ seed) & 0x01 )== 0)
            continue;
        // Check that it's not duplicate and spread uniformly
        for (i = 0; i < idx; i++) {
            if(hopping_frequency[i] == next_ch)
                break;
            if(hopping_frequency[i] <= 42)
                count_1_42++;
            else if (hopping_frequency[i] <= 85)
                count_43_85++;
            else if (hopping_frequency[i] <= 128)
                count_86_128++;
			else
				count_129_168++;
        }
        if (i != idx)
            continue;
        if ((next_ch <= 42 && count_1_42 < 5)
          ||(next_ch >= 43 && next_ch <= 85 && count_43_85 < 5)
		  ||(next_ch >= 86 && next_ch <=128 && count_86_128 < 5)
          ||(next_ch >= 129 && count_129_168 < 5))
        {
            hopping_frequency[idx++] = next_ch;
        }
    }
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id()
{
    u32 lfsr = 0x7649eca9ul;
    
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    printf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        printf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    printf("\r\n");
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    
    // Pump zero bytes for LFSR to diverge more
    for (int i = 0; i < TXID_SIZE; ++i) rand32_r(&lfsr, 0);

    for (u8 i = 0; i < TXID_SIZE; ++i) {
        txid[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }
    
    // Use LFSR to seed frequency hopping sequence after another
    // divergence round
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    calc_fh_channels(lfsr);
}

#define WAIT_WRITE 0x80

static u16 afhds2a_cb()
{
    switch(state) {
        case BIND1:
        case BIND2:
        case BIND3:    
            A7105_Strobe(A7105_STANDBY);
            build_bind_packet();
            A7105_WriteData(packet, 38, bind_count%2 ? 0x0d : 0x8c);
            if(A7105_ReadReg(0) == 0x1b) { // todo: replace with check crc+fec
                A7105_Strobe(A7105_RST_RDPTR);
                A7105_ReadData(packet, RXPACKET_SIZE);
                if(packet[0] == 0xbc) {
                    for(u8 i=0; i<4; i++) {
                        rxid[i] = packet[5+i];
                    }
                    Model.proto_opts[PROTOOPTS_RXID] = (u16)rxid[0]<<8 | rxid[1];
                    Model.proto_opts[PROTOOPTS_RXID2]= (u16)rxid[2]<<8 | rxid[3];
                    if(packet[9] == 0x01)
                        state = BIND4;
                }
            } 
            bind_count++;
            state |= WAIT_WRITE;
            return 1700;
        
        case BIND1|WAIT_WRITE:
        case BIND2|WAIT_WRITE:
        case BIND3|WAIT_WRITE:
            A7105_Strobe(A7105_RX);
            state &= ~WAIT_WRITE;
            state++;
            if(state > BIND3)
                state = BIND1;
            return 2150;
        
        case BIND4:
            A7105_Strobe(A7105_STANDBY);
            build_bind_packet();
            A7105_WriteData(packet, 38, bind_count%2 ? 0x0d : 0x8c);
            bind_count++;
            bind_reply++;
            if(bind_reply>=4) { 
                bind_count=0;
                channel=1;
                state = DATA1;
                PROTOCOL_SetBindState(0);
            }                        
            state |= WAIT_WRITE;
            return 1700;
            
        case BIND4|WAIT_WRITE:
            A7105_Strobe(A7105_RX);
            state &= ~WAIT_WRITE;
            return 2150;
        
        case DATA1:    
            A7105_Strobe(A7105_STANDBY);
            build_packet(packet_type);
            A7105_WriteData(packet, 38, hopping_frequency[channel++]);
            if(channel >= 16)
                channel = 0;
            packet_type = PACKET_STICKS; // todo : check for settings changes
             // got some data from RX ?
             // we've no way to know if RX fifo has been filled
             // as we can't poll GIO1 or GIO2 to check WTR
             // we can't check A7105_MASK_TREN either as we know
             // it's currently in transmit mode.
             if(!(A7105_ReadReg(0) & (1<<5 | 1<<6))) { // FECF+CRCF Ok
                 A7105_Strobe(A7105_RST_RDPTR);
                 u8 check;
                 A7105_ReadData(&check,1);
                 if(check == 0xaa) {
                     A7105_Strobe(A7105_RST_RDPTR);
                     A7105_ReadData(packet, RXPACKET_SIZE);
                     if(packet[9] == 0xfc) { // rx is asking for settings ?
                         packet_type=PACKET_SETTINGS;
                     }
                     else {
                         update_telemetry();
                     }
                 }
             }
            state |= WAIT_WRITE;
            return 1700;
            
        case DATA1|WAIT_WRITE:
            state &= ~WAIT_WRITE;
            A7105_Strobe(A7105_RX);
            return 2150;
    }
    return 3850; // never reached, please the compiler
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (afhds2a_init())
            break;
    }
    initialize_tx_id();
    packet_type = PACKET_STICKS;
    bind_count = 0;
    bind_reply = 0;
    if(bind) {
        state = BIND1;
        PROTOCOL_SetBindState(0xffffffff);
    }
    else {
        state = DATA1;
        rxid[0] = (Model.proto_opts[PROTOOPTS_RXID] >> 8) & 0xff;
        rxid[1] = (Model.proto_opts[PROTOOPTS_RXID]) & 0xff;
        rxid[2] = (Model.proto_opts[PROTOOPTS_RXID2] >> 8)  & 0xff;
        rxid[3] = (Model.proto_opts[PROTOOPTS_RXID2]) & 0xff;
    }
    channel = 0;
    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_FRSKY);
    CLOCK_StartTimer(50000, afhds2a_cb);
}

const void *AFHDS2A_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(A7105_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L; // todo: test 14 channels
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
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
