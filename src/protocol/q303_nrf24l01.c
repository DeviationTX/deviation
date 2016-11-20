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
  #define Q303_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

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
    #define BIND_COUNT 4
    #define dbgprintf printf
#else
    #define BIND_COUNT 2335
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define PACKET_SIZE  10
#define PACKET_PERIOD   1500  // Timeout for callback in uSec
#define INITIAL_WAIT     500
#define RF_BIND_CHANNEL 0x02
#define NUM_RF_CHANNELS    4

static u8 packet[PACKET_SIZE];
static u8 phase;
static u16 bind_counter;
static u8 tx_power;
static u8 tx_addr[5];
static u8 current_chan;
static u8 txid[4];
static u8 rf_chans[4];

// haven't figured out txid<-->rf channel mapping yet
static const struct {
    u8 txid[sizeof(txid)];
    u8 rfchan[NUM_RF_CHANNELS];
} q303_tx_rf_map[] =  //{{{0xb8, 0x69, 0x64, 0x67}, {0x48, 0x4a, 0x4c, 0x46}}}; // tx2
                    {{{0xAE, 0x89, 0x97, 0x87}, {0x4A, 0x4C, 0x4E, 0x48}}}; // tx1

enum {
    BIND,
    DATA
};

static const char * const q303_opts[] = {
    _tr_noop("Format"), "Q303", NULL, 
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_Q303 = 0,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// For code readability
enum {
    CHANNEL1 = 0,   // Aileron
    CHANNEL2,       // Elevator
    CHANNEL3,       // Throttle
    CHANNEL4,       // Rudder
    CHANNEL5,       // Altitude Hold
    CHANNEL6,       // Flip
    CHANNEL7,       // Still Camera
    CHANNEL8,       // Video Camera
    CHANNEL9,       // Headless
    CHANNEL10,      // RTH
    CHANNEL11,      // Gimbal control (3 pos)
};

#define CHANNEL_AHOLD    CHANNEL5
#define CHANNEL_FLIP     CHANNEL6
#define CHANNEL_SNAPSHOT CHANNEL7
#define CHANNEL_VIDEO    CHANNEL8
#define CHANNEL_HEADLESS CHANNEL9
#define CHANNEL_RTH      CHANNEL10
#define CHANNEL_GIMBAL   CHANNEL11

// flags going to packet[8]
#define FLAG_HIGHRATE 0x03
#define FLAG_AHOLD    0x40
#define FLAG_RTH      0x80

// flags going to packet[9]
#define FLAG_GIMBAL_DN 0x04
#define FLAG_GIMBAL_UP 0x20
#define FLAG_HEADLESS  0x08
#define FLAG_FLIP      0x80
#define FLAG_SNAPSHOT  0x10
#define FLAG_VIDEO     0x01

// Bit vector from bit position
#define BV(bit) (1 << bit)

// Channel values: 0-1000
static u16 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u16) ((ch * 500 / CHAN_MAX_VALUE) + 500);
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    if(bind) {
        packet[0] = 0xaa;
        memcpy(&packet[1], txid, 4);
        memset(&packet[5], 0, 5);
    }
    else {
        u16 aileron  = convert_channel(CHANNEL1);
        u16 elevator = 1000 - convert_channel(CHANNEL2);
        u16 throttle = convert_channel(CHANNEL3);
        u16 rudder   = 1000 - convert_channel(CHANNEL4);
        
        packet[0] = 0x55;
        packet[1] = aileron >> 2     ;     // 8 bits
        packet[2] = (aileron & 0x03) << 6  // 2 bits
                  | (elevator >> 4);       // 6 bits
        packet[3] = (elevator & 0x0f) << 4 // 4 bits
                  | (throttle >> 6);       // 4 bits
        packet[4] = (throttle & 0x3f) << 2 // 6 bits 
                  | (rudder >> 8);         // 2 bits
        packet[5] = rudder & 0xff;         // 8 bits
        packet[6] = 0x10; // trim(s) ?
        packet[7] = 0x10; // trim(s) ?
        packet[8] = 0x03  // high rate (0-3)
                  | GET_FLAG(CHANNEL_AHOLD, FLAG_AHOLD)
                  | GET_FLAG(CHANNEL_RTH, FLAG_RTH);
        packet[9] = 0x40 // always set
                  | GET_FLAG(CHANNEL_HEADLESS, FLAG_HEADLESS)
                  | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP)
                  | GET_FLAG(CHANNEL_SNAPSHOT, FLAG_SNAPSHOT)
                  | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO);
        if(Channels[CHANNEL_GIMBAL] < CHAN_MIN_VALUE / 2)
            packet[9] |= FLAG_GIMBAL_DN;
        else if(Channels[CHANNEL_GIMBAL] > CHAN_MAX_VALUE / 2)
            packet[9] |= FLAG_GIMBAL_UP;
        // set low rate for first packets
        if(bind_counter != 0) {
            packet[8] &= ~0x03;
            bind_counter--;
        }
    }
    
#ifdef EMULATOR
    uint8_t pos;
    dbgprintf("\nCh: %02x    ", bind ? 0x02 : rf_chans[current_chan]);
    for(pos=0; pos<sizeof(packet); pos++)
        dbgprintf("%02x ", packet[pos]);
#endif
    
    // Power on, TX mode, CRC enabled
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (bind) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chans[current_chan++]);
        current_chan %= NUM_RF_CHANNELS;
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

static void q303_init()
{
    const u8 bind_address[] = {0xcc,0xcc,0xcc,0xcc,0xcc};
    
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetScrambledMode(XN297_UNSCRAMBLED);
    XN297_SetTXAddr(bind_address, 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_250K);
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
    NRF24L01_Activate(0x73);
    
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
static u16 q303_callback()
{
#ifdef EMULATOR
    uint8_t pos;
#endif
    switch (phase) {
        case BIND:
            if (bind_counter == 0) {
                tx_addr[0] = 0x55;
                memcpy(&tx_addr[1], txid, 4);
#ifdef EMULATOR
                dbgprintf("\ntxid: ");
                for(pos=0; pos<sizeof(txid); pos++)
                    dbgprintf("%02x ", txid[pos]);
                dbgprintf("\ntx_addr: ");
                for(pos=0; pos<sizeof(tx_addr); pos++)
                    dbgprintf("%02x ", tx_addr[pos]);
                dbgprintf("\nrf_chans: ");
                for(pos=0; pos<sizeof(rf_chans); pos++)
                    dbgprintf("%02x ", rf_chans[pos]);
#endif
                XN297_SetTXAddr(tx_addr, 5);
                phase = DATA;
                bind_counter = BIND_COUNT;
                PROTOCOL_SetBindState(0);
            } else {
                send_packet(1);
                bind_counter--;
            }
            break;
        case DATA:
            send_packet(0);
            break;
    }
    return PACKET_PERIOD;
}

// haven't figured out txid<-->rf channel mapping yet
static void initialize_txid()
{
    memcpy(txid, q303_tx_rf_map[Model.fixed_id % (sizeof(q303_tx_rf_map)/sizeof(q303_tx_rf_map[0]))].txid, sizeof(txid));
    memcpy(rf_chans, q303_tx_rf_map[Model.fixed_id % (sizeof(q303_tx_rf_map)/sizeof(q303_tx_rf_map[0]))].rfchan, sizeof(rf_chans));
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    initialize_txid();
    q303_init();
    bind_counter = BIND_COUNT;
    current_chan = 0;
    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    phase = BIND;
    CLOCK_StartTimer(INITIAL_WAIT, q303_callback);
}

const void *Q303_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 11L; // A, E, T, R, altitude hold, flip, photo, video, headless, RTH, gimbal
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)11L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return q303_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif

