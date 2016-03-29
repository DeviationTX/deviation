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
  #define H8_3D_Cmds PROTO_Cmds
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
#define BIND_COUNT 10
#define PACKET_PERIOD    450
#define dbgprintf printf
#else
#define BIND_COUNT       1000
#define PACKET_PERIOD    1800 // Timeout for callback in uSec
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT       500
#define PACKET_SIZE        20
#define RF_NUM_CHANNELS    4
#define ADDRESS_LENGTH     5

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // LED Light
    CHANNEL6,     // Flip
    CHANNEL7,     // 
    CHANNEL8,     // 
    CHANNEL9,     // RTH + Headless (H8 3D), Headless (H20)
    CHANNEL10,    // 180/360 flip mode (H8 3D), RTH (H20)
};

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_HEADLESS    CHANNEL9  // RTH + Headless on H8 3D
#define CHANNEL_RTH         CHANNEL10 // 180/360 flip mode on H8 3D

enum {
    H8_3D_INIT1 = 0,
    H8_3D_BIND2,
    H8_3D_DATA
};

enum {
    // flags going to packet[17]
    FLAG_FLIP     = 0x01,
    FLAG_RATE_MID = 0x02,
    FLAG_RATE_HIGH= 0x04,
    FLAG_LED      = 0x08,
    FLAG_HEADLESS = 0x10, // RTH + headless on H8, headless on JJRC H20
    FLAG_RTH      = 0x20, // 360° flip mode on H8 3D, RTH on JJRC H20
};

enum {
    // flags going to packet[18]
    FLAG_CALIBRATE= 0x20, // accelerometer calibration
};

static u16 counter;
static u8 phase;
static u8 packet[PACKET_SIZE];
static u8 tx_power;
static u8 txid[4];
static u8 rf_chan; 
static u8 rf_channels[RF_NUM_CHANNELS]; 
static const u8 rx_tx_addr[ADDRESS_LENGTH] = { 0xc4, 0x57, 0x09, 0x65, 0x21};


// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 sum = packet[9];
    for (int i=10; i < PACKET_SIZE-1; i++) sum += packet[i];
    return sum;
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static s16 scale_channel(u8 ch)
{
    s32 chanval = Channels[ch];
    s16 destMin=0, destMax=0;
    switch(ch) {
        case CHANNEL1:
        case CHANNEL2:
            destMin = 0x43;
            destMax = 0xbb;
            break;
        case CHANNEL3:
            destMin = 0;
            destMax = 0xff;
            break;
        case CHANNEL4:
            if(chanval > CHAN_MAX_VALUE / 60) {
                destMin = 0x44;
                destMax = 0xbc;
            }
            else if(chanval < CHAN_MIN_VALUE / 60) {
                destMin = 0x3c;
                destMax = -0x3c;
            }
            else {
                return 0;
            }
            break;
    }
    
    s32 range = destMax - destMin;

    if      (chanval < CHAN_MIN_VALUE) chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE) chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    packet[0] = 0x13;
    packet[1] = txid[0];
    packet[2] = txid[1];
    packet[3] = txid[2];
    packet[4] = txid[3];
    packet[8] = (txid[0]+txid[1]+txid[2]+txid[3]) & 0xff; // txid checksum
    memset(&packet[9], 0, 10);
    if (bind) {    
        packet[5] = 0x00;
        packet[6] = 0x00;
        packet[7] = 0x01;
    } else {
        packet[5] = rf_chan;
        packet[6] = 0x08;
        packet[7] = 0x03;
        packet[9] = scale_channel( CHANNEL3); // throttle
        packet[10] = scale_channel( CHANNEL4); // rudder
        packet[11]= scale_channel( CHANNEL2); // elevator
        packet[12]= scale_channel( CHANNEL1); // aileron
        // neutral trims
        packet[13] = 0x20;
        packet[14] = 0x20;
        packet[15] = 0x20;
        packet[16] = 0x20;
        packet[17] = FLAG_RATE_HIGH
                   | GET_FLAG( CHANNEL_LED, FLAG_LED)
                   | GET_FLAG( CHANNEL_FLIP, FLAG_FLIP)
                   | GET_FLAG( CHANNEL_HEADLESS, FLAG_HEADLESS)
                   | GET_FLAG( CHANNEL_RTH, FLAG_RTH); // 180/360 flip mode on H8 3D
    
        // both sticks bottom left: calibrate acc
        if(packet[9] <= 0x05 && packet[10] >= 0xa7 && packet[11] <= 0x57 && packet[12] >= 0xa7)
            packet[18] |= FLAG_CALIBRATE;
    }
    packet[19] = checksum(); // data checksum
    
    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? rf_channels[0] : rf_channels[rf_chan++]);
        rf_chan %= sizeof(rf_channels);
    
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
}

static void h8_3d_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    XN297_SetTXAddr(rx_tx_addr, ADDRESS_LENGTH);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable RX pipe 1 (unused)
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5 byte address width
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
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

MODULE_CALLTYPE
static u16 h8_3d_callback()
{
    switch (phase) {
    case H8_3D_INIT1:
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = H8_3D_BIND2;
        break;

    case H8_3D_BIND2:
        if (counter == 0) {
            phase = H8_3D_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case H8_3D_DATA:
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

    // tx id
    txid[0] = 0xa0 + (((lfsr >> 24) & 0xFF) % 0x10);
    txid[1] = 0xb0 + (((lfsr >> 16) & 0xFF) % 0x20);
    txid[2] = ((lfsr >> 8) & 0xFF) % 0x20;
    txid[3] = (lfsr & 0xFF) % 0x11;
    
    // rf channels
    rf_channels[0] = 0x06 + (((txid[0]>>8) + (txid[0]&0x0f)) % 0x0f);
    rf_channels[1] = 0x15 + (((txid[1]>>8) + (txid[1]&0x0f)) % 0x0f);
    rf_channels[2] = 0x24 + (((txid[2]>>8) + (txid[2]&0x0f)) % 0x0f);
    rf_channels[3] = 0x33 + (((txid[3]>>8) + (txid[3]&0x0f)) % 0x0f);
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    
    counter = BIND_COUNT;
    initialize_txid();
    h8_3d_init();
    phase = H8_3D_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, h8_3d_callback);
}

const void *H8_3D_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 10L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)10L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
