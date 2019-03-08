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
    #define E016H_Cmds PROTO_Cmds
    #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define E016H_BIND_COUNT 4
#define dbgprintf printf
#else
#define E016H_BIND_COUNT 750  // 3 seconds
// printf inside an interrupt handler is really dangerous
// this shouldn't be enabled even in debug builds without explicitly
// turning it on
#define dbgprintf if (0) printf
#endif

#define E016H_PACKET_PERIOD 4080
#define E016H_PACKET_SIZE     10
#define E016H_BIND_CHANNEL    80
#define E016H_NUM_CHANNELS    4

static u8 packet[E016H_PACKET_SIZE];
static u8 phase;
static u8 tx_power;
static u8 hopping_frequency[E016H_NUM_CHANNELS];
static u8 rx_tx_addr[5];
static u8 hopping_frequency_no;
static u16 bind_count;

enum {
    BIND,
    DATA
};

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,   // flip
    CHANNEL6,   // headless
    CHANNEL7,   // rth
    CHANNEL8,   // emergency stop
};

// flags going to packet[1]
#define FLAG_STOP   0x20
#define FLAG_FLIP   0x04

// flags going to packet[3]
#define FLAG_HEADLESS   0x10
#define FLAG_RTH        0x04

// flags going to packet[7]
#define FLAG_TAKEOFF    0x80
#define FLAG_HIGHRATE   0x08

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u16 scale_channel(s32 chanval, s32 inMin, s32 inMax, u16 destMin, u16 destMax)
{
    s32 range = (s32)destMax - (s32)destMin;
    s32 chanrange = inMax - inMin;

    if (chanval < inMin)
        chanval = inMin;
    else if (chanval > inMax)
        chanval = inMax;
    return (range * (chanval - inMin)) / chanrange + destMin;
}

static void init_e016h()
{
    NRF24L01_Initialize();
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    XN297_SetTXAddr((u8*)"\x5a\x53\x46\x30\x31", 5);  // bind address
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, E016H_BIND_CHANNEL);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);      // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);       // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);  // No retransmit
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    tx_power = Model.tx_power;
    NRF24L01_SetPower(tx_power);
    NRF24L01_SetTxRxMode(TX_EN);
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    u16 val;
    u8 can_flip = 0;
    if (bind) {
        memcpy(packet, &rx_tx_addr[1], 4);
        memcpy(&packet[4], hopping_frequency, 4);
        packet[8] = 0x23;
    } else {
        // trim commands
        packet[0] = 0;
        // aileron
        val = scale_channel(Channels[CHANNEL1], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0x3ff, 0);
        can_flip |= (val < 0x100) || (val > 0x300);
        packet[1] = val >> 8;
        packet[2] = val & 0xff;
        // elevator
        val = scale_channel(Channels[CHANNEL2], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0x3ff, 0);
        can_flip |= (val < 0x100) || (val > 0x300);
        packet[3] = val >> 8;
        packet[4] = val & 0xff;
        // throttle
        val = scale_channel(Channels[CHANNEL3], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0, 0x3ff);
        packet[5] = val >> 8;
        packet[6] = val & 0xff;
        // rudder
        val = scale_channel(Channels[CHANNEL4], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0x3ff, 0);
        packet[7] = val >> 8;
        packet[8] = val & 0xff;
        // flags
        packet[1] |= GET_FLAG(CHANNEL8, FLAG_STOP)
                  |  (can_flip ? GET_FLAG(CHANNEL5, FLAG_FLIP) : 0);
        packet[3] |= GET_FLAG(CHANNEL6, FLAG_HEADLESS)
                  |  GET_FLAG(CHANNEL7, FLAG_RTH);
        packet[7] |= FLAG_HIGHRATE;
        // frequency hopping
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++ & 0x03]);
    }
    // checksum
    packet[9] = packet[0];
    for (u8 i=1; i < E016H_PACKET_SIZE-1; i++)
        packet[9] += packet[i];

    // transmit packet
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, E016H_PACKET_SIZE);

    // keep transmit power updated
    if (tx_power != Model.tx_power) {
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static u16 E016H_callback()
{
    switch (phase) {
        case BIND:
            send_packet(1);
            if (bind_count-- == 0) {
                phase = DATA;
                XN297_SetTXAddr(rx_tx_addr, 5);
                PROTOCOL_SetBindState(0);
            }
            break;
        case DATA:
            send_packet(0);
            break;
    }
    return E016H_PACKET_PERIOD;
}

static void init_txid()
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
    rx_tx_addr[0] = 0xa5;
    rx_tx_addr[1] = 0x00;
    rx_tx_addr[2] = lfsr >> 24;
    rx_tx_addr[3] = lfsr >> 16;
    rx_tx_addr[4] = lfsr >> 8;

    // rf channels
    rand32_r(&lfsr, 0);
    hopping_frequency[0] = (lfsr >> 24) % 80;
    hopping_frequency[1] = (lfsr >> 16) % 80;
    hopping_frequency[2] = (lfsr >>  8) % 80;
    hopping_frequency[3] = (lfsr >>  0) % 80;
}

static void initialize()
{
    CLOCK_StopTimer();
    init_txid();
    init_e016h();
    hopping_frequency_no = 0;
    phase = BIND;
    bind_count = E016H_BIND_COUNT;
    PROTOCOL_SetBindState(E016H_BIND_COUNT * E016H_PACKET_PERIOD / 1000);
    CLOCK_StartTimer(500, E016H_callback);
}

uintptr_t E016H_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return  8;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
