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
#include "config/tx.h" // for Transmitter


#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 20
#define PACKET_PERIOD    150
#define FQ777_PACKET_PERIOD 150
#define dbgprintf printf
#else
#define BIND_COUNT 80
#define PACKET_PERIOD    13500 // Timeout for callback in uSec
#define FQ777_PACKET_PERIOD 10000
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT       500
#define BIND_PACKET_SIZE   10
#define PACKET_SIZE        12
#define RF_BIND_CHANNEL    0

static const char * const hontai_opts[] = {
  _tr_noop("Format"), "Hontai", "JJRCX1", "FQ777-951", NULL,
  NULL
};
enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};
enum {
    FORMAT_HONTAI = 0,
    FORMAT_JJRCX1,
    FORMAT_FQ777,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);



// For code readability
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
#define CHANNEL_LED         CHANNEL5
#define CHANNEL_ARM         CHANNEL5    // for JJRC X1
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_PICTURE     CHANNEL7
#define CHANNEL_VIDEO       CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10
#define CHANNEL_CALIBRATE   CHANNEL11

enum{
    FLAG_FLIP      = 0x01, 
    FLAG_PICTURE   = 0x02, 
    FLAG_VIDEO     = 0x04, 
    FLAG_HEADLESS  = 0x08, 
    FLAG_RTH       = 0x10,
    FLAG_CALIBRATE = 0x20,
};

enum {
    HonTai_INIT1 = 0,
    HonTai_BIND2,
    HonTai_DATA
};

static u8 phase;
static u8 packet[PACKET_SIZE];
static u8 tx_power;
static u8 txid[5];
static u8 rf_chan = 0; 
static u16 counter;
static u16 packet_period;
static const u8 rf_channels[][3] = {{0x05, 0x19, 0x28},     // Hontai
                                    {0x0a, 0x1e, 0x2d},     // JJRC X1
                                    {0x05, 0x19, 0x28}};    // FQ777-951
static const u8 rx_tx_addr[] = {0xd2, 0xb5, 0x99, 0xb3, 0x4a};
static const u8 addr_vals[4][16] = {
                    {0x24, 0x26, 0x2a, 0x2c, 0x32, 0x34, 0x36, 0x4a,
                     0x4c, 0x4e, 0x54, 0x56, 0x5a, 0x64, 0x66, 0x6a},
                    {0x92, 0x94, 0x96, 0x9a, 0xa4, 0xa6, 0xac, 0xb2,
                     0xb4, 0xb6, 0xca, 0xcc, 0xd2, 0xd4, 0xd6, 0xda},
                    {0x93, 0x95, 0x99, 0x9b, 0xa5, 0xa9, 0xab, 0xad,
                     0xb3, 0xb5, 0xc9, 0xcb, 0xcd, 0xd3, 0xd5, 0xd9},
                    {0x25, 0x29, 0x2b, 0x2d, 0x33, 0x35, 0x49, 0x4b,
                     0x4d, 0x59, 0x5b, 0x65, 0x69, 0x6b, 0x6d, 0x6e}};

// Bit vector from bit position
#define BV(bit) (1 << bit)

// proudly swiped from http://www.drdobbs.com/implementing-the-ccitt-cyclical-redundan/199904926
#define POLY 0x8408
static u16 crc16(u8 *data_p, u32 length)
{
    u8 i;
    u32 data;
    u32 crc;
     
    crc = 0xffff;
     
    if (length == 0) return (~crc);
     
    length -= 2;
    do {
        for (i = 0, data = (u8)0xff & *data_p++;
             i < 8;
             i++, data >>= 1) {
                 if ((crc & 0x0001) ^ (data & 0x0001))
                     crc = (crc >> 1) ^ POLY;
                 else
                     crc >>= 1;
        }
    } while (--length);
     
    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xFF);
    *data_p++ = crc >> 8;
    *data_p   = crc & 0xff;
    return crc;
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static s8 scale_channel(u8 ch, s8 start, s8 end)
{
    s32 range = end - start;
    s32 chanval = Channels[ch];

    if      (chanval < CHAN_MIN_VALUE) chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE) chanval = CHAN_MAX_VALUE;

    s32 round = range < 0 ? 0 : CHAN_RANGE / range;   // channels round up
    if (start < 0) round = CHAN_RANGE / range / 2;    // trims zero centered around zero
    return (range * (chanval - CHAN_MIN_VALUE + round)) / CHAN_RANGE + start;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    if (bind) {
        memcpy(packet, txid, 5);
        memset(&packet[5], 0, 3);
    } else {
        memset(packet,0,PACKET_SIZE);
        packet[3] = scale_channel(CHANNEL3, 0x00, 0x7f) << 1; // throttle
        packet[4] = scale_channel(CHANNEL1, 0x3f, 0x01); // aileron   
        packet[5] = scale_channel(CHANNEL2, 0x01, 0x3f); // elevator
        packet[6] = scale_channel(CHANNEL4, 0x3f, 0x01); // rudder  
        packet[7] = scale_channel(CHANNEL1, -16, 16); // aileron trim
        packet[8] = scale_channel(CHANNEL4, -16, 16); // rudder trim
        packet[9] = scale_channel(CHANNEL2, -16, 16); // elevator trim
        
        // feature flags
        switch(Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_HONTAI:
                packet[0] =  0x0b;
                packet[3] |= GET_FLAG(CHANNEL_PICTURE, 0x01);
                packet[4] |= GET_FLAG(CHANNEL_RTH, 0x80)
                          |  GET_FLAG(CHANNEL_HEADLESS, 0x40);
                packet[5] |= GET_FLAG(CHANNEL_CALIBRATE, 0x80)
                          |  GET_FLAG(CHANNEL_FLIP, 0x40);
                packet[6] |= GET_FLAG(CHANNEL_VIDEO, 0x80);
                break;
            case FORMAT_JJRCX1:
                packet[0] =  GET_FLAG(CHANNEL_ARM, 0x02);
                packet[3] |= GET_FLAG(CHANNEL_PICTURE, 0x01);
                packet[4] |= 0x80; // unknown
                packet[5] |= GET_FLAG(CHANNEL_CALIBRATE, 0x80)
                          |  GET_FLAG(CHANNEL_FLIP, 0x40);
                packet[6] |= GET_FLAG(CHANNEL_VIDEO, 0x80);
                packet[8] =  0xc0 // high rate, no rudder trim
                          |  GET_FLAG(CHANNEL_RTH, 0x02)
                          |  GET_FLAG(CHANNEL_HEADLESS, 0x01);
                break;
            case FORMAT_FQ777:
                // todo: add missing calibration flag
                packet[0] =  GET_FLAG(CHANNEL_PICTURE, 0x01)
                          |  GET_FLAG(CHANNEL_VIDEO, 0x02);
                packet[3] |= GET_FLAG(CHANNEL_FLIP, 0x01);
                packet[4] |= 0xc0; // high rate (mid=0xa0, low=0x60)
                packet[6] |= GET_FLAG(CHANNEL_HEADLESS, 0x40);
                if((packet[4] & 0x3f) > 0x3d && (packet[5] & 0x3f) < 3)
                    packet[5] |= 0x80; // accelerometer recalibration
                break;
        }
    }
    
    crc16(packet, bind ? BIND_PACKET_SIZE : PACKET_SIZE);
    
    // Power on, TX mode, 2byte CRC
    if ( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_HONTAI
      || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777 ) {
        XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    } else {
        NRF24L01_SetTxRxMode(TX_EN);
    }

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? RF_BIND_CHANNEL : rf_channels[Model.proto_opts[PROTOOPTS_FORMAT]][rf_chan++]);
    rf_chan %= sizeof(rf_channels[0]);

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    if ( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_HONTAI
      || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777 ) {
        XN297_WritePayload(packet, bind ? BIND_PACKET_SIZE : PACKET_SIZE);
    } else {
        NRF24L01_WritePayload(packet, bind ? BIND_PACKET_SIZE : PACKET_SIZE);
    }

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }

#ifdef EMULATOR
    dbgprintf("next chan 0x%02x, bind %d, data %02x", bind ? RF_BIND_CHANNEL : rf_channels[Model.proto_opts[PROTOOPTS_FORMAT]][rf_chan], bind, packet[0]);
    for(int i=1; i < (bind ? BIND_PACKET_SIZE : PACKET_SIZE); i++) dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif
}

static void ht_init()
{
    NRF24L01_Initialize();

    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5); 
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7); 
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7); 

    if ( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_HONTAI
      || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777 ) {
        XN297_SetTXAddr(rx_tx_addr, sizeof(rx_tx_addr));
    } else {
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, sizeof(rx_tx_addr));
    }

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                              // Activate feature register
    if ( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_HONTAI
      || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777 ) {
        NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);  // no retransmits
        NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
        NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
        NRF24L01_Activate(0x73);                          // Deactivate feature register
    } else {
        NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xff);  // JJRC uses dynamic payload length
        NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);       // match other stock settings even though AA disabled...
        NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
    }
}

static void ht_init2()
{
    u8 data_tx_addr[] = {0x2a, 0xda, 0xa5, 0x25, 0x24};

    data_tx_addr[0] = addr_vals[0][ txid[3]       & 0x0f];
    data_tx_addr[1] = addr_vals[1][(txid[3] >> 4) & 0x0f];
    data_tx_addr[2] = addr_vals[2][ txid[4]       & 0x0f];
    data_tx_addr[3] = addr_vals[3][(txid[4] >> 4) & 0x0f];

    if ( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_HONTAI
      || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777 ) {
        XN297_SetTXAddr(data_tx_addr, sizeof(data_tx_addr));
    } else {
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, data_tx_addr, sizeof(data_tx_addr));
    }
}

static u16 ht_callback()
{
    switch (phase) {
    case HonTai_INIT1:
        phase = HonTai_BIND2;
        break;

    case HonTai_BIND2:
        if (counter == 0) {
            ht_init2();
            phase = HonTai_DATA;
            PROTOCOL_SetBindState(0);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case HonTai_DATA:
        send_packet(0);
        break;
    }
    return packet_period;
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
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    if ( Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_HONTAI 
      || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777 ) {
        txid[0] = 0x4c; // first three bytes some kind of model id? - set same as stock tx
        txid[1] = 0x4b;
        txid[2] = 0x3a;
    } else {
        txid[0] = 0x4b; // JJRC X1
        txid[1] = 0x59;
        txid[2] = 0x3a;
    }
    txid[3] = (lfsr >> 8 ) & 0xff;
    txid[4] = lfsr & 0xff; 
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_FQ777)
        packet_period = FQ777_PACKET_PERIOD;
    else
        packet_period = PACKET_PERIOD;
    counter = BIND_COUNT;
    
    initialize_txid();
    ht_init();
    phase = HonTai_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * packet_period / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, ht_callback);
}

uintptr_t HonTai_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;  // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 11;  // A, E, T, R, light, flip, photo, video, headless, RTH, calibrate
        case PROTOCMD_DEFAULT_NUMCHAN: return 11;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)hontai_opts;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
