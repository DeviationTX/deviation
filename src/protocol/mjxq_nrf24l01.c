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
  #define MJXq_Cmds PROTO_Cmds
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
#define BIND_COUNT 150
#define PACKET_PERIOD    4000 // Timeout for callback in uSec
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT       500
#define PACKET_SIZE        16
#define RF_NUM_CHANNELS    4
#define ADDRESS_LENGTH     5

static const char * const mjxq_opts[] = {
  _tr_noop("Format"), "WLH08", "X600", "X800", NULL,
  NULL
};
enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};
enum {
    FORMAT_WLH08 = 0,
    FORMAT_X600,
    FORMAT_X800,
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
    CHANNEL11,    // AutoFlip By Button
};
#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_PICTURE     CHANNEL7
#define CHANNEL_VIDEO       CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10
#define CHANNEL_AUTOFLIP    CHANNEL11


enum {
    MJXq_INIT1 = 0,
    MJXq_BIND1,
    MJXq_DATA
};

static u16 counter;
static u8 phase;
static u8 tx_power;
static u8 rf_chan; 
static u8 txid[3];
static u8 packet[PACKET_SIZE];
static u8 rf_channels[RF_NUM_CHANNELS]; 

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 sum = packet[0];
    for (int i=1; i < PACKET_SIZE-1; i++) sum += packet[i];
    return sum;
}

#define BABS(X) (((X) < 0) ? -(u8)(X) : (X))
// Channel values are sign + magnitude 8bit values
static u8 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE)      ch = CHAN_MIN_VALUE;
    else if (ch > CHAN_MAX_VALUE) ch = CHAN_MAX_VALUE;
    return (u8) ((ch < 0 ? 0x80 : 0) | BABS(ch * 127 / CHAN_MAX_VALUE));
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
//#define CHAN2TRIM(X) ((((X) & 0x80 ? 0xff - (X) : 0x80 + (X)) >> 1) + 0x00)
#define CHAN2TRIM(X) (((X) & 0x80 ? (X) : 0x7f - (X)) >> 1)
static void send_packet(u8 bind)
{
    packet[0] = convert_channel(CHANNEL3);          // throttle
    packet[0] = packet[0] & 0x80 ? 0xff - packet[0] : 0x80 + packet[0];
    packet[1] = convert_channel(CHANNEL4);          // rudder
    packet[4] = 0x40;         // rudder does not work well with dyntrim
    packet[2] = 0x80 ^ convert_channel(CHANNEL2);   // elevator
    packet[5] = CHAN2TRIM(packet[2]); // 0x40;      // trim elevator
    packet[3] = convert_channel(CHANNEL1);          // aileron
    packet[6] = CHAN2TRIM(packet[3]); // 0x40;      // trim aileron
    packet[7] = txid[0];
    packet[8] = txid[1];
    packet[9] = txid[2];


    packet[11] = 0;   // overwritten below for X600
    packet[12] = 0;
    packet[13] = 0;

    packet[14] = 0xc0;  // bind value
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
    case FORMAT_WLH08:
        packet[10] = GET_FLAG(CHANNEL_RTH, 0x02)
                   | GET_FLAG(CHANNEL_HEADLESS, 0x01);
        if (!bind) {
            packet[14] = 0x04
                       | GET_FLAG(CHANNEL_FLIP, 0x01)
                       | GET_FLAG(CHANNEL_PICTURE, 0x08)
                       | GET_FLAG(CHANNEL_VIDEO, 0x10)
                       | GET_FLAG(CHANNEL_LED, 0x20); // air/ground mode
        }
        break;

    case FORMAT_X600:
        packet[10] = GET_FLAG(CHANNEL_LED, 0x02);
        packet[11] = GET_FLAG(CHANNEL_RTH, 0x01);
        if (!bind) {
            packet[14] = 0x02      // always high rates by bit2 = 1
                       | GET_FLAG(CHANNEL_FLIP, 0x04)
                       | GET_FLAG(CHANNEL_AUTOFLIP, 0x10)
                       | GET_FLAG(CHANNEL_HEADLESS, 0x20);
        }
        break;

    case FORMAT_X800:
    default:
        packet[10] = 0x10
                   | GET_FLAG(CHANNEL_LED, 0x02)
                   | GET_FLAG(CHANNEL_AUTOFLIP, 0x01);
        if (!bind) {
            packet[14] = 0x02      // always high rates by bit2 = 1
                       | GET_FLAG(CHANNEL_FLIP, 0x04)
                       | GET_FLAG(CHANNEL_PICTURE, 0x08)
                       | GET_FLAG(CHANNEL_VIDEO, 0x10);    
        }
    }
    packet[15] = checksum();

    
    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[rf_chan++ / 2]);
    rf_chan %= 2 * sizeof(rf_channels);  // channels repeated

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, PACKET_SIZE);

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
    dbgprintf("next chan 0x%02x, bind %d, data %02x", rf_channels[rf_chan/2], bind, packet[0]);
    for(int i=1; i < PACKET_SIZE; i++) dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif
}

static void mjxq_init()
{
    u8 rx_tx_addr[ADDRESS_LENGTH];

    if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_WLH08) {
        memcpy(rf_channels, "\x12\x22\x32\x42", sizeof(rf_channels));
        memcpy(rx_tx_addr, "\x6d\x6a\x77\x77\x77", sizeof(rx_tx_addr));
    } else {
        memcpy(rf_channels, "\x0a\x35\x42\x3d", sizeof(rf_channels));
        memcpy(rx_tx_addr, "\x6d\x6a\x73\x73\x73", sizeof(rx_tx_addr));
    }


    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5); 
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7); 
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7); 

    XN297_SetTXAddr(rx_tx_addr, sizeof(rx_tx_addr));

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PACKET_SIZE);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
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

static void mjxq_init2()
{
    if (!Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_WLH08)
        memcpy(rf_channels, "\x0a\x46\x3a\x42", sizeof(rf_channels));
    
}


MODULE_CALLTYPE
static u16 mjxq_callback()
{
    switch (phase) {
    case MJXq_INIT1:
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = MJXq_BIND1;
        break;

    case MJXq_BIND1:
        if (counter == 0) {
            mjxq_init2();
            phase = MJXq_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case MJXq_DATA:
        send_packet(0);
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
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_WLH08) {
        // txid must be multiple of 8
        txid[0] = (lfsr >> 16) & 0xf8;
        txid[1] = (lfsr >> 8 ) & 0xff;
        txid[2] = lfsr & 0xff; 
    } else {
        txid[0] = 0xf8 + (Model.fixed_id & 0xff); //(lfsr >> 16) & 0xff;
        txid[1] = (0x4f + ((Model.fixed_id >> 8) & 0xff)); //(lfsr >> 8 ) & 0xff;
        txid[2] = (0x1c + ((Model.fixed_id >> 16) & 0xff)); //lfsr & 0xff; 
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    
    counter = BIND_COUNT;
    initialize_txid();
    mjxq_init();
    phase = MJXq_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, mjxq_callback);
}

const void *MJXq_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 11L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)11L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return mjxq_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif

