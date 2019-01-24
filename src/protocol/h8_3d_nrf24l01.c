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
  #define H8_3D_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

#ifdef MODULAR
  //Some versions of gcc apply this to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 10
#define dbgprintf printf
#else
#define BIND_COUNT       1000
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD    1800 // Timeout for callback in uSec
#define H20H_PACKET_PERIOD 9340
#define H20MINI_PACKET_PERIOD 3100
#define INITIAL_WAIT       500
#define PACKET_SIZE        20
#define RF_NUM_CHANNELS    4
#define ADDRESS_LENGTH     5
#define H20H_BIND_RF       0x49

static const char * const h8_3d_opts[] = {
    _tr_noop("Format"), "H8 3D", "H20H", "H20 Mini", "H30 Mini", NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};

enum{
    FORMAT_H8_3D,
    FORMAT_H20H,
    FORMAT_H20MINI,
    FORMAT_H30MINI,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // LED Light / motors on/off (H20H)
    CHANNEL6,     // Flip
    CHANNEL7,     // Picture snapshot
    CHANNEL8,     // Video recording
    CHANNEL9,     // RTH + Headless (H8 3D), Headless (H20)
    CHANNEL10,    // 180/360 flip mode (H8 3D), RTH (H20)
    CHANNEL11,    // Camera gimbal down / neutral / up
};

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_SNAPSHOT    CHANNEL7  // (H11D)
#define CHANNEL_VIDEO       CHANNEL8  // (H11D)
#define CHANNEL_HEADLESS    CHANNEL9  // RTH + Headless on H8 3D
#define CHANNEL_RTH         CHANNEL10 // 180/360 flip mode on H8 3D
#define CHANNEL_GIMBALL     CHANNEL11 // H11D

enum {
    H8_3D_INIT1 = 0,
    H8_3D_BIND2,
    H8_3D_DATA
};

enum {
    // flags going to packet[17]
    FLAG_FLIP     = 0x01,
    FLAG_RATE_MID = 0x02,
    FLAG_RATE_HIGH= 0x04,
    FLAG_LED      = 0x08, // motors on/off (H20H)
    FLAG_HEADLESS = 0x10, // RTH + headless on H8, headless on JJRC H20
    FLAG_RTH      = 0x20, // 360� flip mode on H8 3D, RTH on JJRC H20
};

enum {
    // flags going to packet[18]
    FLAG_CAM_UP   = 0x04,
    FLAG_CAM_DOWN = 0x08,
    FLAG_CALIBRATE2=0x10, // acc calib. (H11D, H20, H20H)
    FLAG_CALIBRATE= 0x20, // acc calib. (H8 3D), headless calib (H20, H20H)
    FLAG_SNAPSHOT = 0x40,
    FLAG_VIDEO    = 0x80,
};

static u16 counter;
static u8 phase;
static u16 packet_period;
static u8 packet[PACKET_SIZE];
static u8 tx_power;
static u8 txid[4];
static u8 rf_chan; 
static u8 rf_channels[RF_NUM_CHANNELS]; 
static const u8 rx_tx_addr[ADDRESS_LENGTH] = { 0xc4, 0x57, 0x09, 0x65, 0x21};
static const u8 h20h_rx_tx_addr[ADDRESS_LENGTH] = {0xee, 0xdd, 0xcc, 0xbb, 0x11};

// captured from H20H stock transmitters
static const struct {
    u8 txid[4];
    u8 rfchan[2];
}
h20h_tx_rf_map[] = {{{0x83, 0x3c, 0x60, 0x00}, {0x47, 0x3e}},
                    {{0x5c, 0x2b, 0x60, 0x00}, {0x4a, 0x3c}},
                    {{0x57, 0x07, 0x00, 0x00}, {0x41, 0x48}}};
                    
// captured from H20 Mini / H30 Mini stock transmitters
static const struct {
    u8 txid[4];
    u8 rfchan[4];
}
h20mini_tx_rf_map[] = {{{0xb4, 0xbb, 0x09, 0x00}, {0x3e, 0x45, 0x47, 0x4a}},
                       {{0x94, 0x9d, 0x0b, 0x00}, {0x3e, 0x43, 0x49, 0x4a}},
                       {{0xd1, 0xd0, 0x00, 0x00}, {0x3f, 0x42, 0x46, 0x4a}},
                       {{0xcb, 0xcd, 0x04, 0x00}, {0x41, 0x43, 0x46, 0x4a}}};

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 sum = packet[9];
    for (int i=10; i < PACKET_SIZE-1; i++) sum += packet[i];
    return sum;
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static s16 scale_channel(u8 ch)
{
    s32 chanval = Channels[ch];
    s16 destMin=0, destMax=0;
    switch(ch) {
        case CHANNEL1:
        case CHANNEL2:
            if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_H30MINI) {
                destMin = 0xbb;
                destMax = 0x43;
            }
            else {
                destMin = 0x43;
                destMax = 0xbb;
            }
            break;
        case CHANNEL3:
            destMin = 0;
            destMax = 0xff;
            break;
        case CHANNEL4:
            if(chanval > CHAN_MAX_VALUE / 60) {
                destMin = 0x44;
                destMax = 0xbc;
            }
            else if(chanval < CHAN_MIN_VALUE / 60) {
                destMin = 0x3c;
                destMax = -0x3c;
            }
            else {
                return 0;
            }
            break;
    }
    
    s32 range = destMax - destMin;

    if      (chanval < CHAN_MIN_VALUE) chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE) chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define H20H_STEP CHAN_MAX_VALUE / 60
static s16 h20h_scale_channel(u8 ch)
{
    s32 chanval = Channels[ch];
    const s16 destMin=0x43, destMax=0xbb;   
    if(ch == CHANNEL4 && chanval > -H20H_STEP && chanval < H20H_STEP)
        return 0x7f;
    s32 range = destMax - destMin;
    if      (chanval < CHAN_MIN_VALUE) chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE) chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_H8_3D:
            case FORMAT_H20MINI:
            case FORMAT_H30MINI:
                packet[0] = 0x13;
                break;
            case FORMAT_H20H:
                packet[0] = 0x14;
                break;
    }
    packet[1] = txid[0];
    packet[2] = txid[1];
    packet[3] = txid[2];
    packet[4] = txid[3];
    packet[8] = (txid[0]+txid[1]+txid[2]+txid[3]) & 0xff; // txid checksum
    memset(&packet[9], 0, 10);
    if (bind) {    
        packet[5] = 0x00;
        packet[6] = 0x00;
        packet[7] = 0x01;
    } else {
        packet[5] = rf_chan;
        packet[7] = 0x03;
        switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_H8_3D:
            case FORMAT_H20MINI:
            case FORMAT_H30MINI:
                packet[6] = 0x08;
                packet[9] = scale_channel( CHANNEL3); // throttle
                packet[10] = scale_channel( CHANNEL4); // rudder
                packet[11]= scale_channel( CHANNEL2); // elevator
                packet[12]= scale_channel( CHANNEL1); // aileron
                packet[15] = 0x20; // trims
                packet[16] = 0x20; // trims
                break;
            case FORMAT_H20H:
                packet[6] = rf_chan == 0 ? 8 - counter : 16 - counter;
                packet[9] = h20h_scale_channel(CHANNEL3); // throttle
                packet[10] = h20h_scale_channel( CHANNEL4); // rudder
                packet[11]= h20h_scale_channel( CHANNEL2); // elevator
                packet[12]= h20h_scale_channel( CHANNEL1); // aileron
                packet[15] = 0x40; // trims
                packet[16] = 0x40; // trims
                break;
        }
        // neutral trims
        packet[13] = 0x20;
        packet[14] = 0x20;
    
        // flags
        packet[17] = FLAG_RATE_HIGH
                   | GET_FLAG( CHANNEL_LED, FLAG_LED) // emergency stop on H20H
                   | GET_FLAG( CHANNEL_FLIP, FLAG_FLIP)
                   | GET_FLAG( CHANNEL_HEADLESS, FLAG_HEADLESS)
                   | GET_FLAG( CHANNEL_RTH, FLAG_RTH); // 180/360 flip mode on H8 3D
        packet[18] = GET_FLAG( CHANNEL_SNAPSHOT, FLAG_SNAPSHOT)
                   | GET_FLAG( CHANNEL_VIDEO, FLAG_VIDEO);
        // camera gimball
        if(Channels[CHANNEL_GIMBALL] < CHAN_MIN_VALUE / 2)
            packet[18] |= FLAG_CAM_DOWN;
        else if(Channels[CHANNEL_GIMBALL] > CHAN_MAX_VALUE / 2)
            packet[18] |= FLAG_CAM_UP;
        
        // calibration
        switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_H8_3D:
            case FORMAT_H20MINI:
            case FORMAT_H30MINI:
                // both sticks bottom left: calibrate acc
                if(packet[9] <= 0x05 && packet[10] >= 0xa7 && packet[11] <= 0x57 && packet[12] >= 0xa7)
                    packet[18] |= FLAG_CALIBRATE;
                
                // both sticks bottom right: calib2
                else if(packet[9] <= 0x05 && (packet[10] >= 0x27 && packet[10] <= 0x3c) && packet[11] <= 0x57 && packet[12] <= 0x57)
                    packet[18] |= FLAG_CALIBRATE2;
                break;
            case FORMAT_H20H:
                // calibrate acc
                if(packet[9] < 0x46 && packet[10] < 0x46 && packet[11] < 0x46 && packet[12] < 0x46)
                    packet[18] |= FLAG_CALIBRATE2;
                
                // calibrate (reset) headless
                else if(packet[9] < 0x46 && packet[10] > 0xb8 && packet[11] < 0x46 && packet[12] > 0xb8)
                    packet[18] |= FLAG_CALIBRATE;
                
                break;
        }
    }
    packet[19] = checksum(); // data checksum
    
    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_H8_3D:
            case FORMAT_H20MINI:
            case FORMAT_H30MINI:
                NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? rf_channels[0] : rf_channels[rf_chan++]);
                rf_chan %= sizeof(rf_channels);
                break;
            case FORMAT_H20H:
                NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? H20H_BIND_RF : rf_channels[(u8)(counter/8)]);  
                if(!bind) {
                    counter++;
                    if(counter>15) {
                        counter = 0;
                        rf_chan = 0;
                    }
                    else if(counter > 7) {
                        rf_chan = 1;
                    }
                }
                break;
    }
    
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, PACKET_SIZE);

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void h8_3d_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_H8_3D:
        case FORMAT_H20MINI:
        case FORMAT_H30MINI:
            XN297_SetTXAddr(rx_tx_addr, ADDRESS_LENGTH);
            break;
        case FORMAT_H20H:
            XN297_SetTXAddr(h20h_rx_tx_addr, ADDRESS_LENGTH);
            break;
    }
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable RX pipe 1 (unused)
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5 byte address width
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
}

static u16 h8_3d_callback()
{
    switch (phase) {
    case H8_3D_INIT1:
        phase = H8_3D_BIND2;
        break;

    case H8_3D_BIND2:
        if (counter == 0) {
            phase = H8_3D_DATA;
            PROTOCOL_SetBindState(0);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case H8_3D_DATA:
        send_packet(0);
        break;
    }
    return packet_period;
}

static void initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    u8 i,j,ch;
    
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif

    if (Model.fixed_id) {
       for (i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_H8_3D:
            // tx id
            txid[0] = (lfsr >> 24) & 0xFF;
            txid[1] = (lfsr >> 16) & 0xFF;
            txid[2] = (lfsr >> 8) & 0xFF;
            txid[3] = lfsr & 0xFF;
            
            // rf channels
            for(ch=0; ch<4; ch++)
                rf_channels[ch] = 6 + (0x0f*ch) + (((txid[ch] >> 4) + (txid[ch] & 0x0f)) % 0x0f);
            break;
        case FORMAT_H20H:
            memcpy(txid, h20h_tx_rf_map[Model.fixed_id % (sizeof(h20h_tx_rf_map)/sizeof(h20h_tx_rf_map[0]))].txid, 4);
            memcpy(rf_channels, h20h_tx_rf_map[Model.fixed_id % (sizeof(h20h_tx_rf_map)/sizeof(h20h_tx_rf_map[0]))].rfchan, 2);
            break;
        case FORMAT_H20MINI:
        case FORMAT_H30MINI:
            memcpy(txid, h20mini_tx_rf_map[Model.fixed_id % (sizeof(h20mini_tx_rf_map)/sizeof(h20mini_tx_rf_map[0]))].txid, 4);
            memcpy(rf_channels, h20mini_tx_rf_map[Model.fixed_id % (sizeof(h20mini_tx_rf_map)/sizeof(h20mini_tx_rf_map[0]))].rfchan, 4);
            break;
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    
    counter = BIND_COUNT;
    initialize_txid();
    h8_3d_init();
    phase = H8_3D_INIT1;
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_H8_3D:
            packet_period = PACKET_PERIOD;
            break;
        case FORMAT_H20H:
            packet_period = H20H_PACKET_PERIOD;
            break;
        case FORMAT_H20MINI:
        case FORMAT_H30MINI:
            packet_period = H20MINI_PACKET_PERIOD;
            break;
    }
    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, h8_3d_callback);
}

const void *H8_3D_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 11L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)11L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return h8_3d_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
