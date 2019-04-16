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
  // Allows the linker to properly relocate
  #define JJRC345_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"  // for Transmitter

#ifdef PROTO_HAS_NRF24L01

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define JJRC345_BIND_COUNT      20
#define JJRC345_PACKET_PERIOD   100
#define dbgprintf printf
#else
#define JJRC345_BIND_COUNT    500
#define JJRC345_PACKET_PERIOD 4000  // Timeout for callback in uSec
#define dbgprintf if (0) printf
#endif

#define JJRC345_INITIAL_WAIT 500
#define JJRC345_PACKET_LEN  16
#define JJRC345_RF_BIND_CHANNEL 5
#define JJRC345_NUM_CHANNELS 4

static u8 packet[JJRC345_PACKET_LEN];
static u8 tx_power;
static u8 hopping_frequency[JJRC345_NUM_CHANNELS];
static u8 txid[2];
static u8 hopping_frequency_no;
static u8 phase;
static u16 bind_counter;

enum{
    JJRC345_BIND,
    JJRC345_DATA
};

static const char * const jjrc345_opts[] = {
    _tr_noop("Period"), "1000", "20000", "100", NULL,
    _tr_noop("Orig.ID"), "Yes", "No", NULL,
    _tr_noop("Orig.Ch"), "Yes", "No", NULL,
    NULL
};

enum {
    PROTOOPTS_PERIOD = 0,
    PROTOOPTS_ORIGINAL_ID,
    PROTOOPTS_ORIGINAL_CH,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum{
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

static void JJRC345_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((u8*)"\xcc\xcc\xcc\xcc\xcc", 5);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, JJRC345_RF_BIND_CHANNEL);    // Bind channel
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1 Mbps
    NRF24L01_SetPower(tx_power);
}

static void JJRC345_initialize_txid()
{
    if (Model.proto_opts[PROTOOPTS_ORIGINAL_ID] == 0) {
        // only fixed id for now
        txid[0] = 0x1b;
        txid[1] = 0x12;
    }
    else  // arbitrary ID
    {
        txid[0] = 0xf0;
        txid[1] = 0x0d;
    }

    if (Model.proto_opts[PROTOOPTS_ORIGINAL_CH] == 0) {
        // as dumped from original transmitter
        hopping_frequency[0] = 0x3f;
        hopping_frequency[1] = 0x49;
        hopping_frequency[2] = 0x47;
        hopping_frequency[3] = 0x47;
    }
    else  // arbitrary channels
    {
        hopping_frequency[0] = 0x10;
        hopping_frequency[1] = 0x20;
        hopping_frequency[2] = 0x30;
        hopping_frequency[3] = 0x40;
    }
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static u16 scale_channel(u8 ch, u16 destMin, u16 destMax)
{
    s32 chanval = Channels[ch];
    s32 range = (s32) destMax - (s32) destMin;

    if (chanval < CHAN_MIN_VALUE)
        chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE)
        chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

static void JJRC45_send_packet(u8 bind)
{
    packet[0] = 0x00;
    packet[2] = 0x00;

    if (bind) {
        packet[1]  = JJRC345_RF_BIND_CHANNEL;
        packet[4]  = hopping_frequency[0];
        packet[5]  = hopping_frequency[1];
        packet[6]  = hopping_frequency[2];
        packet[7]  = hopping_frequency[3];
        packet[12] = 0xa5;
    }
    else
    {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
        if (hopping_frequency_no == JJRC345_NUM_CHANNELS)
            hopping_frequency_no = 0;
        packet[1]  = hopping_frequency[hopping_frequency_no];  // next packet will be sent on this channel
        packet[4]  = scale_channel(CHANNEL3, 0, 255);      // throttle
        packet[5]  = scale_channel(CHANNEL4, 0xc1, 0x41);  // rudder
        packet[6]  = scale_channel(CHANNEL2, 0x41, 0xc1);  // elevator
        packet[7]  = scale_channel(CHANNEL1, 0xc1, 0x41);  // aileron
        packet[12] = 0x02;  // high rates
    }
    packet[3] = (packet[4] == 0xff) ? 0x0e : 0x0a;
    
    packet[8] = 0;      // trim
    packet[9] = 0;      // trim
    packet[10] = 0x40;  // trim

    packet[11] = 0x3f;  // ?

    // checksum
    packet[13] = 0xf8;  // not sure this is a constant
    for (u8 i = 0; i < 13; i++)
        packet[13] += packet[i];

    // tx id ?
    packet[14] = txid[0];
    packet[15] = txid[1];

    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, JJRC345_PACKET_LEN);

    if (tx_power != Model.tx_power) {
        // Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static u16 JJRC345_callback()
{
    switch (phase) {
        case JJRC345_BIND:
            JJRC45_send_packet(1);
            bind_counter--;
            if (bind_counter == 0)
                phase = JJRC345_DATA;
            break;
        case JJRC345_DATA:
            JJRC45_send_packet(0);
            break;
    }
    return Model.proto_opts[PROTOOPTS_PERIOD];
}

static void JJRC345_initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    PROTOCOL_SetBindState((JJRC345_BIND_COUNT * JJRC345_PACKET_PERIOD)/1000);
    JJRC345_initialize_txid();
    JJRC345_init();
    hopping_frequency_no = 0;
    bind_counter = JJRC345_BIND_COUNT;
    phase = JJRC345_BIND;
    CLOCK_StartTimer(JJRC345_INITIAL_WAIT, JJRC345_callback);
}

uintptr_t JJRC345_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT:  JJRC345_initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  JJRC345_initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 6;
        case PROTOCMD_DEFAULT_NUMCHAN: return 6;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS:
            if (Model.proto_opts[PROTOOPTS_PERIOD] == 0)
                Model.proto_opts[PROTOOPTS_PERIOD] = 4000;
            return (uintptr_t)jjrc345_opts;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
