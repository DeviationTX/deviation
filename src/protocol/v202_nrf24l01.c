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
  #define V202_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "telemetry.h"

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
#define BIND_COUNT 5
#else
#define BIND_COUNT 1000
#endif

// Timeout for callback in uSec, 4ms=4000us for V202
#define PACKET_PERIOD 4000
// Time to wait for packet to be sent (no ACK, so very short)
#define PACKET_CHKTIME  100

// Every second
#define BLINK_COUNT 250
// ~ every 0.25 sec
#define BLINK_COUNT_MIN 64
// ~ every 2 sec
#define BLINK_COUNT_MAX 512


enum {
    // flags going to byte 14
    FLAG_CAMERA = 0x01, // also automatic Missile Launcher and Hoist in one direction
    FLAG_VIDEO  = 0x02, // also Sprayer, Bubbler, Missile Launcher(1), and Hoist in the other dir.
    FLAG_FLIP   = 0x04,
    FLAG_UNK9   = 0x08,
    FLAG_LED    = 0x10,
    FLAG_UNK10  = 0x20,
    FLAG_BIND   = 0xC0,
    // flags going to byte 10
    FLAG_HEADLESS  = 0x0200,
    FLAG_MAG_CAL_X = 0x0800,
    FLAG_MAG_CAL_Y = 0x2000,
    FLAG_EMERGENCY = 0x8000, // JXD-506
};

enum {
    // flags going to byte 11 (JXD-506)
    FLAG_START_STOP = 0x40,
    FLAG_CAMERA_UP = 0x0001,   
    FLAG_CAMERA_DN = 0x0002,
};

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
    CHANNEL7,
    CHANNEL8,
    CHANNEL9,
    CHANNEL10,
    CHANNEL11,
    CHANNEL12
};

#define PAYLOADSIZE 16

static u8 packet[PAYLOADSIZE];
static u8 packet_sent;
static u8 tx_id[3];
static u8 rf_ch_num;
static u16 counter;
static u32 packet_counter;
static u8 tx_power;
//static u8 auto_flip; // Channel 6 <= 0 - disabled > 0 - enabled
static u16 led_blink_count;
static u8  throttle, rudder, elevator, aileron;
static u16 flags;


//
static u8 phase;
enum {
    V202_INIT2 = 0,
    V202_INIT2_NO_BIND,
    V202_BIND1,
    V202_BIND2,
    V202_DATA
};

//#define USE_BLINK_OPTION

static const char * const v202_opts[] = {
  _tr_noop("Format"), "V202", "JXD-506", NULL,
  _tr_noop("Re-bind"),  _tr_noop("No"), _tr_noop("Yes"), NULL,
  _tr_noop("250kbps"),  _tr_noop("No"), _tr_noop("Yes"), NULL,
#if defined(USE_BLINK_OPTION)
  _tr_noop("Blink"),  _tr_noop("No"), _tr_noop("Yes"), NULL,
#endif
  NULL
};
enum {
    PROTOOPTS_FORMAT = 0,
    PROTOOPTS_STARTBIND,
    PROTOOPTS_BITRATE,
#if defined(USE_BLINK_OPTION)
    PROTOOPTS_USEBLINK,
#endif
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum{
    FORMAT_V202 = 0,
    FORMAT_JXD506,
};

enum {
    STARTBIND_NO  = 0,
    STARTBIND_YES = 1,
};
enum {
    BITRATE_1MBPS   = 0,
    BITRATE_250KBPS = 1
};
#if defined(USE_BLINK_OPTION)
enum {
    USEBLINK_NO  = 0,
    USEBLINK_YES = 1,
};
#endif

// static u32 bind_count;

// This is frequency hopping table for V202 protocol
// The table is the first 4 rows of 32 frequency hopping
// patterns, all other rows are derived from the first 4.
// For some reason the protocol avoids channels, dividing
// by 16 and replaces them by subtracting 3 from the channel
// number in this case.
// The pattern is defined by 5 least significant bits of
// sum of 3 bytes comprising TX id
static const u8 freq_hopping[][16] = {
 { 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
   0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
 { 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
   0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
 { 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
   0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
 { 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
   0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};
static u8 rf_channels[16];

// Bit vector from bit position
#define BV(bit) (1 << bit)

// Packet ack status values
enum {
    PKT_PENDING = 0,
    PKT_ACKED,
    PKT_TIMEOUT
};

static u8 packet_ack()
{
    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
    case BV(NRF24L01_07_TX_DS):
        return PKT_ACKED;
    case BV(NRF24L01_07_MAX_RT):
        return PKT_TIMEOUT;
    }
    return PKT_PENDING;
}

static void v202_init()
{
    NRF24L01_Initialize();

    // 2-bytes CRC, radio off
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xFF); // 4ms retransmit t/o, 15 tries
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);      // Channel 8
    NRF24L01_SetBitrate(Model.proto_opts[PROTOOPTS_BITRATE] == BITRATE_250KBPS ? NRF24L01_BR_250K: NRF24L01_BR_1M);
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
//    NRF24L01_WriteReg(NRF24L01_08_OBSERVE_TX, 0x00); // no write bits in this field
//    NRF24L01_WriteReg(NRF24L01_00_CD, 0x00);         // same
    NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3); // LSB byte of pipe 2 receive address
    NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
    NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
    NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);   // bytes of data payload for pipe 1
    NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here
    u8 rx_tx_addr[] = {0x66, 0x88, 0x68, 0x68, 0x68};
    u8 rx_p1_addr[] = {0x88, 0x66, 0x86, 0x86, 0x86};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_p1_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}

static void V202_init2()
{
    NRF24L01_FlushTx();
    packet_sent = 0;
    rf_ch_num = 0;

    // Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
    u8 config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config);
    // Implicit delay in callback
    // delayMicroseconds(150);
}

static void set_tx_id(u32 id)
{
    u8 sum;
    tx_id[0] = (id >> 16) & 0xFF;
    tx_id[1] = (id >> 8) & 0xFF;
    tx_id[2] = (id >> 0) & 0xFF;
    sum = tx_id[0] + tx_id[1] + tx_id[2];
    // Base row is defined by lowest 2 bits
    const u8 *fh_row = freq_hopping[sum & 0x03];
    // Higher 3 bits define increment to corresponding row
    u8 increment = (sum & 0x1e) >> 2;
    for (u8 i = 0; i < 16; ++i) {
        u8 val = fh_row[i] + increment;
        // Strange avoidance of channels divisible by 16
        rf_channels[i] = (val & 0x0f) ? val : val - 3;
    }
}

static void add_pkt_checksum()
{
  u8 sum = 0;
  for (u8 i = 0; i < 15;  ++i) sum += packet[i];
  packet[15] = sum;
}


static u8 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u8) (((ch * 0xFF / CHAN_MAX_VALUE) + 0x100) >> 1);
}


static void read_controls(u8* throttle, u8* rudder, u8* elevator, u8* aileron,
                          u16* flags, u16* led_blink_count)
{
    // Protocol is registered AETRG, that is
    // Aileron is channel 0, Elevator - 1, Throttle - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range
    u8 a;

    // Channel 3
    *throttle = convert_channel(CHANNEL3);

    // Channel 4
    a = convert_channel(CHANNEL4);
    *rudder = a < 0x80 ? 0x7f - a : a;

    // Channel 2
    a = convert_channel(CHANNEL2);
    *elevator = a < 0x80 ? 0x7f - a : a;

    // Channel 1
    a = convert_channel(CHANNEL1);
    *aileron = a < 0x80 ? 0x7f - a : a;

    // Channel 5
    // 512 - slow blinking (period 4ms*2*512 ~ 4sec), 64 - fast blinking (4ms*2*64 ~ 0.5sec)
    u16 new_led_blink_count;
    s32 ch = Channels[CHANNEL5];
    if (ch == CHAN_MIN_VALUE) {
        new_led_blink_count = BLINK_COUNT_MAX + 1;
    } else if (ch == CHAN_MAX_VALUE) {
        new_led_blink_count = BLINK_COUNT_MIN - 1;
    } else {
#if defined(USE_BLINK_OPTION)
        if (Model.proto_opts[PROTOOPTS_USEBLINK] == USEBLINK_YES) {
#endif
            new_led_blink_count = (BLINK_COUNT_MAX+BLINK_COUNT_MIN)/2 -
                ((s32) Channels[CHANNEL5] * (BLINK_COUNT_MAX-BLINK_COUNT_MIN) / (2*CHAN_MAX_VALUE));
#if defined(USE_BLINK_OPTION)
        } else {
            new_led_blink_count = ch <=0 ? BLINK_COUNT_MAX + 1 : BLINK_COUNT_MIN - 1;
        }
#endif
    }
    if (*led_blink_count != new_led_blink_count) {
        if (counter > new_led_blink_count) counter = new_led_blink_count;
        *led_blink_count = new_led_blink_count;
    }

    int num_channels = Model.num_channels;


    // Channel 6
    if (Channels[CHANNEL6] <= 0) *flags &= ~FLAG_FLIP;
    else *flags |= FLAG_FLIP;

    // Channel 7
    if (num_channels < 7 || Channels[CHANNEL7] <= 0) *flags &= ~FLAG_CAMERA;
    else *flags |= FLAG_CAMERA;

    // Channel 8
    if (num_channels < 8 || Channels[CHANNEL8] <= 0) *flags &= ~FLAG_VIDEO;
    else *flags |= FLAG_VIDEO;

    // Channel 9
    if (num_channels < 9 || Channels[CHANNEL9] <= 0) *flags &= ~FLAG_HEADLESS;
    else *flags |= FLAG_HEADLESS;

    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_JXD506) {
        // Channel 11
        if (num_channels < 11 || Channels[CHANNEL11] <= 0) *flags &= ~FLAG_EMERGENCY;
        else *flags |= FLAG_EMERGENCY;
    }
    else {
        // Channel 10
        if (num_channels < 10 || Channels[CHANNEL10] <= 0) *flags &= ~FLAG_MAG_CAL_X;
        else *flags |= FLAG_MAG_CAL_X;

        // Channel 11
        if (num_channels < 11 || Channels[CHANNEL11] <= 0) *flags &= ~FLAG_MAG_CAL_Y;
        else *flags |= FLAG_MAG_CAL_Y;
    }

    // Print channels every second or so
    if ((packet_counter & 0xFF) == 1) {
        printf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        printf("Aileron %d, elevator %d, throttle %d, rudder %d, led_blink_count %d\n",
               (s16) *aileron, (s16) *elevator, (s16) *throttle, (s16) *rudder,
               (s16) *led_blink_count);
    }
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    if (bind) {
        flags     = FLAG_BIND;
        packet[0] = 0;
        packet[1] = 0;
        packet[2] = 0;
        packet[3] = 0;
        packet[4] = 0;
        packet[5] = 0;
        packet[6] = 0;
    } else {
        // regular packet
        read_controls(&throttle, &rudder, &elevator, &aileron, &flags, &led_blink_count);

        packet[0] = throttle;
        packet[1] = rudder;
        packet[2] = elevator;
        packet[3] = aileron;
        // Trims, middle is 0x40
        packet[4] = 0x40; // yaw
        packet[5] = 0x40; // pitch
        packet[6] = 0x40; // roll

    }
    // TX id
    packet[7] = tx_id[0];
    packet[8] = tx_id[1];
    packet[9] = tx_id[2];
    // empty
    packet[10] = flags >> 8;
    packet[11] = 0x00;
    packet[12] = 0x00;
    packet[13] = 0x00;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_JXD506) {
        packet[11] = GET_FLAG(CHANNEL10, FLAG_START_STOP);
        if(Model.num_channels >= 12) {
            if(Channels[CHANNEL12] <= CHAN_MIN_VALUE / 2)
                packet[11] |= FLAG_CAMERA_DN;
            else if(Channels[CHANNEL12] >= CHAN_MAX_VALUE / 2)
                packet[11] |= FLAG_CAMERA_UP;
        }
        packet[12] = 0x40; // ?
        packet[13] = 0x40; // ?
    }
    //
    packet[14] = flags & 0xff;
    add_pkt_checksum();

    packet_sent = 0;
    // Each packet is repeated twice on the same
    // channel, hence >> 1
    // We're not strictly repeating them, rather we
    // send new packet on the same frequency, so the
    // receiver gets the freshest command. As receiver
    // hops to a new frequency as soon as valid packet
    // received it does not matter that the packet is
    // not the same one repeated twice - nobody checks this
    u8 rf_ch = rf_channels[rf_ch_num >> 1];
    rf_ch_num = (rf_ch_num + 1) & 0x1F;
    //  Serial.print(rf_ch); Serial.write("\n");
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, sizeof(packet));
    ++packet_counter;
    packet_sent = 1;
//    radio.ce(HIGH);
//    delayMicroseconds(15);
    // It saves power to turn off radio after the transmission,
    // so as long as we have pins to do so, it is wise to turn
    // it back.
//    radio.ce(LOW);

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (! rf_ch_num && tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static u16 v202_callback()
{
    switch (phase) {
    case V202_INIT2:
        V202_init2();
//        phase = V202_BIND1;
        phase = V202_BIND2;
        return 150;
        break;
    case V202_INIT2_NO_BIND:
        V202_init2();
        phase = V202_DATA;
        return 150;
        break;
    case V202_BIND1:
        send_packet(1);
        if (throttle >= 240) phase = V202_BIND2;
        break;
    case V202_BIND2:
        if (packet_sent && packet_ack() != PKT_ACKED) {
            printf("Packet not sent yet\n");
            return PACKET_CHKTIME;
        }
        send_packet(1);
//        if (throttle == 0) {
        if (--counter == 0) {
            phase = V202_DATA;
            counter = led_blink_count;
            flags = 0;
            PROTOCOL_SetBindState(0);
        }
        break;
    case V202_DATA:
        if (led_blink_count > BLINK_COUNT_MAX) {
            flags |= FLAG_LED;
        } else if (led_blink_count < BLINK_COUNT_MIN) {
            flags &= ~FLAG_LED;
        } else if (--counter == 0) {
            counter = led_blink_count;
            flags ^= FLAG_LED;
        }
        if (packet_sent && packet_ack() != PKT_ACKED) {
            printf("Packet not sent yet\n");
            return PACKET_CHKTIME;
        }
        send_packet(0);
        break;
    }
    // Packet every 4ms
    return PACKET_PERIOD;
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id()
{
    u32 lfsr = 0xb2c54a2ful;

#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    printf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        printf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    printf("\r\n");
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    set_tx_id(lfsr);
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_counter = 0;
    led_blink_count = BLINK_COUNT_MAX;
    v202_init();
    phase = bind ? V202_INIT2 : V202_INIT2_NO_BIND;
    if (bind) {
        counter = BIND_COUNT;
        PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000); //msec
    } else {
        counter = BLINK_COUNT;
    }

    initialize_tx_id();

    CLOCK_StartTimer(50000, v202_callback);
}

const void *V202_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:
            initialize(Model.proto_opts[PROTOOPTS_STARTBIND] == STARTBIND_YES);
            return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 12L; // T, R, E, A, LED (on/off/blink), Auto flip, camera, video, headless, X-Y calibration
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return v202_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
