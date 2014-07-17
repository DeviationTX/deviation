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
  #define KN_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"
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
#define BIND_COUNT 1000 // for KN 2sec every 2ms - 1000 packets
#endif

// Timeout for callback in uSec, 2ms=2000us for KN
#define PACKET_PERIOD 2000
#define PACKET_CHKTIME  100 // Time to wait for packet to be sent (no ACK, so very short)

#define PAYLOADSIZE 16
#define NFREQCHANNELS 4
#define TXID_SIZE 4


enum {
    FLAG_DR     = 0x01, // Dual Rate
    FLAG_TH     = 0x02, // Throttle Hold
    FLAG_IDLEUP = 0x04, // Idle up
    FLAG_RES1   = 0x08,
    FLAG_RES2   = 0x10,
    FLAG_RES3   = 0x20,
    FLAG_GYRO3  = 0x40, // 00 - 6G mode, 01 - 3G mode
    FLAG_GYROR  = 0x80  // Always 0 so far
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
    CHANNEL10
};


static u8 packet[PAYLOADSIZE];
static u8 packet_sent;
static u8 tx_id[TXID_SIZE];
static u8 rf_ch_num;
static u8 packet_count;
static u16 bind_counter;
static u32 total_packets;
static u8 tx_power;
static u16 throttle, rudder, elevator, aileron;
static u8 flags;
static u8 hopping_frequency[NFREQCHANNELS];


//
static u8 phase;
enum {
    KN_INIT2 = 0,
    KN_INIT2_NO_BIND,
    KN_BIND,
    KN_DATA
};

static const char * const kn_opts[] = {
  _tr_noop("Re-bind"),  _tr_noop("No"), _tr_noop("Yes"), NULL,
  _tr_noop("1Mbps"),  _tr_noop("No"), _tr_noop("Yes"), NULL,
  NULL
};
enum {
    PROTOOPTS_STARTBIND = 0,
    PROTOOPTS_USE1MBPS,
};
enum {
    STARTBIND_NO  = 0,
    STARTBIND_YES = 1,
};
enum {
    USE1MBPS_NO  = 0,
    USE1MBPS_YES = 1,
};



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

// 2-bytes CRC
#define CRC_CONFIG (BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO))

static u16 kn_init()
{
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 0x20);   // bytes of data payload for pipe 0


    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
    // Enable: Dynamic Payload Length, Payload with ACK , W_TX_PAYLOAD_NOACK
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, BV(NRF2401_1D_EN_DPL) | BV(NRF2401_1D_EN_ACK_PAY) | BV(NRF2401_1D_EN_DYN_ACK));

    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    printf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        printf("BK2421 detected\n");
//        long nul = 0;
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\xF9\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
//        NRF24L01_WriteRegisterMulti(0x06, (u8 *) &nul, 4);
//        NRF24L01_WriteRegisterMulti(0x07, (u8 *) &nul, 4);
//        NRF24L01_WriteRegisterMulti(0x08, (u8 *) &nul, 4);
//        NRF24L01_WriteRegisterMulti(0x09, (u8 *) &nul, 4);
//        NRF24L01_WriteRegisterMulti(0x0A, (u8 *) &nul, 4);
//        NRF24L01_WriteRegisterMulti(0x0B, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC7\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
    } else {
        printf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    // Delay 50 ms
    return 50000u;
}


static u16 kn_init2()
{
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    packet_sent = 0;
    packet_count = 0;
    rf_ch_num = 0;

    // Turn radio power on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG | BV(NRF24L01_00_PWR_UP));
    // delayMicroseconds(150);
    return 150u;
}


static void set_tx_for_bind()
{
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 83);
    NRF24L01_SetBitrate(NRF24L01_BR_1M); // 1Mbps for binding
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (u8 *) "KNDZK", 5);
}


static void set_tx_for_data()
{
    u8 tx_addr[5];
    for (int i = 0; i < TXID_SIZE; ++i)
        tx_addr[i] = tx_id[i];
    tx_addr[4] = 'K';
    if (Model.proto_opts[PROTOOPTS_USE1MBPS] == USE1MBPS_YES) {
        NRF24L01_SetBitrate(NRF24L01_BR_1M);
    } else {
        NRF24L01_SetBitrate(NRF24L01_BR_250K);
    }
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_addr, 5);
}

static void calc_fh_channels(u32 seed)
{
    int idx = 0;
    u32 rnd = seed;
    while (idx < NFREQCHANNELS) {
        int i;
        rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

        // Use least-significant byte. 73 is prime, so channels 76..77 are unused
        u8 next_ch = ((rnd >> 8) % 73) + 2;
        // Keep the distance 2 between the channels - either odd or even
        if (((next_ch ^ seed) & 0x01 )== 0)
            continue;
        // Check that it's not duplicate and spread uniformly
        for (i = 0; i < idx; i++) {
            if(hopping_frequency[i] == next_ch)
                break;
        }
        if (i != idx)
            continue;
        hopping_frequency[idx++] = next_ch;
    }
}


static void set_tx_id(u32 id)
{
    tx_id[0] = (id >> 24) & 0xFF;
    tx_id[1] = (id >> 16) & 0xFF;
    tx_id[2] = (id >> 8) & 0xFF;
    tx_id[3] = (id >> 0) & 0xFF;
    printf("TX id %02X %02X %02X %02X\n", tx_id[0], tx_id[1], tx_id[2], tx_id[3]);
    calc_fh_channels(id);
    printf("FQ ch %02X %02X %02X %02X\n", hopping_frequency[0], hopping_frequency[1],
                                          hopping_frequency[2], hopping_frequency[3]);
}


static u16 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u16) ((ch * 511 / CHAN_MAX_VALUE) + 512);
}


static void read_controls(u16* throttle, u16* aileron, u16* elevator, u16* rudder,
                          u8* flags)
{
    // Protocol is registered TAERG, that is
    // Throttle is channel 0, Aileron - 1, Elevator - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range

    *throttle = convert_channel(CHANNEL1);
    *aileron  = convert_channel(CHANNEL2);
    *elevator = convert_channel(CHANNEL3);
    *rudder   = convert_channel(CHANNEL4);

    if (Channels[CHANNEL5] <= 0) *flags &= ~FLAG_DR;
    else *flags |= FLAG_DR;

    if (Channels[CHANNEL6] <= 0) *flags &= ~FLAG_TH;
    else *flags |= FLAG_TH;

    if (Channels[CHANNEL7] <= 0) *flags &= ~FLAG_IDLEUP;
    else *flags |= FLAG_IDLEUP;

    if (Channels[CHANNEL8] <= 0) *flags &= ~FLAG_GYRO3;
    else *flags |= FLAG_GYRO3;


    // Print channels every now and then
    if (0) { //(total_packets & 0x3FF) == 1) {
        printf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        printf("Aileron %d, elevator %d, throttle %d, rudder %d\n",
               (s16) *aileron, (s16) *elevator, (s16) *throttle, (s16) *rudder);
    }
}



#define PACKET_COUNT_SHIFT 5
#define RF_CHANNEL_SHIFT 2
static void send_packet(u8 bind)
{
    u8 rf_ch;
    if (bind) {
        rf_ch = 83;
        packet[0]  = 'K';
        packet[1]  = 'N';
        packet[2]  = 'D';
        packet[3]  = 'Z';
        packet[4]  = tx_id[0];
        packet[5]  = tx_id[1];
        packet[6]  = tx_id[2];
        packet[7]  = tx_id[3];
        packet[8]  = hopping_frequency[0];
        packet[9]  = hopping_frequency[1];
        packet[10] = hopping_frequency[2];
        packet[11] = hopping_frequency[3];
        packet[12] = 0x00;
        packet[13] = 0x00;
        packet[14] = 0x00;
        packet[15] = Model.proto_opts[PROTOOPTS_USE1MBPS] == USE1MBPS_YES ? 0x01 : 0x00;
    } else {
        rf_ch = hopping_frequency[rf_ch_num];

        // Each packet is repeated 4 times on the same channel
        // We're not strictly repeating them, rather we
        // send new packet on the same frequency, so the
        // receiver gets the freshest command. As receiver
        // hops to a new frequency as soon as valid packet
        // received it does not matter that the packet is
        // not the same one repeated twice - nobody checks this

        // NB! packet_count overflow is handled and used in
        // callback.
        if (++packet_count == 4)
            rf_ch_num = (rf_ch_num + 1) & 0x03;

        read_controls(&throttle, &aileron, &elevator, &rudder, &flags);

        packet[0]  = (throttle >> 8) & 0xFF;
        packet[1]  = throttle & 0xFF;
        packet[2]  = (aileron >> 8) & 0xFF;
        packet[3]  = aileron  & 0xFF;
        packet[4]  = (elevator >> 8) & 0xFF;
        packet[5]  = elevator & 0xFF;
        packet[6]  = (rudder >> 8) & 0xFF;
        packet[7]  = rudder & 0xFF;
        // Trims, middle is 0x64 (100) 0-200
        packet[8]  = 0x64; // T
        packet[9]  = 0x64; // A
        packet[10] = 0x64; // E
        packet[11] = 0x64; // R

        packet[12] = flags;

        packet[13] = (packet_count << PACKET_COUNT_SHIFT) | (rf_ch_num << RF_CHANNEL_SHIFT);

        packet[14] = 0x00;
        packet[15] = 0x00;
    }


    packet_sent = 0;
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, sizeof(packet));
    ++total_packets;
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


MODULE_CALLTYPE
static u16 kn_callback()
{
    u16 timeout = PACKET_PERIOD;
    switch (phase) {
    case KN_INIT2:
        bind_counter = BIND_COUNT;
        timeout = kn_init2();
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = KN_BIND;
        set_tx_for_bind();
        break;
    case KN_INIT2_NO_BIND:
        timeout = kn_init2();
        phase = KN_DATA;
        set_tx_for_data();
        break;
    case KN_BIND:
        if (packet_sent && packet_ack() != PKT_ACKED)
            return PACKET_CHKTIME;
        send_packet(1);
        if (--bind_counter == 0) {
            phase = KN_DATA;
            set_tx_for_data();
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        }
        break;
    case KN_DATA:
        if (packet_count == 4)
            packet_count = 0;
        else {
            if (packet_sent && packet_ack() != PKT_ACKED) {
                printf("Packet not sent yet\n");
                return PACKET_CHKTIME;
            }
            send_packet(0);
        }
        break;
    }
    return timeout;
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
    total_packets = 0;
    u16 timeout = kn_init();
    phase = bind ? KN_INIT2 : KN_INIT2_NO_BIND;
    if (bind) {
        PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000); //msec
    }
    initialize_tx_id();

    CLOCK_StartTimer(timeout, kn_callback);
}

const void *KN_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:
            initialize(Model.proto_opts[PROTOOPTS_STARTBIND] == STARTBIND_YES);
            return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; // Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 8L; // T, R, E, A, DR, TH, IDLEUP, GYRO3
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return kn_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_SET_TXPOWER:
            tx_power = Model.tx_power;
            NRF24L01_SetPower(tx_power);
            break;
        default: break;
    }
    return 0;
}
#endif
