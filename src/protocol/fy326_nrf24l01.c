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
  #define FY326_Cmds PROTO_Cmds
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
#define dbgprintf printf
#else
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT    500
#define PACKET_PERIOD   1500  // Timeout for callback in uSec
#define PACKET_CHKTIME  300   // Time to wait if packet not yet received or sent
#define PACKET_SIZE     15
#define BIND_COUNT      16


#if 0
static const char * const fy326_opts[] = {
    _tr_noop("Format"), _tr_noop("Green"), _tr_noop("Blue-A"), _tr_noop("DM007"), NULL, 
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_FY326_GREEN = 0,
    FORMAT_FY326_BLUE,
    FORMAT_DM007,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);
#endif


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
    CHANNEL10,    // Return To Home
    CHANNEL11,    // Calibrate
};
#define CHANNEL_FLIP      CHANNEL6
#define CHANNEL_HEADLESS  CHANNEL9
#define CHANNEL_RTH       CHANNEL10

static u8 bind_counter;
static u8 phase;
static u8 tx_power;
static u8 packet[PACKET_SIZE];

// frequency channel management
#define RF_BIND_CHANNEL    0x17
#define NUM_RF_CHANNELS    5
static u8 current_chan;
static u8 rf_chans[NUM_RF_CHANNELS];
static u8 txid[5];
static u8 rxid;

enum {
    FY326_INIT1 = 0,
    FY326_BIND1,
    FY326_BIND2,
    FY326_DATA
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static u8 scale_channel(u8 ch, u8 destMin, u8 destMax)
{
    s32 range = destMax - destMin;
    return (range * (Channels[ch] - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    packet[0] = txid[3];
    if (bind) {
        packet[1] = 0x55;
    } else {
        packet[1] = GET_FLAG(CHANNEL_HEADLESS, 0x80)
                  | GET_FLAG(CHANNEL_RTH,      0x40)
                  | GET_FLAG(CHANNEL_FLIP,     0x02)
                  | 0x04; // always in "expert mode"
    }
    packet[2] = scale_channel(CHANNEL1, 0, 200);  // aileron
    packet[3] = scale_channel(CHANNEL2, 0, 200);  // elevator
    packet[4] = scale_channel(CHANNEL4, 0, 200);  // rudder
    packet[5] = scale_channel(CHANNEL3, 0, 200);  // throttle
    packet[6] = txid[0];
    packet[7] = txid[1];
    packet[8] = txid[2];
    packet[9] = 0; //aileron_trim;
    packet[10] = 0; //elevator_trim;
    packet[11] = 0; //yaw_trim;
    packet[12] = 0; //throttle_trim;
    packet[13] = rxid;
    packet[14] = txid[4];

    if (bind) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chans[current_chan++]);
        current_chan %= NUM_RF_CHANNELS;
    }

#ifdef EMULATOR
    dbgprintf("chan 0x%02x, bind %d, data %02x", rf_chans[current_chan], bind, packet[0]);
    for(int i=1; i < PACKET_SIZE; i++) dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif

    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    NRF24L01_WritePayload(packet, PACKET_SIZE);

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

static void fy326_init()
{
    const u8 rx_tx_addr[] = {0x15, 0x59, 0x23, 0xc6, 0x29};

    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);   // Three-byte rx/tx address
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_tx_addr, sizeof(rx_tx_addr));
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, sizeof(rx_tx_addr));
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PACKET_SIZE);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    NRF24L01_SetBitrate(NRF24L01_BR_250K);
    NRF24L01_SetPower(Model.tx_power);
    
    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);

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
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\x99\x00\x39\x41", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x8A\xDB", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x0F\xB6", 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xDF\x96\x8A\xDB", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x8A\xDB", 4);
    } else {
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back
}

MODULE_CALLTYPE
static u16 fy326_callback()
{
    switch (phase) {
    case FY326_INIT1:
        MUSIC_Play(MUSIC_TELEMALARM1);
        bind_counter = BIND_COUNT;
        phase = FY326_BIND2;
        send_packet(1);
        return PACKET_CHKTIME;
        break;

    case FY326_BIND1:
#ifdef EMULATOR
        if (1) {
            packet[13] = 0x7e;
#else
        if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) { // RX fifo data ready
            NRF24L01_ReadPayload(packet, PACKET_SIZE);
#endif
            rxid = packet[13];
            txid[0] = 0xaa;
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
            phase = FY326_DATA;
        } else if (bind_counter-- == 0) {
            bind_counter = BIND_COUNT;
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            send_packet(1);
            phase = FY326_BIND2;
            return PACKET_CHKTIME;
        }
        break;

    case FY326_BIND2:
#ifdef EMULATOR
        if (1) {
#else
        if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_TX_DS)) { // TX data sent
#endif
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_FlushRx();
            NRF24L01_SetTxRxMode(RX_EN);
            phase = FY326_BIND1;
        } else {
            return PACKET_CHKTIME;
        }
        break;

    case FY326_DATA:
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;
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
#if 0
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    // tx id
    txid[0] = (lfsr >> 24) & 0xFF;
    txid[1] = ((lfsr >> 16) & 0xFF);
    txid[2] = (lfsr >> 8) & 0xFF;
    txid[3] = lfsr & 0xFF;
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    txid[4] = lfsr & 0xFF;
#else
    txid[0] = Model.fixed_id & 0xFF;
    txid[1] = (Model.fixed_id >> 8) & 0xFF;
    txid[2] = 0xc0 + ((Model.fixed_id >> 16) & 0xFF);
    txid[3] = 0x90;
    txid[4] = 0x1c;
#endif
    // rf channels
    rf_chans[0] =         txid[0] & 0x0F;
    rf_chans[1] = 0x10 + (txid[0] >> 4);
    rf_chans[2] = 0x20 + (txid[1] & 0x0F);
    rf_chans[3] = 0x30 + (txid[1] >> 4);
    rf_chans[4] = 0x40 + (txid[2] >> 4);
}

static void initialize()
{
    CLOCK_StopTimer();
    PROTOCOL_SetBindState(0xFFFFFFFF);
    tx_power = Model.tx_power;
    rxid = 0xaa;
    phase = FY326_INIT1;
    bind_counter = BIND_COUNT;
    initialize_txid();
    fy326_init();
    CLOCK_StartTimer(INITIAL_WAIT, fy326_callback);
}

const void *FY326_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 10L; // A, E, T, R, enable flip, expert mode
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)10L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return 0; //fy326_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif

