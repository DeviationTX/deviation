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
  #define YD717_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

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
#define BIND_COUNT 60
#endif

// Timeout for callback in uSec, 8ms=8000us for YD717
#define PACKET_PERIOD 8000

// Stock tx fixed frequency  TODO: Will reciever find others?
#define RF_CHANNEL 0x3C

enum {
    FLAG_FLIP   = 0x0F
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

#define PAYLOADSIZE 8

static u8 packet[PAYLOADSIZE];
static u8 packet_sent;
static u8 tx_id[3];
static u16 counter;
static u32 packet_counter;
static u8 tx_power;
//static u8 auto_flip; // Channel 6 <= 0 - disabled > 0 - enabled
static u8 throttle, rudder, elevator, aileron, flags;


//
static u8 phase;
enum {
    YD717_INIT2 = 0,
    YD717_INIT2_NO_BIND,
    YD717_BIND1,
    YD717_BIND2,
    YD717_BIND3,
    YD717_DATA
};

// static u32 bind_count;

//static u8 rf_channels[16];

// Bit vector from bit position
#define BV(bit) (1 << bit)

static void packet_ack()
{
    if (packet_sent) {
        //bool report_done = false;
        //    if  (!(radio.read_register(STATUS) & _BV(TX_DS))) { Serial.write("Waiting for radio\n"); report_done = true; }
        while (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)) )) ;
        NRF24L01_WriteReg(NRF24L01_07_STATUS, BV(NRF24L01_07_TX_DS));
        //    if (report_done) Serial.write("Done\n");
        packet_sent = 0;
    }
}

static void packet_write()
{
    packet_ack();
//  yd717 always transmits on same channel    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x7F);     // Clear any pending interrupts
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, sizeof(packet));
    ++packet_counter;
    packet_sent = 1;
}

static void yd717_init()
{
    NRF24L01_Initialize();

    // CRC, radio on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_PWR_UP)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);      // Auto Acknoledgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x1A); // 500uS retransmit t/o, 10 tries
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL);      // Channel 3C
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
//    NRF24L01_WriteReg(NRF24L01_08_OBSERVE_TX, 0x00); // no write bits in this field
//    NRF24L01_WriteReg(NRF24L01_09_CD, 0x00);         // no write bits in this field
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
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Set feature bits on


    u8 rx_tx_addr[] = {0x29, 0xC3, 0x21, 0x11, 0xC1};
//    u8 rx_p1_addr[] = {0x29, 0xC3, 0x21, 0x11, 0xC1};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
//    NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_p1_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);

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
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\x99\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xDF\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
    } else {
        printf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    // Implicit delay in callback
    // delay(50);
}

static void YD717_init2()
{
    packet_sent = 0;

    // Turn radio power on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_FlushTx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x7F);     // Clear any pending interrupts

    // Implicit delay in callback
    // delayMicroseconds(150);
}

static void YD717_init3()
{
    // set address to prearranged value known to receiver (guessing...)
    u8 rx_tx_addr[] = {0x65, 0x65, 0x65, 0x65, 0x65};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);

    packet[0]= 0x29; // send address to use in first 4 bytes
    packet[1]= 0xC3;
    packet[2]= 0x21;
    packet[3]= 0x11;
    packet[4]= 0x56; // fixed value
    packet[5]= 0xAA; // fixed value
    packet[6]= 0x32; // fixed - same as trim values in bind packets...
    packet[7]= 0x00; // fixed value
    packet_write();
}

static void YD717_init4()
{
    packet_ack();   // acknowledge packet sent by init3.  Must be complete before changing address

    // set address
    u8 rx_tx_addr[] = {0x29, 0xC3, 0x21, 0x11, 0xC1};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}

static void set_tx_id(u32 id)
{
    tx_id[0] = (id >> 16) & 0xFF;
    tx_id[1] = (id >> 8) & 0xFF;
    tx_id[2] = (id >> 0) & 0xFF;
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
                          u8* flags)
{
    // Protocol is registered AETRG, that is
    // Aileron is channel 0, Elevator - 1, Throttle - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range

    // Channel 3
    *throttle = convert_channel(CHANNEL3);

    // Channel 4
    *rudder = convert_channel(CHANNEL4);

    // Channel 2
    *elevator = convert_channel(CHANNEL2);

    // Channel 1
    *aileron = convert_channel(CHANNEL1);

    // Channel 5
    if (Channels[CHANNEL5] <= 0) *flags &= ~FLAG_FLIP;
    else *flags |= FLAG_FLIP;

    // Print channels every second or so
    if ((packet_counter & 0xFF) == 1) {
        printf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        printf("Aileron %d, elevator %d, throttle %d, rudder %d, flip enable %d\n",
               (s16) *aileron, (s16) *elevator, (s16) *throttle, (s16) *rudder,
               (s16) *flags);
    }
}

static void send_packet(u8 bind)
{
    if (bind) {
        packet[0] = 0;    // stock controller puts channel values in during bind
        packet[1] = 0x80;
        packet[3] = 0x80;
        packet[4] = 0x80;
        packet[2] = 0x32; // trims have fixed values for bind
        packet[5] = 0x32;
        packet[6] = 0x32;
        packet[7] = 0;
    } else {
        // regular packet
        read_controls(&throttle, &rudder, &elevator, &aileron, &flags);
        packet[0] = throttle;
        packet[1] = rudder;
        packet[3] = elevator;
        packet[4] = aileron;

        // Trims, middle is 0x40
        packet[2] = 0x40; // pitch
        packet[5] = 0x40; // roll
        packet[6] = 0x40; // yaw
        packet[7] = flags;
    }

    packet_write();

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


MODULE_CALLTYPE
static u16 yd717_callback()
{
    switch (phase) {
    case YD717_INIT2:
        YD717_init2();
        MUSIC_Play(MUSIC_TELEMALARM1);
//        phase = YD717_BIND1;
        phase = YD717_BIND2;
        return 150;
        break;
    case YD717_INIT2_NO_BIND:
        YD717_init2();
        phase = YD717_DATA;
        return 150;
        break;
    case YD717_BIND1:
        send_packet(1);
        if (throttle >= 240) phase = YD717_BIND2;
        break;
    case YD717_BIND2:
//        if (throttle == 0) {
        if (--counter == 0) {
            YD717_init3();    // send rx/tx address to use on fixed bind address
            phase = YD717_BIND3;
            return 150;
        } else {
            send_packet(1);
        }
        break;
    case YD717_BIND3:
        YD717_init4();  // set rx/tx address back from bind address
        flags = 0;
        phase = YD717_DATA;
        PROTOCOL_SetBindState(0);
        MUSIC_Play(MUSIC_DONE_BINDING);
        // TODO: YD717 controller puts 2 second delay here - required?
        break;
    case YD717_DATA:
        send_packet(0);
        break;
    }
    // Packet every 8ms
    return PACKET_PERIOD;
}

// Linear feedback shift register with 32-bit Xilinx polinomial x^32 + x^22 + x^2 + x + 1
static const uint32_t LFSR_FEEDBACK = 0x80200003ul;
static const uint32_t LFSR_INTAP = 32-1;

static void update_lfsr(uint32_t *lfsr, uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        *lfsr = (*lfsr >> 1) ^ ((-(*lfsr & 1u) & LFSR_FEEDBACK) ^ ~((uint32_t)(b & 1) << LFSR_INTAP));
        b >>= 1;
    }
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
        update_lfsr(&lfsr, var[i]);
    }
    printf("\r\n");
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           update_lfsr(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) update_lfsr(&lfsr, 0);

    set_tx_id(lfsr);
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_counter = 0;
    packet_sent = 0;
    yd717_init();
    phase = bind ? YD717_INIT2 : YD717_INIT2_NO_BIND;
    if (bind) {
        counter = BIND_COUNT;
        PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000); //msec
    }

    initialize_tx_id();

    CLOCK_StartTimer(50000, yd717_callback);
}

const void *YD717_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(1); return 0;
        case PROTOCMD_DEINIT: return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 5L; // T, R, E, A, enable flip
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)-1;
        case PROTOCMD_SET_TXPOWER:
            tx_power = Model.tx_power;
            NRF24L01_SetPower(tx_power);
            break;
        default: break;
    }
    return 0;
}
#endif

