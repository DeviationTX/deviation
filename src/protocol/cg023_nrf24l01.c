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
  #define CG023_Cmds PROTO_Cmds
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
#define BIND_COUNT 20
#define dbgprintf printf
#else
#define BIND_COUNT 800  // 6 seconds
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define CG023_PACKET_PERIOD    8200 // Timeout for callback in uSec
#define YD829_PACKET_PERIOD    2600 // stock tx is 4100, but seems to work better with shorter period
#define INITIAL_WAIT     500
#define PACKET_SIZE 15   // packets have 15-byte payload
#define RF_BIND_CHANNEL 0x2D

static const char * const cg023_opts[] = { 
    "Format", "CG023", "YD-829", NULL,  
    "Dyn Trims", _tr_noop("Off"), _tr_noop("On"), NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    PROTOOPTS_DYNTRIM,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define FORMAT_CG023 0
#define FORMAT_YD829 1

#define DYNTRIM_OFF 0
#define DYNTRIM_ON 1

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Leds
    CHANNEL6,     // Flip
    CHANNEL7,     // Still camera
    CHANNEL8,     // Video camera
    CHANNEL9,     // Headless
    CHANNEL10     // Rate (3 pos)
};

enum{
    // flags going to packet[13] (CG023)
    FLAG_FLIP     = 0x01, 
    FLAG_EASY     = 0x02, 
    FLAG_VIDEO    = 0x04, 
    FLAG_STILL    = 0x08, 
    FLAG_LED_OFF  = 0x10,
    MASK_RATE     = 0x60,
    FLAG_RATE_MID = 0x20,
    FLAG_RATE_HIGH= 0x40,
};

enum{
    // flags going to packet[13] (YD-829)
    YD_FLAG_FLIP     = 0x01,
    YD_MASK_RATE     = 0x0C,
    YD_FLAG_RATE_MID = 0x04,
    YD_FLAG_RATE_HIGH= 0x08,
    YD_FLAG_HEADLESS = 0x20,
    YD_FLAG_STILL    = 0x40, // untested
    YD_FLAG_VIDEO    = 0x80, // untested
};

enum {
    CG023_INIT1 = 0,
    CG023_BIND2,
    CG023_DATA
};

static u8 packet[PACKET_SIZE];
static u16 counter;
static u8 tx_power;
static u8 txid[2];
static u8 rf_chan; 
static const u8 rx_tx_addr[] = {0x26, 0xA8, 0x67, 0x35, 0xCC};
static u8 phase;
static u32 packet_period;

// Bit vector from bit position
#define BV(bit) (1 << bit)

static s16 scale_channel(u8 ch, s32 destMin, s32 destMax)
{
    s32 a = (destMax - destMin) * ((s32)Channels[ch] - CHAN_MIN_VALUE);
    s32 b = CHAN_MAX_VALUE - CHAN_MIN_VALUE;
    return ((a / b) - (destMax - destMin)) + destMax;
}

static void send_packet(u8 bind)
{
    if (bind) {
      packet[0]= 0xaa;
    } else {
      packet[0]= 0x55;
    }
    // transmitter id
    packet[1] = txid[0]; 
    packet[2] = txid[1];
    // unknown
    packet[3] = 0x00;
    packet[4] = 0x00;
    // throttle : 0x00 - 0xFF
    packet[5] = scale_channel(CHANNEL3, 0x00, 0xFF); 
    if( Channels[CHANNEL4] > 0) {
        // yaw right : 0x80 (neutral) - 0xBC (right)
        packet[6] = scale_channel(CHANNEL4, 0x44 , 0xBC);
    } else {  
        // yaw left : 0x00 (neutral) - 0x3C (left)
        packet[6] = scale_channel(CHANNEL4, 0x3C, -0x3C);
    }
    // elevator : 0xBB - 0x7F - 0x43
    packet[7] = scale_channel(CHANNEL2, 0xBB, 0x43); 
    // aileron : 0x43 - 0x7F - 0xBB
    packet[8] = scale_channel(CHANNEL1, 0x43, 0xBB); 
    // throttle trim : 0x30 - 0x20 - 0x10
    packet[9] = 0x20; // neutral
    if(Model.proto_opts[PROTOOPTS_DYNTRIM] == DYNTRIM_ON) { // experimental dynamic protocol trims, has no effect on 3D X4   
        // rudder trim : 0x10 - 0x20 - 0x30
        packet[10] = scale_channel(CHANNEL4, 0x10, 0x30); 
        // elevator trim : 0x60 - 0x40 - 0x20
        packet[11] = scale_channel(CHANNEL2, 0x60, 0x20);  
        // aileron trim : 0x60 - 0x40 - 0x20
        packet[12] = scale_channel(CHANNEL1, 0x60, 0x20); 
    } else { 
        // neutral trims
        packet[10] = 0x20;
        packet[11] = 0x40;
        packet[12] = 0x40;
    }
    // flags
    packet[13] = 0x00;
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_CG023:
            if(Channels[CHANNEL10] > 0) {
                if(Channels[CHANNEL10] < CHAN_MAX_VALUE / 2)
                    packet[13] |= FLAG_RATE_MID;
            } else
                packet[13] |= FLAG_RATE_HIGH;             
            if(Channels[CHANNEL5] > 0)
                packet[13] |= FLAG_LED_OFF;
            if(Channels[CHANNEL6] > 0)
                packet[13] |= FLAG_FLIP;
            if(Channels[CHANNEL7] > 0)
                packet[13] |= FLAG_STILL;
            if(Channels[CHANNEL8] > 0)
                packet[13] |= FLAG_VIDEO;
            if(Channels[CHANNEL9] > 0)
                packet[13] |= FLAG_EASY;
            break;
        case FORMAT_YD829:
            // reverse aileron direction
            packet[8] = 0xFE - packet[8];
            if(Channels[CHANNEL10] > 0) {
                if(Channels[CHANNEL10] < CHAN_MAX_VALUE / 2)
                    packet[13] |= YD_FLAG_RATE_MID;
            } else
                packet[13] |= YD_FLAG_RATE_HIGH; 
            if(Channels[CHANNEL6] > 0)
                packet[13] |= YD_FLAG_FLIP;
            if(Channels[CHANNEL7] > 0)
                packet[13] |= YD_FLAG_STILL;
            if(Channels[CHANNEL8] > 0)
                packet[13] |= YD_FLAG_VIDEO;
            if(Channels[CHANNEL9] > 0)
                packet[13] |= YD_FLAG_HEADLESS;
            break;
    }
    packet[14] = 0;
    
    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (bind) {
      NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
      NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chan);
    }
    // clear packet status bits and TX FIFO
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

static void cg023_init()
{
    NRF24L01_Initialize();

    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5); 
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7); 
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7); 

    XN297_SetTXAddr(rx_tx_addr, 5);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on

    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    dbgprintf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        dbgprintf("BK2421 detected\n");
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\x99\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xDF\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
    } else {
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back
}

MODULE_CALLTYPE
static u16 cg023_callback()
{
    switch (phase) {
    case CG023_INIT1:
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = CG023_BIND2;
        break;

    case CG023_BIND2:
        if (counter == 0) {
            phase = CG023_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case CG023_DATA:
        send_packet(0);
        break;
    }
    return packet_period;
}

static void initialize_txid()
{
    u32 temp;
    if (Model.fixed_id) {
        temp = Crc(&Model.fixed_id, sizeof(Model.fixed_id));
    } else {
        temp = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) ;
    }
    txid[0] = 0x80 | (temp % 0x40); 
    if( txid[0] == 0xAA) // avoid using same freq for bind and data channel
        txid[0] ++;
    txid[1] = (temp >> 8) & 0xFF;
    // rf channel for data packets
    rf_chan = txid[0] - 0x7D;
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    
    counter = BIND_COUNT;
    initialize_txid();
    cg023_init();
    phase = CG023_INIT1;
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_YD829:
            packet_period = YD829_PACKET_PERIOD;
            break;
        case FORMAT_CG023:
            packet_period = CG023_PACKET_PERIOD;
            break;    
    }
    PROTOCOL_SetBindState(BIND_COUNT * packet_period / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, cg023_callback);
}

const void *CG023_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 10L; // A, E, T, R, light, flip, photo, video, headless, rate
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)10L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return cg023_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif