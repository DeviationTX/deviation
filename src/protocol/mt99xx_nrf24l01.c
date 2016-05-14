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
  #define MT99XX_Cmds PROTO_Cmds
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
#define dbgprintf printf
#else
#define BIND_COUNT 928
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD_MT 2625 // Timeout for callback in uSec
#define PACKET_PERIOD_YZ 3125 // Yi Zhan i6S
#define INITIAL_WAIT     500
#define PACKET_SIZE 9

static const u8 data_freq[] = {
    0x02, 0x48, 0x0C, 0x3e, 0x16, 0x34, 0x20, 0x2A,
    0x2A, 0x20, 0x34, 0x16, 0x3e, 0x0c, 0x48, 0x02
};

static const u8 h7_mys_byte[] = {
    0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, 0x14, 
    0x05, 0x15, 0x06, 0x16, 0x07, 0x17, 0x00, 0x10
};

static const u8 ls_mys_byte[] = {
    0x05, 0x15, 0x25, 0x06, 0x16, 0x26,
    0x07, 0x17, 0x27, 0x00, 0x10, 0x20,
    0x01, 0x11, 0x21, 0x02, 0x12, 0x22,
    0x03, 0x13, 0x23, 0x04, 0x14, 0x24
};

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Light
    CHANNEL6,     // Flip flag
    CHANNEL7,     // Snapshot
    CHANNEL8,     // Video
    CHANNEL9,     // Headless
};

#define CHANNEL_LIGHT    CHANNEL5
#define CHANNEL_FLIP     CHANNEL6
#define CHANNEL_SNAPSHOT CHANNEL7
#define CHANNEL_VIDEO    CHANNEL8
#define CHANNEL_HEADLESS CHANNEL9

enum{
    // flags going to packet[6] (MT99xx, H7)
    FLAG_MT_RATE1   = 0x01, // (H7 high rate)
    FLAG_MT_RATE2   = 0x02, // (MT9916 only)
    FLAG_MT_VIDEO   = 0x10, // HeadLess on LS114
    FLAG_MT_SNAPSHOT= 0x20,
    FLAG_MT_FLIP    = 0x80,
};

enum {
    MT99XX_INIT = 0,
    MT99XX_BIND,
    MT99XX_DATA
};

static const char * const mt99xx_opts[] = {
    _tr_noop("Format"), "MT9916", "H7", "YZ i6S", "LS114", NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum {
    PROTOOPTS_FORMAT_MT99,
    PROTOOPTS_FORMAT_H7,
    PROTOOPTS_FORMAT_YZ,
    PROTOOPTS_FORMAT_LS,
};

static u8 packet[PACKET_SIZE];
static u16 bind_counter;
static u8 tx_power;
static u8 txid[2];
static u8 checksum_offset; 
static u8 channel_offset;
static u8 rf_chan; 
static u8 rx_tx_addr[5];
static u8 state;
static u32 packet_period;

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static s16 scale_channel(u8 ch, s16 destMin, s16 destMax)
{
    s32 chanval = Channels[ch];
    s32 range = destMax - destMin;

    if      (chanval < CHAN_MIN_VALUE) chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE) chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

static u8 calcChecksum() {
    u8 result=checksum_offset;
    for(uint8_t i=0; i<8; i++)
        result += packet[i];
    return result & 0xFF;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

static void mt99xx_send_packet()
{
    const u8 yz_p4_seq[3] = {0xa0, 0x20, 0x60};
    static u8 packet_count=0;
    static u8 yz_seq_num=0;
    static u8 ls_counter=0;
    
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
        case PROTOOPTS_FORMAT_MT99:
        case PROTOOPTS_FORMAT_H7:
        case PROTOOPTS_FORMAT_LS:
            packet[0] = scale_channel(CHANNEL3, 0xe1, 0x00); // throttle
            packet[1] = scale_channel(CHANNEL4, 0x00, 0xe1); // rudder
            packet[2] = scale_channel(CHANNEL1, 0xe1, 0x00); // aileron
            packet[3] = scale_channel(CHANNEL2, 0x00, 0xe1); // elevator
            packet[4] = 0x20; // neutral pitch trim (0x3f-0x20-0x00)
            packet[5] = 0x20; // neutral roll trim (0x00-0x20-0x3f)
            packet[6] = GET_FLAG( CHANNEL_FLIP, FLAG_MT_FLIP);
            packet[7] = h7_mys_byte[rf_chan]; // next rf channel index ?
            
            if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_H7) {
                packet[6] |= FLAG_MT_RATE1; // max rate on H7
            }
            else if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_MT99) {
                packet[6] |= 0x40 | FLAG_MT_RATE2
                          | GET_FLAG( CHANNEL_SNAPSHOT, FLAG_MT_SNAPSHOT)
                          | GET_FLAG( CHANNEL_VIDEO, FLAG_MT_VIDEO); // max rate on MT99xx
            }
            else if (Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_LS) {
                packet[6] |= FLAG_MT_RATE2; // max rate
                packet[6] |= GET_FLAG( CHANNEL_HEADLESS, 0x10);
                packet[7] = ls_mys_byte[ls_counter++];
                if(ls_counter >= sizeof(ls_mys_byte))
                    ls_counter = 0;
            }
           
            packet[8] = calcChecksum();
            break;
        case PROTOOPTS_FORMAT_YZ:
            packet[0] = scale_channel(CHANNEL3, 0, 0x64); // throttle
            packet[1] = scale_channel(CHANNEL4, 0x64, 0); // rudder
            packet[2] = scale_channel(CHANNEL2, 0, 0x64); // elevator
            packet[3] = scale_channel(CHANNEL1, 0x64, 0); // aileron
            if(packet_count++ >= 23) {
                yz_seq_num ++;
                if(yz_seq_num > 2)
                    yz_seq_num = 0;
                packet_count=0;
            }
            packet[4]= yz_p4_seq[yz_seq_num]; 
            
            packet[5]= 0x02 // expert ? (0=unarmed, 1=normal)
                     | GET_FLAG(CHANNEL_VIDEO, 0x10)
                     | GET_FLAG(CHANNEL_FLIP,  0x80)
                     | GET_FLAG(CHANNEL_HEADLESS, 0x04)
                     | GET_FLAG(CHANNEL_SNAPSHOT, 0x20);
            packet[6] = GET_FLAG(CHANNEL_LIGHT, 0x80);
            packet[7] = packet[0];            
            for(u8 idx = 1; idx < PACKET_SIZE-2; idx++)
                packet[7] += packet[idx];
            packet[8] = 0xff;
            break;
    }
    
    if (Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_LS) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x2d); // LS always transmits on the same channel
    }
    else {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, data_freq[rf_chan] + channel_offset);
    }
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, PACKET_SIZE);
    
    rf_chan++;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_YZ) {
        rf_chan++; // skip every other channel
    }
        
    if(rf_chan > 15) {
        rf_chan = 0;
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
}

static void mt99xx_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_FlushTx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);  // 5 bytes address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);// no auto retransmit
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_YZ)
        NRF24L01_SetBitrate(NRF24L01_BR_250K);     // 250Kbps (nRF24L01+ only)
    else
        NRF24L01_SetBitrate(NRF24L01_BR_1M);          // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    // this sequence necessary for module from stock tx
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on

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
    // Power on, TX mode, 2byte CRC
    u16 rf_config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP);
    if( Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_YZ)
        rf_config |= BV(XN297_UNSCRAMBLED);
    XN297_Configure(rf_config);
    // set tx address for bind packets
    // it is important to set address 
    // *after* XN297_Configure() as the i6S is using unscrambled mode
    for(u8 i=0; i<5; i++)
        rx_tx_addr[i] = 0xCC;
    XN297_SetTXAddr(rx_tx_addr, 5);
}

static void initialize_txid()
{
    u32 temp;
    if (Model.fixed_id) {
        temp = Crc(&Model.fixed_id, sizeof(Model.fixed_id));
    } else {
        temp = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) ;
    }
    txid[0] = (temp >> 8) & 0xff;
    txid[1] = (temp >> 0) & 0xff;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_YZ) {
        txid[0] = 0x53; // test (SB id)
        txid[1] = 0x00;
    }
    checksum_offset = (Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_LS) ? 0xcc : 0;
    checksum_offset += txid[0];
    checksum_offset += txid[1];
    checksum_offset &= 0xff;
    channel_offset = (((checksum_offset & 0xf0)>>4) + (checksum_offset & 0x0f)) % 8;
}

MODULE_CALLTYPE
static u16 mt99xx_callback()
{
    switch (state) {
    case MT99XX_INIT:
        MUSIC_Play(MUSIC_TELEMALARM1);
        state = MT99XX_BIND;
        break;

    case MT99XX_BIND:
        if (bind_counter == 0) {
            if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_LS) {
                rx_tx_addr[1] = txid[0];
                rx_tx_addr[2] = txid[1];
            }
            else {
                rx_tx_addr[0] = txid[0];
                rx_tx_addr[1] = txid[1];
                rx_tx_addr[2] = 0x00;
            }
            // set tx address for data packets
            XN297_SetTXAddr(rx_tx_addr, 5);
            state = MT99XX_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_LS) {
                NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x2d);
            }
            else {
                NRF24L01_WriteReg(NRF24L01_05_RF_CH, data_freq[rf_chan]);
            }
            NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
            NRF24L01_FlushTx();
            XN297_WritePayload(packet, PACKET_SIZE); // bind packet
            rf_chan++;
            if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_YZ)
                rf_chan++; // skip every other channel
            if(rf_chan > 15)
                rf_chan = 0;
            bind_counter -= 1;
        }
        break;

    case MT99XX_DATA:
        mt99xx_send_packet();
        break;
    }
    return packet_period;
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    bind_counter = BIND_COUNT;
    initialize_txid();
    mt99xx_init();
    state = MT99XX_INIT;
    // bind packet
    switch(Model.proto_opts[PROTOOPTS_FORMAT]) {
        case PROTOOPTS_FORMAT_MT99:
        case PROTOOPTS_FORMAT_H7:
            packet_period = PACKET_PERIOD_MT;
            packet[0] = 0x20;
            packet[1] = 0x14;
            packet[2] = 0x03;
            packet[3] = 0x25;
            break;
        case PROTOOPTS_FORMAT_YZ:
            packet_period = PACKET_PERIOD_YZ;
            packet[0] = 0x20;
            packet[1] = 0x15;
            packet[2] = 0x05;
            packet[3] = 0x06;
            break;
        case PROTOOPTS_FORMAT_LS:
            packet_period = PACKET_PERIOD_MT;
            packet[0] = 0x20;
            packet[1] = 0x14;
            packet[2] = 0x05;
            packet[3] = 0x11;
    }
    
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_LS) {
        packet[4] = 0xcc;
        packet[5] = txid[0];
        packet[6] = txid[1];
    }
    else {
        packet[4] = txid[0]; // 1th byte for data state tx address  
        packet[5] = txid[1]; // 2th byte for data state tx address (always 0x00 on Yi Zhan ?)
        packet[6] = 0x00; // 3th byte for data state tx address (always 0x00 ?)
    }
    packet[7] = checksum_offset; // checksum offset
    packet[8] = 0xAA; // fixed
    
    PROTOCOL_SetBindState(BIND_COUNT * packet_period / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, mt99xx_callback);
}

const void *MT99XX_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 9L; // A, E, T, R, light, flip, snapshot, video, headless
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)9L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return mt99xx_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif