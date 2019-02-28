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

// Uncomment define below to enable packet loss telemetry.
//#define YD717_TELEMETRY

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "telemetry.h"

#ifdef PROTO_HAS_NRF24L01

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 5
#define dbgprintf printf
#else
#define BIND_COUNT 60
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#ifndef EMULATOR
#define PACKET_PERIOD   8000     // Timeout for callback in uSec, 8ms=8000us for YD717
#define INITIAL_WAIT   50000     // Initial wait before starting callbacks
#else
#define PACKET_PERIOD   1000     // Adjust timeouts for reasonable emulator printouts
#define INITIAL_WAIT     500
#endif
#define PACKET_CHKTIME   500     // Time to wait if packet not yet acknowledged or timed out    

// Stock tx fixed frequency is 0x3C. Receiver only binds on this freq.
#define RF_CHANNEL 0x3C

#define FLAG_FLIP     0x0F
#define FLAG_LIGHT    0x80
#define FLAG_PICTURE  0x40
#define FLAG_VIDEO    0x20
#define FLAG_HEADLESS 0x10

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
    CHANNEL10
};

#define PAYLOADSIZE 8       // receive data pipes set to this size, but unused
#define MAX_PACKET_SIZE 9   // YD717 packets have 8-byte payload, Syma X4 is 9

static u8 packet[MAX_PACKET_SIZE];
static u16 counter;
static u32 packet_counter;
static u8 tx_power;
static u8 throttle, rudder, elevator, aileron, flags;
static u8 rudder_trim, elevator_trim, aileron_trim;
static u8 rx_tx_addr[5];

static u8 phase;
enum {
    YD717_INIT1 = 0,
    YD717_BIND2,
    YD717_BIND3,
    YD717_DATA
};

static const char * const yd717_opts[] = {
  _tr_noop("Format"),  "YD717", "Sky Wlkr", "XinXun", "Ni Hui", "SymaX4", NULL,
  _tr_noop("USE A.ACK"),  _tr_noop("Yes"), _tr_noop("No"), NULL,
#ifdef YD717_TELEMETRY
  _tr_noop("Telemetry"),  _tr_noop("Off"), _tr_noop("On"), NULL,
#endif
  NULL
};
enum {
    PROTOOPTS_FORMAT = 0,
    PROTOOPTS_SKIPAACK,
#ifdef YD717_TELEMETRY
    PROTOOPTS_TELEMETRY,
#endif
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define FORMAT_YD717   0
#define FORMAT_SKYWLKR 1
#define FORMAT_XINXUN  2
#define FORMAT_NI_HUI  3
#define FORMAT_SYMAX2  4

#define AUTO_ACK_ENABLE   0
#define AUTO_ACK_DISABLE  1

#ifdef YD717_TELEMETRY
#define TELEM_OFF 0
#define TELEM_ON 1
#endif

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
    if (Model.proto_opts[PROTOOPTS_SKIPAACK] == AUTO_ACK_DISABLE)
        return PKT_ACKED;

    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
    case BV(NRF24L01_07_TX_DS):
        return PKT_ACKED;
    case BV(NRF24L01_07_MAX_RT):
        return PKT_TIMEOUT;
    }
#ifndef EMULATOR
    return PKT_PENDING;
#else
    return PKT_ACKED;
#endif
}


static u8 convert_channel(u8 num)
{
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }

    return (u8) (((ch * 0xFF / CHAN_MAX_VALUE) + 0x100) >> 1);
}


static void read_controls(u8* throttle, u8* rudder, u8* elevator, u8* aileron,
                          u8* flags, u8* rudder_trim, u8* elevator_trim, u8* aileron_trim)
{
    *throttle = convert_channel(CHANNEL3);

    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_XINXUN) {
      *rudder = convert_channel(CHANNEL4);
      *rudder_trim = (0xff - *rudder) >> 1;
    } else {
      *rudder = 0xff - convert_channel(CHANNEL4);
      *rudder_trim = *rudder >> 1;
    }

    *elevator = convert_channel(CHANNEL2);
    *elevator_trim = *elevator >> 1;

    *aileron = 0xff - convert_channel(CHANNEL1);
    *aileron_trim = *aileron >> 1;

    if (Channels[CHANNEL5] <= 0)
      *flags &= ~FLAG_FLIP;
    else
      *flags |= FLAG_FLIP;

    if (Channels[CHANNEL6] <= 0)
      *flags &= ~FLAG_LIGHT;
    else
      *flags |= FLAG_LIGHT;

    if (Channels[CHANNEL7] <= 0)
      *flags &= ~FLAG_PICTURE;
    else
      *flags |= FLAG_PICTURE;

    if (Channels[CHANNEL8] <= 0)
      *flags &= ~FLAG_VIDEO;
    else
      *flags |= FLAG_VIDEO;

    if (Channels[CHANNEL9] <= 0)
      *flags &= ~FLAG_HEADLESS;
    else
      *flags |= FLAG_HEADLESS;

    dbgprintf("ail %3d+%3d, ele %3d+%3d, thr %3d, rud %3d+%3d, flags 0x%02x\n",
            *aileron, *aileron_trim, *elevator, *elevator_trim, *throttle,
            *rudder, *rudder_trim, *flags);
}


static void send_packet(u8 bind)
{
    if (bind) {
        packet[0]= rx_tx_addr[0]; // send data phase address in first 4 bytes
        packet[1]= rx_tx_addr[1];
        packet[2]= rx_tx_addr[2];
        packet[3]= rx_tx_addr[3];
        packet[4] = 0x56;
        packet[5] = 0xAA;
        packet[6] = Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_NI_HUI ? 0x00 : 0x32;
        packet[7] = 0x00;
    } else {
        read_controls(&throttle, &rudder, &elevator, &aileron, &flags, &rudder_trim, &elevator_trim, &aileron_trim);
        packet[0] = throttle;
        packet[1] = rudder;
        packet[3] = elevator;
        packet[4] = aileron;
        if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_YD717) {
            packet[2] = elevator_trim;
            packet[5] = aileron_trim;
            packet[6] = rudder_trim;
        } else {
            packet[2] = rudder_trim;
            packet[5] = elevator_trim;
            packet[6] = aileron_trim;
        }
        packet[7] = flags;
    }


    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();

    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_YD717) {
        NRF24L01_WritePayload(packet, 8);
    } else {
        packet[8] = packet[0];  // checksum
        for(u8 i=1; i < 8; i++) packet[8] += packet[i];
        packet[8] = ~packet[8];

        NRF24L01_WritePayload(packet, 9);
    }

    ++packet_counter;

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
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void yd717_init()
{
    NRF24L01_Initialize();

    // CRC, radio on
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_PWR_UP)); 
    
    if (Model.proto_opts[PROTOOPTS_SKIPAACK] == AUTO_ACK_DISABLE)
        NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No- Auto Acknoledgement
    else    
        NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);      // Auto Acknoledgement on all data pipes
    
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x1A); // 500uS retransmit t/o, 10 tries
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL);      // Channel 3C
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
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
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);       // Enable dynamic payload length on all pipes

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);       // Enable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Set feature bits on


    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}


static void YD717_init1()
{
    // for bind packets set address to prearranged value known to receiver
    u8 bind_rx_tx_addr[] = {0x65, 0x65, 0x65, 0x65, 0x65};
    if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_SYMAX2)
        for(u8 i=0; i < 5; i++) bind_rx_tx_addr[i]  = 0x60;
    else if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_NI_HUI)
        for(u8 i=0; i < 5; i++) bind_rx_tx_addr[i]  = 0x64;

    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bind_rx_tx_addr, 5);
}


static void YD717_init2()
{
    // set rx/tx address for data phase
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}


#ifdef YD717_TELEMETRY
static void update_telemetry() {
  static u8 frameloss = 0;

  frameloss += NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX) >> 4;
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL);   // reset packet loss counter

  Telemetry.p.dsm.flog.frameloss = frameloss;
  TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);
}
#endif

static u16 yd717_callback()
{
    switch (phase) {
    case YD717_INIT1:
        if (Model.proto_opts[PROTOOPTS_SKIPAACK] == AUTO_ACK_ENABLE) {
          send_packet(0);      // receiver doesn't re-enter bind mode if connection lost...check if already bound
          phase = YD717_BIND3;
        } else {
          YD717_init1();       // can't tell if bound when auto-ack disabled
          counter = BIND_COUNT;
          phase = YD717_BIND2;
          send_packet(1);
        }
        break;

    case YD717_BIND2:
        if (counter == 0) {
            if (packet_ack() == PKT_PENDING)
                return PACKET_CHKTIME;             // packet send not yet complete
            YD717_init2();                         // change to data phase rx/tx address
            send_packet(0);
            phase = YD717_BIND3;
        } else {
            if (packet_ack() == PKT_PENDING)
                return PACKET_CHKTIME;             // packet send not yet complete
            send_packet(1);
            counter -= 1;
        }
        break;

    case YD717_BIND3:
        switch (packet_ack()) {
        case PKT_PENDING:
            return PACKET_CHKTIME;                 // packet send not yet complete
        case PKT_ACKED:
            phase = YD717_DATA;
            PROTOCOL_SetBindState(0);
            break;
        case PKT_TIMEOUT:
            YD717_init1();                         // change to bind rx/tx address
            counter = BIND_COUNT;
            phase = YD717_BIND2;
            send_packet(1);
        }
        break;

    case YD717_DATA:
#ifdef YD717_TELEMETRY
        update_telemetry();
#endif
        if (packet_ack() == PKT_PENDING)
            return PACKET_CHKTIME;                 // packet send not yet complete
#if 0  // unimplemented channel hopping for Ni Hui quad
        else if (packet_ack() == PKT_TIMEOUT && Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_NI_HUI) {
            // Sequence (after default channel 0x3C) is channels 0x02, 0x21 (at least for TX Addr is 87 04 14 00)
        }
#endif
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;                          // Packet every 8ms
}


static void set_rx_tx_addr(u32 id)
{
    rx_tx_addr[0] = (id >> 24) & 0xFF;
    rx_tx_addr[1] = (id >> 16) & 0xFF;
    rx_tx_addr[2] = (id >>  8) & 0xFF;
    rx_tx_addr[3] = (id >>  0) & 0xFF;
    rx_tx_addr[4] = 0xC1; // always uses first data port
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_rx_tx_addr()
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

    set_rx_tx_addr(lfsr);
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    initialize_rx_tx_addr();
    packet_counter = 0;
    flags = 0;

    yd717_init();
    phase = YD717_INIT1;

    PROTOCOL_SetBindState(0xFFFFFFFF);
    CLOCK_StartTimer(INITIAL_WAIT, yd717_callback);
}

uintptr_t YD717_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND: initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 9;
        case PROTOCMD_DEFAULT_NUMCHAN: return 5;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)yd717_opts;
#ifdef YD717_TELEMETRY
        case PROTOCMD_TELEMETRYSTATE:
            return (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
        case PROTOCMD_TELEMETRYTYPE: 
            return TELEM_DSM;
#else
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
#endif
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
