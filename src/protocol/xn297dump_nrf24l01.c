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
#include "config/model.h"
#include "rftools.h"

#ifdef PROTO_HAS_NRF24L01
#if SUPPORT_XN297DUMP

#ifdef MODULAR
    struct Xn297dump xn297dump;
#endif

#define INITIAL_WAIT       500
#define ADDRESS_LENGTH     5
#define PERIOD_DUMP        100
#define MAX_PACKET_LEN     32
#define CRC_LENGTH         2
#define MAX_RF_CHANNEL     84
#define DUMP_RETRIES       10  // stay on channels long enough to capture packets

static const char *const xn297dump_opts[] = {
    _tr_noop("Bitrate"), "1 Mbps", "2 Mbps", "250 Kbps", NULL,
    _tr_noop("Address"), "5 byte", "4 byte", "3 byte", NULL,
    _tr_noop("Scrambled"), _tr_noop("Yes"), _tr_noop("No"), NULL,
    _tr_noop("Retries"), "10", "245", NULL,
    _tr_noop("Intvl Scan"), "5", "20", NULL,
    NULL
};

enum {
    PROTOOPTS_BITRATE = 0,
    PROTOOPTS_ADDRESS,
    PROTOOPTS_UNSCRAMBLED,
    PROTOOPTS_RETRIES,
    PROTOOPTS_INTERVAL,
    LAST_PROTO_OPT,
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// Bit vector from bit position
#define BV(bit) (1 << bit)

enum {
    STATE_GET_PACKET = 0,
    STATE_PROCESS_PACKET,
    STATE_DELAY,
    STATE_DELAY2,
    STATE_INTERVAL,
};

static u8 phase, cur_channel, dumps, new_packet;
static u8 raw_packet[MAX_PACKET_LEN];
static u32 time_ms;

static u8 get_packet(void)
{
#ifdef EMULATOR
    for  (int i = 0; i < MAX_PACKET_LEN; i++)
        raw_packet[i] = rand32() % 0xFF;
    return 1;
#else
    if (xn297dump.channel > MAX_RF_CHANNEL)
        xn297dump.channel = 0;
    if (xn297dump.channel != cur_channel) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, xn297dump.channel);
        cur_channel = xn297dump.channel;
        memset(raw_packet, 0, MAX_PACKET_LEN);
        xn297dump.crc_valid = 0;
        NRF24L01_FlushRx();
    }
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {
        if (!NRF24L01_ReadReg(NRF24L01_09_CD) && xn297dump.mode == XN297DUMP_SCAN) {
            NRF24L01_FlushRx();
            return 0;  // ignore weak signals in scan mode
        }
        NRF24L01_ReadPayload(raw_packet, xn297dump.pkt_len);
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);
        NRF24L01_FlushRx();
            return 1;
    }
    return 0;
#endif
}

static u32 measure_interval(u8 average)
{
    u32 hits;
    u32 result;
    if (!average)
        return 0;  // we shouldn't divide by 0...
    result = 0;
    for (int k = 0; k < average; k++) {
        time_ms = CLOCK_getms();
        hits = 0;
        for (u32 i = 0; i < 200000; i++) {
            CLOCK_ResetWatchdog();
            if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {
                NRF24L01_FlushRx();
                NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);
                hits++;
            }
        }
        result += 1000 * (CLOCK_getms()-time_ms) / hits;
    }
    return result /= average;
}

static u8 process_packet(void)
{
    int i;
    u16 packet_crc = 0;
    u16 crc = 0xb5d2;

    // unscramble address and reverse order
    for (i = 0; i < ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS]; i++) {
        crc = crc16_update(crc, raw_packet[i], 8);
        if (Model.proto_opts[PROTOOPTS_UNSCRAMBLED])
            xn297dump.packet[ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS] - i - 1] = raw_packet[i];
        else
            xn297dump.packet[ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS] - i - 1] = raw_packet[i] ^ xn297_scramble[i];
    }

    // unscramble payload
    for (i = ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS]; i < xn297dump.pkt_len - CRC_LENGTH; i++) {
        crc = crc16_update(crc, raw_packet[i], 8);
        if (Model.proto_opts[PROTOOPTS_UNSCRAMBLED])
            xn297dump.packet[i] = bit_reverse(raw_packet[i]);
        else
            xn297dump.packet[i] = bit_reverse(raw_packet[i] ^ xn297_scramble[i]);
    }

    // check crc
    packet_crc = ((u16)(raw_packet[xn297dump.pkt_len - 2]) << 8)
                | ((u16)(raw_packet[xn297dump.pkt_len - 1]) & 0xff);
    xn297dump.packet[xn297dump.pkt_len - 2] = raw_packet[xn297dump.pkt_len - 2];
    xn297dump.packet[xn297dump.pkt_len - 1] = raw_packet[xn297dump.pkt_len - 1];
    if (Model.proto_opts[PROTOOPTS_UNSCRAMBLED])
        crc ^= xn297_crc_xorout[xn297dump.pkt_len-(ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS] - 2 + CRC_LENGTH)];
    else
        crc ^= xn297_crc_xorout_scrambled[xn297dump.pkt_len-(ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS] - 2 + CRC_LENGTH)];

    // clear invalid packets
    for (i = xn297dump.pkt_len; i < MAX_PACKET_LEN; i++)
        xn297dump.packet[i] = 0;

    if (packet_crc == crc) {
        if (xn297dump.scan && xn297dump.mode == XN297DUMP_SCAN)
            xn297dump.scan = 0;  // Stop scanning on valid CRC
        return 1;
    } else {
        return 0;
    }
}

static void xn297dump_init()
{
    u8 rx_addr[] = { 0x55, 0x0f, 0x71 };  // preamble is 0x550f71 for XN297

    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(RX_EN);

    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);            // Disable CRC check for dumps
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);             // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);          // 3 byte RX/TX address
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_addr, 3);     // set up RX address to xn297 preamble
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, MAX_PACKET_LEN);
    NRF24L01_SetBitrate(Model.proto_opts[PROTOOPTS_BITRATE]);
    NRF24L01_Activate(0x73);                                // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);             // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, xn297dump.channel);
}

static u16 xn297dump_callback()
{
    switch (phase) {
        case STATE_GET_PACKET:
            if (xn297dump.mode == XN297DUMP_SCAN && xn297dump.scan) {
                dumps++;
                if (dumps > (u8)Model.proto_opts[PROTOOPTS_RETRIES] + DUMP_RETRIES) {
                    dumps = 0;
                    xn297dump.channel++;
                    return PERIOD_DUMP;
                }
            }
            new_packet = get_packet();
            /* FALLTHROUGH */
        case STATE_PROCESS_PACKET:
            if (new_packet)
                xn297dump.crc_valid = process_packet();  // process_packet will set scan = 0 if valid CRC is found to stop scanning
            switch (xn297dump.mode) {
                case XN297DUMP_SCAN:
                    if (xn297dump.scan) {
                        xn297dump.pkt_len--;
                        if (xn297dump.pkt_len > ADDRESS_LENGTH - Model.proto_opts[PROTOOPTS_ADDRESS] + CRC_LENGTH) {
                            phase = STATE_PROCESS_PACKET;
                        } else {
                            xn297dump.pkt_len = MAX_PACKET_LEN;
                            phase = STATE_GET_PACKET;
                        }
                    } else {
                        phase = STATE_GET_PACKET;
                    }
                    break;
                case XN297DUMP_INTERVAL:
                    if (xn297dump.scan)
                        phase = STATE_DELAY;
                    else
                        phase = STATE_GET_PACKET;
                    break;
                default:
                    phase = STATE_GET_PACKET;
            }
            break;
        case STATE_DELAY:
            phase = STATE_DELAY2;
            return 65355;  // Give status display some time to update
        case STATE_DELAY2:
            phase = STATE_INTERVAL;
            return 65355;  // Give status display some more time to update
        case STATE_INTERVAL:
            xn297dump.scan = 0;  // only run this once;
            xn297dump.interval = measure_interval(Model.proto_opts[PROTOOPTS_INTERVAL]);
            phase = STATE_GET_PACKET;
    }
    return PERIOD_DUMP;
}

static void initialize()
{
    CLOCK_StopTimer();
    xn297dump_init();
    cur_channel = xn297dump.channel;
    xn297dump.crc_valid = 0;
    dumps = 0;
    new_packet = 0;
    memset(raw_packet, 0, MAX_PACKET_LEN);
    phase = STATE_GET_PACKET;
    CLOCK_StartTimer(INITIAL_WAIT, xn297dump_callback);
}

uintptr_t XN297Dump_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
    case PROTOCMD_INIT:
        initialize();
        return 0;
    case PROTOCMD_DEINIT:
    case PROTOCMD_RESET:
        CLOCK_StopTimer();
        return (NRF24L01_Reset()? 1 : -1);
    case PROTOCMD_CHECK_AUTOBIND:
        return 1;     // always Autobind
    case PROTOCMD_BIND:
        initialize();
        return 0;
    case PROTOCMD_NUMCHAN:
        return NUM_CHANNELS;
    case PROTOCMD_DEFAULT_NUMCHAN:
        return NUM_CHANNELS;
    case PROTOCMD_CURRENT_ID:
    case PROTOCMD_GETOPTIONS:
        return (uintptr_t)xn297dump_opts;
    case PROTOCMD_TELEMETRYSTATE:
        return PROTO_TELEM_UNSUPPORTED;
    case PROTOCMD_CHANNELMAP:
        return UNCHG;
    default:
        break;
    }
    return 0;
}
#endif  // SUPPORT_XN297DUMP
#endif  // PROTO_HAS_NRF24L01
