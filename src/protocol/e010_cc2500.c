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

#ifdef PROTO_HAS_CC2500

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 20
#define PACKET_PERIOD    150
#define dbgprintf printf
#else
#define BIND_COUNT 150
#define PACKET_PERIOD    4000  // Timeout for callback in uSec
#define dbgprintf if (0) printf
#endif

#define INITIAL_WAIT       500
#define PACKET_SIZE        16
#define RF_NUM_CHANNELS    4
#define ADDRESS_LENGTH     5

const char * const e010_opts[] = {
    _tr_noop("Format"), "E010", "Phoenix", NULL,
    _tr_noop("Freq-Fine"),  "-127", "127", NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    PROTOOPTS_FREQFINE,
    LAST_PROTO_OPT
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum {
    FORMAT_E010,
    FORMAT_PHOENIX,
};

// For code readability
enum {
    CHANNEL1 = 0,  // Aileron
    CHANNEL2,      // Elevator
    CHANNEL3,      // Throttle
    CHANNEL4,      // Rudder
    CHANNEL5,      // Leds / Arm
    CHANNEL6,      // Flip
    CHANNEL7,      // Still camera
    CHANNEL8,      // Video camera
    CHANNEL9,      // Headless
    CHANNEL10,     // Return To Home
};

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_ARM         CHANNEL5  // TDR Phoenix mini
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_PICTURE     CHANNEL7
#define CHANNEL_VIDEO       CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10

enum {
    E010_BIND = 0,
    E010_DATA
};

static u8 tx_power;
static s8 fine;
static u16 counter;
static u8 phase;
static u8 tx_power;
static u8 rf_chan;
static u8 txid[3];
static u8 packet[PACKET_SIZE];
static u8 rf_channels[RF_NUM_CHANNELS];

// dumped from E010 and H36 stock transmitters
static const struct {
    u8 txid[2];
    u8 rfchan[RF_NUM_CHANNELS];
}
e010_tx_rf_map[] = { {{0x4F, 0x1C}, {0x3A, 0x35, 0x4A, 0x45}},
                    {{0x90, 0x1C}, {0x2E, 0x36, 0x3E, 0x46}},
                    {{0x24, 0x36}, {0x32, 0x3E, 0x42, 0x4E}},
                    {{0x7A, 0x40}, {0x2E, 0x3C, 0x3E, 0x4C}},
                    {{0x61, 0x31}, {0x2F, 0x3B, 0x3F, 0x4B}},
                    {{0x5D, 0x37}, {0x33, 0x3B, 0x43, 0x4B}},
                    {{0xFD, 0x4F}, {0x33, 0x3B, 0x43, 0x4B}},
                    {{0x86, 0x3C}, {0x34, 0x3E, 0x44, 0x4E}} };

// xn297 emulation
////////////////////
static u8 xn297_addr_len;
static u8 xn297_tx_addr[5];

static void XN297L_init()
{
    CC2500_Reset();
    CC2500_Strobe(CC2500_SIDLE);

    // Address Config = No address check
    // Base Frequency = 2400
    // CRC Autoflush = false
    // CRC Enable = false
    // Channel Spacing = 333.251953
    // Data Format = Normal mode
    // Data Rate = 249.939
    // Deviation = 126.953125
    // Device Address = 0
    // Manchester Enable = false
    // Modulated = true
    // Modulation Format = GFSK
    // Packet Length Mode = Variable packet length mode. Packet length configured by the first byte after sync word
    // RX Filter BW = 203.125000
    // Sync Word Qualifier Mode = No preamble/sync
    // TX Power = 0
    // Whitening = false

    CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x01);   // Packet Automation Control
    CC2500_WriteReg(CC2500_0B_FSCTRL1,  0x0A);   // Frequency Synthesizer Control
    CC2500_WriteReg(CC2500_0C_FSCTRL0,  0x00);   // Frequency Synthesizer Control
    CC2500_WriteReg(CC2500_0D_FREQ2,    0x5C);   // Frequency Control Word, High Byte
    CC2500_WriteReg(CC2500_0E_FREQ1,    0x4E);   // Frequency Control Word, Middle Byte
    CC2500_WriteReg(CC2500_0F_FREQ0,    0xC3);   // Frequency Control Word, Low Byte
    CC2500_WriteReg(CC2500_10_MDMCFG4,  0x8D);   // Modem Configuration
    CC2500_WriteReg(CC2500_11_MDMCFG3,  0x3B);   // Modem Configuration
    CC2500_WriteReg(CC2500_12_MDMCFG2,  0x10);   // Modem Configuration
    CC2500_WriteReg(CC2500_13_MDMCFG1,  0x23);   // Modem Configuration
    CC2500_WriteReg(CC2500_14_MDMCFG0,  0xA4);   // Modem Configuration
    CC2500_WriteReg(CC2500_15_DEVIATN,  0x62);   // Modem Deviation Setting
    CC2500_WriteReg(CC2500_18_MCSM0,    0x18);   // Main Radio Control State Machine Configuration
    CC2500_WriteReg(CC2500_19_FOCCFG,   0x1D);   // Frequency Offset Compensation Configuration
    CC2500_WriteReg(CC2500_1A_BSCFG,    0x1C);   // Bit Synchronization Configuration
    CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xC7);   // AGC Control
    CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);   // AGC Control
    CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xB0);   // AGC Control
    CC2500_WriteReg(CC2500_21_FREND1,   0xB6);   // Front End RX Configuration
    CC2500_WriteReg(CC2500_23_FSCAL3,   0xEA);   // Frequency Synthesizer Calibration
    CC2500_WriteReg(CC2500_25_FSCAL1,   0x00);   // Frequency Synthesizer Calibration
    CC2500_WriteReg(CC2500_26_FSCAL0,   0x11);   // Frequency Synthesizer Calibration

    CC2500_SetTxRxMode(TX_EN);
}

static void XN297L_SetTXAddr(const u8* addr, u8 len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    xn297_addr_len = len;
    memcpy(xn297_tx_addr, addr, len);
}

static void XN297L_WritePayload(const u8* msg, u8 len)
{
    u8 buf[32];
    u8 last = 0;
    u8 i;
    static const u16 initial = 0xb5d2;

    // address
    for (i = 0; i < xn297_addr_len; ++i) {
        buf[last++] = xn297_tx_addr[xn297_addr_len - i - 1] ^ xn297_scramble[i];
    }

    // payload
    for (i = 0; i < len; ++i) {
        // bit-reverse bytes in packet
        u8 b_out = bit_reverse(msg[i]);
        buf[last++] = b_out ^ xn297_scramble[xn297_addr_len + i];
    }

    u8 offset = xn297_addr_len < 4 ? 1 : 0;

    // crc
    u16 crc = initial;
    for (u8 i = offset; i < last; ++i)
        crc = crc16_update(crc, buf[i], 8);
    crc ^= xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len];
    buf[last++] = crc >> 8;
    buf[last++] = crc & 0xff;

    // stop TX/RX
    CC2500_Strobe(CC2500_SIDLE);
    // flush tx FIFO
    CC2500_Strobe(CC2500_SFTX);
    // packet length
    CC2500_WriteReg(CC2500_3F_TXFIFO, last + 3);
    // xn297L preamble
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (u8*)"\x71\x0f\x55", 3);
    // xn297 packet
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buf, last);
    // transmit
    CC2500_Strobe(CC2500_STX);
}

// end of xn297 emulation
///////////////////////////

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 sum = packet[0];
    for (int i = 1; i < PACKET_SIZE - 1; i++) sum += packet[i];
    return sum;
}

#define BABS(X) (((X) < 0) ? -(u8)(X) : (X))
#define LIMIT_CHAN(X) (X < CHAN_MIN_VALUE ? CHAN_MIN_VALUE : (X > CHAN_MAX_VALUE ? CHAN_MAX_VALUE : X))
// Channel values are sign + magnitude 8bit values
static u8 convert_channel(u8 num)
{
    s32 ch = LIMIT_CHAN(Channels[num]);
    return (u8)((ch < 0 ? 0x80 : 0) | BABS(ch * 127 / CHAN_MAX_VALUE));
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
#define GET_FLAG_INV(ch, mask) (Channels[ch] < 0 ? mask : 0)
#define CHAN2TRIM(X) (((X) & 0x80 ? (X) : 0x7f - (X)) >> 1)
static void send_packet(u8 bind)
{
    packet[0] = convert_channel(CHANNEL3);          // throttle
    packet[0] = packet[0] & 0x80 ? 0xff - packet[0] : 0x80 + packet[0];
    packet[1] = convert_channel(CHANNEL4);          // rudder
    packet[4] = 0x40;         // rudder does not work well with dyntrim
    packet[2] = 0x80 ^ convert_channel(CHANNEL2);   // elevator
    // driven trims cause issues when headless is enabled
    packet[5] = GET_FLAG(CHANNEL_HEADLESS, 1) ? 0x40 : CHAN2TRIM(packet[2]);  // trim elevator
    packet[3] = convert_channel(CHANNEL1);          // aileron
    packet[6] = GET_FLAG(CHANNEL_HEADLESS, 1) ? 0x40 : CHAN2TRIM(packet[3]);  // trim aileron
    packet[7] = txid[0];
    packet[8] = txid[1];
    packet[9] = txid[2];
    packet[10] = 0;   // overwritten below for feature bits
    packet[11] = 0;
    packet[12] = 0;
    packet[13] = 0;
    packet[14] = 0xc0;  // bind value

    packet[10] += GET_FLAG(CHANNEL_RTH, 0x02)
        | GET_FLAG(CHANNEL_HEADLESS, 0x01);
    if (!bind) {
        packet[14] = 0x04
            | GET_FLAG(CHANNEL_FLIP, 0x01)
            | GET_FLAG(CHANNEL_PICTURE, 0x08)
            | GET_FLAG(CHANNEL_VIDEO, 0x10)
            | GET_FLAG_INV(CHANNEL_LED, 0x20);  // air/ground mode
        if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_PHOENIX) {
            packet[10] |= 0x20  // high rate
                | GET_FLAG(CHANNEL_ARM, 0x80);
            packet[14] &= ~0x24;
        }
    }

    packet[15] = checksum();

    rf_chan++;

    // spacing is 333.25 kHz, must multiply xn297 channel by 3
    CC2500_WriteReg(CC2500_0A_CHANNR, rf_channels[rf_chan / 2] * 3);
    rf_chan %= 2 * sizeof(rf_channels);  // channels repeated

    // Make sure that the radio is in IDLE state before flushing the FIFO
    CC2500_Strobe(CC2500_SIDLE);
    // Flush TX FIFO
    CC2500_Strobe(CC2500_SFTX);

    XN297L_WritePayload(packet, PACKET_SIZE);

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        // Keep transmit power updated
        tx_power = Model.tx_power;
        CC2500_SetPower(tx_power);
    }
}

static void e010_init()
{
    u8 rx_tx_addr[ADDRESS_LENGTH];
    XN297L_init();  // setup cc2500 for xn297L@250kbps emulation
    CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
    memcpy(rx_tx_addr, "\x6d\x6a\x77\x77\x77", sizeof(rx_tx_addr));
    memcpy(rf_channels, "\x36\x3e\x46\x2e", sizeof(rf_channels));
    XN297L_SetTXAddr(rx_tx_addr, sizeof(rx_tx_addr));
    CC2500_SetPower(tx_power);
}

static u16 e010_callback()
{
    switch (phase) {
    case E010_BIND:
        if (counter == 0) {
            memcpy(rf_channels, e010_tx_rf_map[Model.fixed_id % (sizeof(e010_tx_rf_map) / sizeof(e010_tx_rf_map[0]))].rfchan, sizeof(rf_channels));
            phase = E010_DATA;
            PROTOCOL_SetBindState(0);
        }
        else
        {
            send_packet(1);
            counter -= 1;
        }
        break;

    case E010_DATA:
        if (fine != (s8)Model.proto_opts[PROTOOPTS_FREQFINE]) {
            fine = (s8)Model.proto_opts[PROTOOPTS_FREQFINE];
            CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
        }
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;
}

static void initialize_txid()
{
    memcpy(txid, e010_tx_rf_map[Model.fixed_id % (sizeof(e010_tx_rf_map) / sizeof(e010_tx_rf_map[0]))].txid, 2);
    txid[2] = 0x00;
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    fine = (s8)Model.proto_opts[PROTOOPTS_FREQFINE];
    counter = BIND_COUNT;
    initialize_txid();
    e010_init();
    phase = E010_BIND;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);

    CLOCK_StartTimer(INITIAL_WAIT, e010_callback);
}

uintptr_t E010_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
    case PROTOCMD_INIT:  initialize(); return 0;
    case PROTOCMD_DEINIT:
    case PROTOCMD_RESET:
        CLOCK_StopTimer();
        return (CC2500_Reset() ? 1 : -1);
    case PROTOCMD_CHECK_AUTOBIND: return 1;
    case PROTOCMD_BIND:  initialize(); return 0;
    case PROTOCMD_NUMCHAN: return 8;
    case PROTOCMD_DEFAULT_NUMCHAN: return 8;
    case PROTOCMD_CURRENT_ID: return Model.fixed_id;
    case PROTOCMD_GETOPTIONS: return (uintptr_t)e010_opts;
    case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
    case PROTOCMD_CHANNELMAP: return AETRG;
    default: break;
    }
    return 0;
}

#endif
