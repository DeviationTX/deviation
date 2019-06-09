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

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define V761_BIND_COUNT 4
#define dbgprintf printf
#else
#define V761_BIND_COUNT 750  // 3 seconds
// printf inside an interrupt handler is really dangerous
// this shouldn't be enabled even in debug builds without explicitly
// turning it on
#define dbgprintf if (0) printf
#endif

#define V761_PACKET_PERIOD  7060  // Timeout for callback in uSec
#define V761_INITIAL_WAIT   500
#define V761_PACKET_SIZE    8

// Fx chan management
#define V761_BIND_FREQ  0x28
#define V761_RF_NUM_CHANNELS    3

static u8 packet[V761_PACKET_SIZE];
static u8 hopping_frequency[V761_RF_NUM_CHANNELS];
static u8 rx_tx_addr[4];
static u8 hopping_frequency_no;
static u8 phase;
static u8 power;
static u8 packet_count;
static u16 bind_counter;

enum
{
    V761_BIND1 = 0,
    V761_BIND2,
    V761_DATA
};

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,   // flight mode
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

static void V761_set_checksum()
{
    u8 checksum = packet[0];
    for (u8 i = 1; i < V761_PACKET_SIZE - 2; i++)
        checksum += packet[i];
    if (phase == V761_BIND1)
    {
        packet[6] = checksum ^ 0xff;
        packet[7] = packet[6];
    }
    else
    {
        checksum += packet[6];
        packet[7] = checksum ^ 0xff;
    }
}

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

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void V761_send_packet()
{
    u8 flags;

    if (phase != V761_DATA)
    {
        packet[0] = rx_tx_addr[0];
        packet[1] = rx_tx_addr[1];
        packet[2] = rx_tx_addr[2];
        packet[3] = rx_tx_addr[3];
        packet[4] = hopping_frequency[1];
        packet[5] = hopping_frequency[2];
        if (phase == V761_BIND2)
            packet[6] = 0xf0;  // ?
    }
    else
    {
        packet[0] = scale_channel(Channels[CHANNEL3], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0, 0xff);  // throttle
        packet[1] = scale_channel(Channels[CHANNEL4], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0xff, 0) >> 1;  // rudder
        packet[2] = scale_channel(Channels[CHANNEL2], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0xff, 0) >> 1;  // elevator
        packet[3] = scale_channel(Channels[CHANNEL1], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 0xff, 0) >> 1;  // Aileron
        packet[5] = (packet_count++ / 3) << 6;
        packet[4] = (packet[5] == 0x40) ? 0x1a : 0x20;

        // Channel 5 - Gyro mode is packet 5
        if (GET_FLAG(CHANNEL5, 1))    // Expert mode, gyro off
            flags = 0x0c;
        else
            flags = 0x0a;     // Midd Mode ( Gyro on, no rate limits)
        packet[5] |= flags;
        packet[6] = 0x80;  // unknown

        // packet counter
        if (packet_count >= 12)
            packet_count = 0;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
        if (hopping_frequency_no >= V761_RF_NUM_CHANNELS)
            hopping_frequency_no = 0;
    }
    V761_set_checksum();
    // Power on, TX mode, 2byte CRC
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, V761_PACKET_SIZE);

    if (power != Model.tx_power)
    {
        power = Model.tx_power;
        NRF24L01_SetPower(power);
    }
}

static void V761_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);            // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);        // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);        // set address length (4 bytes)
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);    // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);                // 1Mbps
    NRF24L01_SetPower(power);
    NRF24L01_Activate(0x73);                            // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);            // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
}

static void V761_initialize_txid()
{
    // use address / frequencies dumped from stock transmitters
    switch (Model.fixed_id % 3) {
        case 0:
            // dump from SPI grab)
            memcpy(rx_tx_addr, (uint8_t *)"\x6f\x2c\xb1\x93", 4);
            // Actual hopping_frequency from SPI grab)
            memcpy(hopping_frequency, (uint8_t *)"\x14\x1e\x4b", 3);
            break;
        case 1:
            // Dump from air on Protonus TX
            memcpy(rx_tx_addr, (uint8_t *)"\xE8\xE4\x45\x09", 4);
            memcpy(hopping_frequency, (uint8_t *)"\x0D\x21\x44", 3);
            break;
        case 2:
            // Dump from air on mshagg2 TX
            memcpy(rx_tx_addr, (uint8_t *)"\xAE\xD1\x45\x09", 4);
            memcpy(hopping_frequency, (uint8_t *)"\x13\x1D\x4A", 3);
            break;
    }
}

static u16 V761_callback()
{
    switch (phase)
    {
    case V761_BIND1:
        if (bind_counter)
            bind_counter--;
        packet_count++;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, V761_BIND_FREQ);
        XN297_SetTXAddr((u8*)"\x34\x43\x10\x10", 4);
        V761_send_packet();
        if (packet_count >= 20)
        {
            packet_count = 0;
            phase = V761_BIND2;
        }
        return 15730;
    case V761_BIND2:
        if (bind_counter)
            bind_counter--;
        packet_count++;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[0]);
        XN297_SetTXAddr(rx_tx_addr, 4);
        V761_send_packet();
        if (bind_counter == 0)
        {
            phase = V761_DATA;
            PROTOCOL_SetBindState(0);
            return 15730;
        }
        else if (packet_count >= 20)
        {
            packet_count = 0;
            phase = V761_BIND1;
        }
        return 15730;
    case V761_DATA:
        V761_send_packet();
        break;
    }
    return V761_PACKET_PERIOD;
}

static void initialize(void)
{
    CLOCK_StopTimer();
    power = Model.tx_power;
    PROTOCOL_SetBindState(V761_BIND_COUNT * V761_PACKET_PERIOD / 1000);
    bind_counter = V761_BIND_COUNT;
    V761_initialize_txid();
    phase = V761_BIND1;
    V761_init();
    hopping_frequency_no = 0;
    packet_count = 0;
    CLOCK_StartTimer(V761_INITIAL_WAIT, V761_callback);
}

uintptr_t V761_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
    case PROTOCMD_INIT:  initialize(); return 0;
    case PROTOCMD_DEINIT:
    case PROTOCMD_RESET:
        CLOCK_StopTimer();
        return (NRF24L01_Reset() ? 1 : -1);
    case PROTOCMD_CHECK_AUTOBIND: return 1;
    case PROTOCMD_BIND:  initialize(); return 0;
    case PROTOCMD_NUMCHAN: return 5;
    case PROTOCMD_DEFAULT_NUMCHAN: return 5;
    case PROTOCMD_CURRENT_ID: return Model.fixed_id;
    case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
    case PROTOCMD_CHANNELMAP: return AETRG;
    default: break;
    }
    return 0;
}

#endif
