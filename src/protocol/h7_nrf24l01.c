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
  #define H7_Cmds PROTO_Cmds
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
#define BIND_COUNT 928
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD    2625 // Timeout for callback in uSec
#define INITIAL_WAIT     500
#define PACKET_SIZE 9

static const u8 data_freq[] = {
    0x02, 0x48, 0x0C, 0x3e, 0x16, 0x34, 0x20, 0x2A,
    0x2A, 0x20, 0x34, 0x16, 0x3e, 0x0c, 0x48, 0x02
};

static const u8 mys_byte[] = {
    0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, 0x14, 
    0x05, 0x15, 0x06, 0x16, 0x07, 0x17, 0x00, 0x10
};

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Rate
    CHANNEL6,     // Flip flag
    CHANNEL7,     // Elevator trim
    CHANNEL8,     // Aileron trim
};

enum{
    // flags going to packet[6]
    
    // 0x40 always set on mt9916 ?
    FLAG_HIGH_RATE= 0x01,
    FLAG_VIDEO    = 0x02, // ?
    FLAG_SNAPSHOT = 0x10, // ?
    // = 0x20, // ?
    FLAG_FLIP     = 0x80 
};

enum {
    H7_INIT = 0,
    H7_BIND,
    H7_DATA
};

static u8 packet[PACKET_SIZE];
static u16 bind_counter;
static u8 tx_power;
static u8 txid[2];
static u8 checksum_offset; 
static u8 channel_offset;
static u8 rf_chan; 
static u8 rx_tx_addr[5];
static u8 state;

// Bit vector from bit position
#define BV(bit) (1 << bit)

static s16 scale_channel(u8 ch, s32 destMin, s32 destMax)
{
    s32 a = (destMax - destMin) * ((s32)Channels[ch] - CHAN_MIN_VALUE);
    s32 b = CHAN_MAX_VALUE - CHAN_MIN_VALUE;
    return ((a / b) - (destMax - destMin)) + destMax;
}

static u8 calcChecksum() {
    u8 result=checksum_offset;
    for(uint8_t i=0; i<8; i++)
        result += packet[i];
    return result & 0xFF;
}

static void h7_send_packet()
{
    packet[0] = scale_channel(CHANNEL3, 0xe1, 0x00); // throttle
    packet[1] = scale_channel(CHANNEL4, 0x00, 0xe1); // rudder
    packet[2] = scale_channel(CHANNEL1, 0xe1, 0x00); // aileron
    packet[3] = scale_channel(CHANNEL2, 0x00, 0xe1); // elevator
    packet[4] = scale_channel(CHANNEL7, 0x3f, 0x00); // elevator trim
    packet[5] = scale_channel(CHANNEL8, 0x00, 0x3f); // aileron trim
    packet[6] = 0x00;
    if( Channels[CHANNEL5] > 0 ) // high rate
        packet[6] |= FLAG_HIGH_RATE;
    if( Channels[CHANNEL6] > 0 ) // flip flag
        packet[6] |= FLAG_FLIP;
    packet[7] = mys_byte[rf_chan]; 
    packet[8] = calcChecksum();
    
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, data_freq[rf_chan] + channel_offset);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, PACKET_SIZE);
    
    rf_chan++;
    if(rf_chan > 15)
        rf_chan = 0;
        
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

static void h7_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    // set tx address for bind packets
    for(u8 i=0; i<5; i++)
        rx_tx_addr[i] = 0xCC;
    XN297_SetTXAddr(rx_tx_addr, 5);
    NRF24L01_FlushTx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);  // 5 bytes address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);// no auto retransmit
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    // this sequence necessary for module from stock tx
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
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
    // Power on, TX mode, 2byte CRC
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
}

static void initialize_txid()
{
    u32 temp;
    if (Model.fixed_id) {
        temp = Crc(&Model.fixed_id, sizeof(Model.fixed_id));
    } else {
        temp = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) ;
    }
    txid[0] = (temp >> 8) & 0xff;
    txid[1] = (temp >> 0) & 0xff;
    checksum_offset = (txid[0] + txid[1]) & 0xff;
    channel_offset = (((checksum_offset & 0xf0)>>4) + (checksum_offset & 0x0f)) % 8;
}

MODULE_CALLTYPE
static u16 h7_callback()
{
    switch (state) {
    case H7_INIT:
        MUSIC_Play(MUSIC_TELEMALARM1);
        state = H7_BIND;
        break;

    case H7_BIND:
        if (bind_counter == 0) {
            rx_tx_addr[0] = txid[0];
            rx_tx_addr[1] = txid[1];
            rx_tx_addr[2] = 0x00;
            // set tx address for data packets
            XN297_SetTXAddr(rx_tx_addr, 5);
            state = H7_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, data_freq[rf_chan]);
            NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
            NRF24L01_FlushTx();
            XN297_WritePayload(packet, PACKET_SIZE); // bind packet
            rf_chan++;
            if(rf_chan > 15)
                rf_chan = 0;
            bind_counter -= 1;
        }
        break;

    case H7_DATA:
        h7_send_packet();
        break;
    }
    return PACKET_PERIOD;
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    bind_counter = BIND_COUNT;
    initialize_txid();
    h7_init();
    state = H7_INIT;
    // bind packet
    packet[0] = 0x20;
    packet[1] = 0x14;
    packet[2] = 0x03;
    packet[3] = 0x25;
    packet[4] = txid[0]; // 1th byte for data state tx address  
    packet[5] = txid[1]; // 2th byte for data state tx address 
    packet[6] = 0x00; // 3th byte for data state tx address (always 0x00 ?)
    packet[7] = checksum_offset; // checksum offset
    packet[8] = 0xAA; // fixed
    
    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, h7_callback);
}

const void *H7_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 8L; // A, E, T, R, flip, rate, E trim, A trim
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif