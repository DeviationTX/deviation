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
  #define HonTai_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

#ifdef MODULAR
  //Some versions of gcc apply this to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 20
#define PACKET_PERIOD    150
#define dbgprintf printf
#else
#define BIND_COUNT 80
#define PACKET_PERIOD    15000 // Timeout for callback in uSec
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT     500
#define BIND_PACKET_SIZE 10
#define PACKET_SIZE      12
#define RF_BIND_CHANNEL  0

enum {
    LAST_PROTO_OPT,
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

static u8 packet[PACKET_SIZE];
static u16 counter;
static u8 tx_power;
static u8 txid[5];
static u8 rf_chan = 0; 
static u8 rf_channels[] = {0x05, 0x19, 0x28}; 
static const u8 rx_tx_addr[] = {0xd2, 0xb5, 0x99, 0xb3, 0x4a};
static u8 phase;

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

static s16 scale_channel(u8 ch, s32 destMin, s32 destMax)
{
    s32 a = (destMax - destMin) * ((s32)Channels[ch] - CHAN_MIN_VALUE);
    s32 b = CHAN_MAX_VALUE - CHAN_MIN_VALUE;
    return ((a / b) - (destMax - destMin)) + destMax;
}

static void send_packet(u8 bind)
{
    if (bind) {
      memcpy(packet, txid, 5);
      memset(&packet[5], 0, 3);
    } else {
      packet[0] = 0x0b;
      packet[1] = 0x00;
      packet[2] = 0x00;
      packet[3] = scale_channel(CHANNEL3, 0x00, 0xFF);              // throttle
     // packet[4] = (0x3f - scale_channel(CHANNEL1, 0x00, 0x3F))      // aileron
      packet[4] = scale_channel(CHANNEL1, 0x3f, 0x00)      // aileron
                | (Channels[CHANNEL_RTH] > 0 ? 0x80 : 0x00)
                | (Channels[CHANNEL_HEADLESS] > 0 ? 0x40 : 0x00);
      packet[5] = (0x3f - scale_channel(CHANNEL2, 0x00, 0x3F))      // elevator
                | (Channels[CHANNEL_CALIBRATE] > 0 ? 0x80 : 0x00)
                | (Channels[CHANNEL_FLIP] > 0 ? 0x40 : 0x00);
      packet[6] = scale_channel(CHANNEL4, 0x00, 0x3F)               // rudder
                | (Channels[CHANNEL_VIDEO] > 0 ? 0x80 : 0x00);
      packet[7] = scale_channel(CHANNEL1, 32, -32);
      packet[8] = scale_channel(CHANNEL4, 32, -32);
      packet[9] = scale_channel(CHANNEL2, -32, 32);
    }
    crc16(packet, bind ? BIND_PACKET_SIZE : PACKET_SIZE);
    
    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? RF_BIND_CHANNEL : rf_channels[rf_chan++]);
    rf_chan %= sizeof(rf_channels);

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, bind ? BIND_PACKET_SIZE : PACKET_SIZE);

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
    dbgprintf("next chan 0x%02x, bind %d, data %02x", bind ? RF_BIND_CHANNEL : rf_channels[rf_chan], bind, packet[0]);
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

    XN297_SetTXAddr(rx_tx_addr, 5);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 3-byte address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);   // 1Mbps
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
    NRF24L01_Activate(0x73);

    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    dbgprintf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        dbgprintf("BK2421 detected\n");
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
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back
}

static void ht_init2()
{
    const u8 data_tx_addr[] = {0x2a, 0xda, 0xa5, 0x25, 0x24};
    XN297_SetTXAddr(data_tx_addr, 5);
}

MODULE_CALLTYPE
static u16 ht_callback()
{
    switch (phase) {
    case HonTai_INIT1:
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = HonTai_BIND2;
        break;

    case HonTai_BIND2:
        if (counter == 0) {
            ht_init2();
            phase = HonTai_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case HonTai_DATA:
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;
}

static void initialize_txid()
{
    u32 temp;
    if (Model.fixed_id) {
        temp = Crc(&Model.fixed_id, sizeof(Model.fixed_id));
    } else {
        temp = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) ;
    }
    txid[0] = 0x80 | (temp % 0x40); 
    if( txid[0] == 0xAA) // avoid using same freq for bind and data channel
        txid[0] ++;
    txid[1] = (temp >> 8) & 0xFF;

//TODO Figure out relationship of txid to rx/tx address and rf channels
    rf_chan = 0;
    txid[0] = 0x4c;
    txid[1] = 0x4b;
    txid[2] = 0x3a;
    txid[3] = 0xf2;
    txid[4] = 0x04;
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    
    counter = BIND_COUNT;
    initialize_txid();
    ht_init();
    phase = HonTai_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, ht_callback);
}

const void *HonTai_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 11L; // A, E, T, R, light, flip, photo, video, headless, RTH, calibrate
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)11L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
