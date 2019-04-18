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
    // printf inside an interrupt handler is really dangerous
    // this shouldn't be enabled even in debug builds without explicitly
    // turning it on
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
static u8 calibration[RF_NUM_CHANNELS];
static u8 calibration_fscal2;
static u8 calibration_fscal3;

// dumped from E010 and H36 stock transmitters
static const struct {
    u8 txid[2];
    u8 rfchan[RF_NUM_CHANNELS];
}
e010_tx_rf_map[] = {{{0x4F, 0x1C}, {0x3A, 0x35, 0x4A, 0x45}},
                    {{0x90, 0x1C}, {0x2E, 0x36, 0x3E, 0x46}},
                    {{0x24, 0x36}, {0x32, 0x3E, 0x42, 0x4E}},
                    {{0x7A, 0x40}, {0x2E, 0x3C, 0x3E, 0x4C}},
                    {{0x61, 0x31}, {0x2F, 0x3B, 0x3F, 0x4B}},
                    {{0x5D, 0x37}, {0x33, 0x3B, 0x43, 0x4B}},
                    {{0xFD, 0x4F}, {0x33, 0x3B, 0x43, 0x4B}},
                    {{0x86, 0x3C}, {0x34, 0x3E, 0x44, 0x4E}}};

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 sum = packet[0];
    for (int i=1; i < PACKET_SIZE-1; i++) sum += packet[i];
    return sum;
}

#define BABS(X) (((X) < 0) ? -(u8)(X) : (X))
#define LIMIT_CHAN(X) (X < CHAN_MIN_VALUE ? CHAN_MIN_VALUE : (X > CHAN_MAX_VALUE ? CHAN_MAX_VALUE : X))
// Channel values are sign + magnitude 8bit values
static u8 convert_channel(u8 num)
{
    s32 ch = LIMIT_CHAN(Channels[num]);
    return (u8) ((ch < 0 ? 0x80 : 0) | BABS(ch * 127 / CHAN_MAX_VALUE));
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
                       |  GET_FLAG(CHANNEL_ARM, 0x80);
            packet[14] &= ~0x24;
        }
    }

    packet[15] = checksum();

    // channel hopping
    rf_chan++;
    CC2500_WriteReg(CC2500_23_FSCAL3, calibration_fscal3);
    CC2500_WriteReg(CC2500_24_FSCAL2, calibration_fscal2);
    CC2500_WriteReg(CC2500_25_FSCAL1, calibration[rf_chan / 2]);
    XN297L_SetChannel(rf_channels[rf_chan / 2]);
    rf_chan %= 2 * sizeof(rf_channels);  // channels repeated

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

// calibrate used RF channels for faster hopping
static void calibrate_rf_chans()
{
    for (int c = 0; c < RF_NUM_CHANNELS; c++) {
        CLOCK_ResetWatchdog();
        CC2500_Strobe(CC2500_SIDLE);
        XN297L_SetChannel(rf_channels[c]);
        CC2500_Strobe(CC2500_SCAL);
        usleep(900);
        calibration[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
    }
    calibration_fscal3 = CC2500_ReadReg(CC2500_23_FSCAL3);  // only needs to be done once
    calibration_fscal2 = CC2500_ReadReg(CC2500_24_FSCAL2);  // only needs to be done once
    CC2500_Strobe(CC2500_SIDLE);
}


static void e010_init()
{
    u8 rx_tx_addr[ADDRESS_LENGTH];
    XN297L_init(XN297_SCRAMBLED, XN297_CRC);  // setup cc2500 for xn297L@250kbps emulation, scrambled, crc enabled
    CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
    memcpy(rx_tx_addr, "\x6d\x6a\x77\x77\x77", sizeof(rx_tx_addr));
    memcpy(rf_channels, "\x36\x3e\x46\x2e", sizeof(rf_channels));
    XN297L_SetTXAddr(rx_tx_addr, sizeof(rx_tx_addr));
    CC2500_SetPower(tx_power);
    calibrate_rf_chans();
    CC2500_SetTxRxMode(TX_EN);
}

static u16 e010_callback()
{
    switch (phase) {
        case E010_BIND:
            if (counter == 0) {
                memcpy(rf_channels, e010_tx_rf_map[Model.fixed_id % (sizeof(e010_tx_rf_map)/sizeof(e010_tx_rf_map[0]))].rfchan, sizeof(rf_channels));
                calibrate_rf_chans();
                phase = E010_DATA;
                PROTOCOL_SetBindState(0);
            } else {
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
    memcpy(txid, e010_tx_rf_map[Model.fixed_id % (sizeof(e010_tx_rf_map)/sizeof(e010_tx_rf_map[0]))].txid, 2);
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
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (CC2500_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(1); return 0;
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
