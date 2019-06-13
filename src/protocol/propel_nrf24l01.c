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
#include "config/tx.h"          // for Transmitter
#include "telemetry.h"

#ifdef PROTO_HAS_NRF24L01

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define PACKET_PERIOD    450
#define BIND_PERIOD      450
#define dbgprintf printf
#else
// Timeout for callback in uSec
#define PACKET_PERIOD    10000
#define BIND_PERIOD      1000

// printf inside an interrupt handler is really dangerous
// this shouldn't be enabled even in debug builds without explicitly
// turning it on
#define dbgprintf if (0) printf
#endif


// For code readability
enum {
    CHANNEL1 = 0,  // Aileron
    CHANNEL2,      // Elevator
    CHANNEL3,      // Throttle
    CHANNEL4,      // Rudder
    CHANNEL5,      // Leds
    CHANNEL6,      // Roll ccw
    CHANNEL7,      // Roll cw
    CHANNEL8,      // Altitude hold
    CHANNEL9,      // Calibrate
    CHANNEL10,
    CHANNEL11,
    CHANNEL12,
    CHANNEL13,
    CHANNEL14,
};
#define CHANNEL_LEDS        CHANNEL5
#define CHANNEL_ROLLCCW     CHANNEL6
#define CHANNEL_ROLLCW      CHANNEL7
#define CHANNEL_ALTHOLD     CHANNEL8
#define CHANNEL_CALIBRATE   CHANNEL9

enum {
    PROPEL_BIND1 = 0,
    PROPEL_BIND2,
    PROPEL_DATA1,
};


#define PACKET_SIZE        14
#define RF_NUM_CHANNELS    4
#define ADDRESS_LENGTH     5

static u8 state;
static u8 packet[PACKET_SIZE];
static u8 tx_power;
static u8 rx_tx_addr[ADDRESS_LENGTH];

// equations for checksum check byte from truth table
// (1)  z =  a && !b
//       ||  a && !c && !d
//       ||  a && !c && !g
//       ||  a && !d && !g
//       ||  a && !c && !h
//       ||  a && !g && !h
//       || !a &&  b &&  c &&  g
//       || !a &&  b &&  c &&  d &&  h
//       || !a &&  b &&  d &&  g &&  h;
//
// (2)  y = !b && !c && !d
//       ||  b &&  c &&  g
//       || !b && !c && !g
//       || !b && !d && !g
//       || !b && !c && !h
//       || !b && !g && !h
//       ||  b &&  c &&  d &&  h
//       ||  b &&  d &&  g &&  h;
//
// (3)  x = !c && !d &&  g
//       ||  c && !d && !g
//       || !c &&  g && !h
//       ||  c && !g && !h
//       ||  c &&  d &&  g &&  h
//       || !c &&  d && !g &&  h;
//
// (4)  w =  d &&  h
//       || !d && !h;
//
// (5)  v = !e;
//
// (6)  u =  f;
//
// (7)  t = !g;
//
// (8)  s =  h;

typedef union {
    struct {
        u8 h:1;
        u8 g:1;
        u8 f:1;
        u8 e:1;
        u8 d:1;
        u8 c:1;
        u8 b:1;
        u8 a:1;
    } bits;
    u8 byte:8;
} byte_bits_t;

static u16 checksum()
{
    u8 sum = packet[0];
    for (int i = 1; i < PACKET_SIZE - 2; i++)
        sum += packet[i];

    byte_bits_t in  = { .byte = sum };
    byte_bits_t out = { .byte = sum ^ 0x0a};
    out.bits.d = !(in.bits.d ^ in.bits.h);
    out.bits.c = (!in.bits.c && !in.bits.d &&  in.bits.g)
              ||  (in.bits.c && !in.bits.d && !in.bits.g)
              || (!in.bits.c &&  in.bits.g && !in.bits.h)
              ||  (in.bits.c && !in.bits.g && !in.bits.h)
              ||  (in.bits.c &&  in.bits.d &&  in.bits.g &&  in.bits.h)
              || (!in.bits.c &&  in.bits.d && !in.bits.g &&  in.bits.h);
    out.bits.b = (!in.bits.b && !in.bits.c && !in.bits.d)
              ||  (in.bits.b &&  in.bits.c &&  in.bits.g)
              || (!in.bits.b && !in.bits.c && !in.bits.g)
              || (!in.bits.b && !in.bits.d && !in.bits.g)
              || (!in.bits.b && !in.bits.c && !in.bits.h)
              || (!in.bits.b && !in.bits.g && !in.bits.h)
              ||  (in.bits.b &&  in.bits.c &&  in.bits.d &&  in.bits.h)
              ||  (in.bits.b &&  in.bits.d &&  in.bits.g &&  in.bits.h);
    out.bits.a =  (in.bits.a && !in.bits.b)
              ||  (in.bits.a && !in.bits.c && !in.bits.d)
              ||  (in.bits.a && !in.bits.c && !in.bits.g)
              ||  (in.bits.a && !in.bits.d && !in.bits.g)
              ||  (in.bits.a && !in.bits.c && !in.bits.h)
              ||  (in.bits.a && !in.bits.g && !in.bits.h)
              || (!in.bits.a &&  in.bits.b &&  in.bits.c &&  in.bits.g)
              || (!in.bits.a &&  in.bits.b &&  in.bits.c &&  in.bits.d &&  in.bits.h)
              || (!in.bits.a &&  in.bits.b &&  in.bits.d &&  in.bits.g &&  in.bits.h);

    return (sum << 8) | (out.byte & 0xff);
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

#define DYNTRIM(chval) ((u8)((chval >> 2) & 0xfc))
#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void bind_packet(u8 type)
{
    memset(packet, 0, PACKET_SIZE);

    packet[0] = 0xd0;
    memcpy(&packet[1], rx_tx_addr, 4);  // only 4 bytes sent of 5-byte address
    packet[9] = 0x03;
    packet[11] = type ? 0x05 : 0x01;

    u16 check = checksum();
    packet[12] = check >> 8;
    packet[13] = check & 0xff;
Telemetry.value[TELEM_FRSKY_CELL1] = packet[13];
TELEMETRY_SetUpdated(TELEM_FRSKY_CELL1);

#ifdef EMULATOR
    dbgprintf("type %d, data %02x", type, packet[0]);
    for (int i = 1; i < PACKET_SIZE; i++)
        dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif
}

static void data_packet()
{
    memset(packet, 0, PACKET_SIZE);

    packet[0] = 0xc0;
    packet[1] = scale_channel(CHANNEL3, 0x2f, 0xcf);    // throttle
    packet[2] = scale_channel(CHANNEL4, 0x2f, 0xcf);    // rudder
    packet[3] = scale_channel(CHANNEL2, 0x2f, 0xcf);    // elevator
    packet[4] = scale_channel(CHANNEL1, 0x2f, 0xcf);    // aileron
    packet[5] = 0x40;
    packet[6] = 0x40;
    packet[7] = 0x40;
    packet[8] = 0x40;
    packet[9] = 0x02    //  always fast speed
              | GET_FLAG(CHANNEL_CALIBRATE, 0x04)
              | GET_FLAG(CHANNEL_ROLLCCW  , 0x80)
              | GET_FLAG(CHANNEL_ROLLCW   , 0x40)
              | GET_FLAG(CHANNEL_ALTHOLD  , 0x20);
    packet[10] = GET_FLAG(CHANNEL_LEDS, 0x80);
    packet[11] = 1;

    u16 check = checksum();
    packet[12] = check >> 8;
    packet[13] = check & 0xff;
Telemetry.value[TELEM_FRSKY_CELL1] = packet[13];
TELEMETRY_SetUpdated(TELEM_FRSKY_CELL1);

#ifdef EMULATOR
    dbgprintf("data %02x", packet[0]);
    for (int i = 1; i < PACKET_SIZE; i++)
        dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif
}

static void process_rx(void)
{
}

static void propel_init()
{
    const u8 address[] = {0x73, 0xd3, 0x31, 0x30, 0x11};
    memcpy(rx_tx_addr, address, ADDRESS_LENGTH);

    NRF24L01_Initialize();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3f);       // AA on all pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3f);   // Enable all pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);    // Enable all pipes
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x36);  // retransmit 1ms, 6 times
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x23);       // bind channel
    NRF24L01_SetBitrate(NRF24L01_BR_1M);              // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);       // Enable dynamic payload length
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Enable all features
    // Beken 2425 register bank 1 initialized here in stock tx capture
    // Hopefully won't matter for nRF compatibility
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0f);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, ADDRESS_LENGTH);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, ADDRESS_LENGTH);
    NRF24L01_FlushTx();
    NRF24L01_SetTxRxMode(TX_EN);
}


#define BV(bit) (1 << bit)
#define CLEAR_MASK  (BV(NRF24L01_07_RX_DR) | BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))
static u16 propel_callback()
{
    static u8 rf_channels[] = {0x39, 0x2A, 0x18, 0x23};
    static u8 rf_chan;
    u8 status;

    switch (state) {
    case PROPEL_BIND1:
        NRF24L01_WriteReg(NRF24L01_07_STATUS, CLEAR_MASK);
        NRF24L01_FlushTx();
        bind_packet(1);
        NRF24L01_WritePayload(packet, PACKET_SIZE);
        state = PROPEL_BIND2;
        return BIND_PERIOD;
        break;

    case PROPEL_BIND2:
        status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
Telemetry.value[TELEM_FRSKY_CELL2] = status;
TELEMETRY_SetUpdated(TELEM_FRSKY_CELL2);
        if (BV(NRF24L01_07_MAX_RT) & status) {
            state = PROPEL_BIND1;
            return BIND_PERIOD;
        }
Telemetry.value[TELEM_FRSKY_CELL3] = status;
TELEMETRY_SetUpdated(TELEM_FRSKY_CELL3);
        if (!(BV(NRF24L01_07_RX_DR) & status))
            return BIND_PERIOD;
Telemetry.value[TELEM_FRSKY_CELL4] = status;
TELEMETRY_SetUpdated(TELEM_FRSKY_CELL4);

        PROTOCOL_SetBindState(0);
        NRF24L01_ReadPayload(packet, PACKET_SIZE);
        NRF24L01_WriteReg(NRF24L01_07_STATUS, CLEAR_MASK);
        NRF24L01_FlushTx();
        bind_packet(0);
        NRF24L01_WritePayload(packet, PACKET_SIZE);
        state = PROPEL_DATA1;
        break;

    case PROPEL_DATA1:
        if (BV(NRF24L01_07_RX_DR) & NRF24L01_ReadReg(NRF24L01_07_STATUS)) {
            NRF24L01_ReadPayload(packet, PACKET_SIZE);
            process_rx();
        }
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[rf_chan++]);
        rf_chan %= sizeof rf_channels / sizeof rf_channels[0];
        NRF24L01_WriteReg(NRF24L01_07_STATUS, CLEAR_MASK);
        NRF24L01_FlushTx();
        data_packet();
        NRF24L01_WritePayload(packet, PACKET_SIZE);

        break;
    }
    return PACKET_PERIOD;
}

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
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i)
        rand32_r(&lfsr, 0);
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;

    initialize_txid();
    propel_init();
    state = PROPEL_BIND1;

    PROTOCOL_SetBindState(0xFFFFFFFF);
    CLOCK_StartTimer(500, propel_callback);
}

uintptr_t Propel_Cmds(enum ProtoCmds cmd)
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
        return 14;
    case PROTOCMD_DEFAULT_NUMCHAN:
        return 14;
    case PROTOCMD_CURRENT_ID:
        return Model.fixed_id;
    case PROTOCMD_CHANNELMAP:
        return AETRG;
    case PROTOCMD_TELEMETRYSTATE:
        return PROTO_TELEM_ON;
    case PROTOCMD_TELEMETRYTYPE:
        return TELEM_FRSKY;
    default:
        break;
    }
    return 0;
}
#endif
