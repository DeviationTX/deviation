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
  #define CX10_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 4.360   // 6 seconds
#define dbgprintf printf
#else
#define BIND_COUNT 4360   // 6 seconds
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define CX10_PACKET_SIZE  15
#define CX10A_PACKET_SIZE 19       // CX10 blue board packets have 19-byte payload
#define Q282_PACKET_SIZE  21
#define CX10_PACKET_PERIOD   1316  // Timeout for callback in uSec
#define CX10A_PACKET_PERIOD  6000

#define INITIAL_WAIT     500

// flags 
#define FLAG_FLIP       0x1000 // goes to rudder channel
#define FLAG_MODE_MASK  0x0003
#define FLAG_HEADLESS   0x0004
// flags2
#define FLAG_VIDEO      0x0002
#define FLAG_SNAPSHOT   0x0004

static const char * const cx10_opts[] = {
    _tr_noop("Format"), _tr_noop("Green"), _tr_noop("Blue-A"), "DM007", "Q282", "JC3015-1", "JC3015-2", "MK33041", "Q242", NULL, 
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_CX10_GREEN = 0,
    FORMAT_CX10_BLUE,
    FORMAT_DM007,
    FORMAT_Q282,
    FORMAT_JC3015_1,
    FORMAT_JC3015_2,
    FORMAT_MK33041,
    FORMAT_Q242,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// For code readability
enum {
    CHANNEL1 = 0,   // Aileron
    CHANNEL2,       // Elevator
    CHANNEL3,       // Throttle
    CHANNEL4,       // Rudder
    CHANNEL5,       // Rate/Mode (+ Headless on CX-10A)
    CHANNEL6,       // Flip
    CHANNEL7,       // Still Camera (DM007, Q282)
    CHANNEL8,       // Video Camera (DM007, Q282)
    CHANNEL9,       // Headless (DM007, Q282)
    CHANNEL10,      // RTH (Q282, MK33041)
    CHANNEL11,      // X Calibration (Q282)
    CHANNEL12,      // Y Calibration (Q282)
};

static u8 packet[Q282_PACKET_SIZE]; // Set to largest packet size
static u8 packet_size;
static u16 packet_period;
static u8 phase;
static u8 bind_phase;
static u16 bind_counter;
static u8 tx_power;
static const u8 rx_tx_addr[] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc};

// frequency channel management
#define RF_BIND_CHANNEL 0x02
#define NUM_RF_CHANNELS    4
static u8 current_chan = 0;
static u8 txid[4];
static u8 rf_chans[4];

enum {
    CX10_INIT1 = 0,
    CX10_BIND1,
    CX10_BIND2,
    CX10_DATA
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

// Channel values are servo time in ms, 1500ms is the middle,
// 1000 and 2000 are min and max values
static u16 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u16) ((ch * 500 / CHAN_MAX_VALUE) + 1500);
}

static u8 set_video(u8 channel, u8 video_state) {
    if (Channels[channel] > 0) {
        if (!(video_state & 0x20)) return video_state ^ 0x21;
    } else {
        if (video_state & 0x20) return video_state & 0x01;
    }
    return video_state;
}

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_PICTURE     CHANNEL7
#define CHANNEL_VIDEO       CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10
#define CHANNEL_XCAL        CHANNEL11
#define CHANNEL_YCAL        CHANNEL12
#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    u16 aileron  = convert_channel(CHANNEL1);
    u16 elevator = 3000 - convert_channel(CHANNEL2);
    u16 throttle = convert_channel(CHANNEL3);
    u16 rudder   = 3000 - convert_channel(CHANNEL4);
    
    switch(Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_DM007:
            aileron = 3000 - aileron;
            break;
        case FORMAT_Q282:
        case FORMAT_Q242:
            aileron = 3000 - aileron;
            rudder = 3000 - rudder;
            break;
        case FORMAT_JC3015_1:
        case FORMAT_JC3015_2:
            aileron = 3000 - aileron;
            elevator = 3000 - elevator;
            break;
        case FORMAT_MK33041:
            elevator = 3000 - elevator;
            break;
    }
    
    u8 offset=0;
    if( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_CX10_BLUE)
        offset = 4;
    packet[0] = bind ? 0xAA : 0x55;
    packet[1] = txid[0];
    packet[2] = txid[1];
    packet[3] = txid[2];
    packet[4] = txid[3];
    // for CX-10A [5]-[8] is aircraft id received during bind 
    packet[5+offset] = aileron & 0xff;
    packet[6+offset] = (aileron >> 8) & 0xff;
    packet[7+offset] = elevator & 0xff;
    packet[8+offset] = (elevator >> 8) & 0xff;
    packet[9+offset] = throttle & 0xff;
    packet[10+offset] = (throttle >> 8) & 0xff;
    packet[11+offset] = rudder & 0xff;
    packet[12+offset] = ((rudder >> 8) & 0xff) | GET_FLAG(CHANNEL_FLIP, 0x10);  // 0x10 here is a flip flag 
    
    // rate mode is 2 lsb of byte 13
    packet[13+offset] = 0;
    if (Channels[CHANNEL5] > 0) {
        if (Channels[CHANNEL5] < CHAN_MAX_VALUE / 2)
            packet[13+offset] |= 1;
        else
            packet[13+offset] |= 2; // headless on CX-10A
    }
    
    switch(Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_CX10_BLUE:
            if(Channels[CHANNEL_PICTURE] <= 0)
                packet[13+offset] |= 0x10;
            packet[13+offset] |= GET_FLAG(CHANNEL_VIDEO, 0x08);
            break;

        case FORMAT_Q282:
        case FORMAT_Q242:
            packet[14] = GET_FLAG(CHANNEL_FLIP, 0x80)
                       | GET_FLAG(CHANNEL_LED, 0x40)
                       | GET_FLAG(CHANNEL_HEADLESS, 0x08)
                       | GET_FLAG(CHANNEL_XCAL, 0x04)
                       | GET_FLAG(CHANNEL_YCAL, 0x02);

            if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q282) {
                packet[13] = 0x03 | GET_FLAG(CHANNEL_RTH, 0x80);
                packet[14] |= GET_FLAG(CHANNEL_PICTURE, 0x10)
                            | set_video(CHANNEL_VIDEO, packet[14] & 0x21);
                memcpy(&packet[15], "\x10\x10\xaa\xaa\x00\x00", 6);
            } else {
                packet[13] = 0x02 | GET_FLAG(CHANNEL_RTH, 0x80);
                packet[14] |= GET_FLAG(CHANNEL_PICTURE, 0x01)
                            | GET_FLAG(CHANNEL_VIDEO, 0x10);
                memcpy(&packet[15], "\x10\x10\x00\x00\x00\x00", 6);
            }
            break;

        case FORMAT_DM007:
            packet[13] |= GET_FLAG(CHANNEL_HEADLESS, FLAG_HEADLESS);
            packet[14] = GET_FLAG(CHANNEL_PICTURE, FLAG_SNAPSHOT)
                       | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO);
            break;
            
        case FORMAT_JC3015_1:
            packet[14] = GET_FLAG(CHANNEL_PICTURE, BV(3))
                       | GET_FLAG(CHANNEL_VIDEO, BV(4));
            break;
            
        case FORMAT_JC3015_2:
            packet[14] = GET_FLAG(CHANNEL_PICTURE, BV(3)); // this channel controls lights
            if( Channels[CHANNEL_FLIP] > CHAN_MAX_VALUE / 2) { // double flip
                packet[12] &= ~0x10;
                packet[14] |= BV(4);
            }
            break;
        
        case FORMAT_MK33041:
            packet[13] |= GET_FLAG(CHANNEL_PICTURE, BV(7))
                        | GET_FLAG(CHANNEL_RTH, BV(2));
            packet[14] = GET_FLAG(CHANNEL_VIDEO, BV(0))
                       | GET_FLAG(CHANNEL_HEADLESS, BV(5));
            break;
    }
    
#ifdef EMULATOR
    dbgprintf("chan 0x%02x, bind %d, data %02x", bind ? RF_BIND_CHANNEL : rf_chans[current_chan], bind, packet[0]);
    for(int i=1; i < packet_size; i++) dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif

    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (bind) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chans[current_chan++]);
        current_chan %= NUM_RF_CHANNELS;
    }
    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, packet_size);

//    radio.ce(HIGH);
//    delayMicroseconds(15);
    // It saves power to turn off radio after the transmission,
    // so as long as we have pins to do so, it is wise to turn
    // it back.
//    radio.ce(LOW);

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

static void cx10_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5); 
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7); 
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7); 

    XN297_SetTXAddr(rx_tx_addr, 5);
    XN297_SetRXAddr(rx_tx_addr, 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_size); // bytes of data payload for rx pipe 1 
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    NRF24L01_SetPower(Model.tx_power);
    
    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on
}

static u16 cx10_callback()
{
    switch (phase) {
    case CX10_INIT1:
        phase = bind_phase;
        break;

    case CX10_BIND1:
        if (bind_counter == 0) {
            phase = CX10_DATA;
            PROTOCOL_SetBindState(0);
        } else {
            send_packet(1);
            bind_counter -= 1;
        }
        break;
        
    case CX10_BIND2:
        if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) { // RX fifo data ready
            XN297_ReadPayload(packet, packet_size);
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            if(packet[9] == 1) {
                phase = CX10_BIND1;
            }
        } else {
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            send_packet(1);
            usleep(300);
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_FlushRx();
            NRF24L01_SetTxRxMode(RX_EN);
            XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) 
                          | BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_PRIM_RX)); 
        }
        break;

    case CX10_DATA:
        send_packet(0);
        break;
    }
    return packet_period;
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif
    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    // tx id
    txid[0] = (lfsr >> 24) & 0xFF;
    txid[1] = ((lfsr >> 16) & 0xFF) % 0x30;
    txid[2] = (lfsr >> 8) & 0xFF;
    txid[3] = lfsr & 0xFF;
    // rf channels
    switch(Model.proto_opts[PROTOOPTS_FORMAT]) {
    case FORMAT_Q282:
    case FORMAT_Q242:
        rf_chans[0] = 0x46;
        rf_chans[1] = 0x48;
        rf_chans[2] = 0x4a;
        rf_chans[3] = 0x4c;
        if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q242)
            for (u8 i=0; i < 4; i++)
                rf_chans[i] += 2;
        break;

    default:
        rf_chans[0] = 0x03 + (txid[0] & 0x0F);
        rf_chans[1] = 0x16 + (txid[0] >> 4);
        rf_chans[2] = 0x2D + (txid[1] & 0x0F);
        rf_chans[3] = 0x40 + (txid[1] >> 4);
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_size = 0;
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_Q282:
        case FORMAT_Q242:
            packet_size = Q282_PACKET_SIZE - CX10_PACKET_SIZE;  // difference in packet size
            /* FALLTHROUGH */
        case FORMAT_CX10_GREEN:
        case FORMAT_DM007:
        case FORMAT_JC3015_1:
        case FORMAT_JC3015_2:
        case FORMAT_MK33041:
            packet_size += CX10_PACKET_SIZE;
            packet_period = CX10_PACKET_PERIOD;
            bind_phase = CX10_BIND1;
            bind_counter = BIND_COUNT;
            PROTOCOL_SetBindState(BIND_COUNT * packet_period / 1000);
            break;
        
        case FORMAT_CX10_BLUE:
            packet_size = CX10A_PACKET_SIZE;
            packet_period = CX10A_PACKET_PERIOD;
            bind_phase = CX10_BIND2;
            bind_counter=0;
            for(u8 i=0; i<4; i++)
                packet[5+i] = 0xFF; // clear aircraft id
            packet[9] = 0;
            PROTOCOL_SetBindState(0xFFFFFFFF);
            break;
    }
    initialize_txid();
    cx10_init();
    phase = CX10_INIT1;
    CLOCK_StartTimer(INITIAL_WAIT, cx10_callback);
}

const void *CX10_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 12L; // A, E, T, R, flight mode, enable flip, photo, video, headless, RTH, X and Y calibration
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return cx10_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif

