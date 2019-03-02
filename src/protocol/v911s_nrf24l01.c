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

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define V911S_BIND_COUNT      20
#define V911S_PACKET_PERIOD   100
#define V911S_BIND_PACKET_PERIOD 100
#define dbgprintf printf
#else
#define V911S_BIND_COUNT      1000
#define V911S_PACKET_PERIOD   5000 // Timeout for callback in uSec
#define V911S_BIND_PACKET_PERIOD    3300
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

//#define V911S_ORIGINAL_ID

#define V911S_INITIAL_WAIT          500
#define V911S_PACKET_SIZE           16
#define V911S_RF_BIND_CHANNEL       35
#define V911S_NUM_RF_CHANNELS       8

static u8 tx_power;
static u8 packet[V911S_PACKET_SIZE];
static u8 rf_ch_num;
static u8 hopping_frequency_no;
static u8 rx_tx_addr[5];
static u8 hopping_frequency[V911S_NUM_RF_CHANNELS];
static u16 bind_counter;
static u16 packet_period;
static u8 phase;

enum{
    V911S_BIND,
    V911S_DATA
};

// flags going to packet[1]
#define V911S_FLAG_EXPERT   0x04
// flags going to packet[2]
#define V911S_FLAG_CALIB    0x01

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,  
};
#define CHANNEL_CALIB   CHANNEL5

// Bit vector from bit position
#define BV(bit) (1 << bit)

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

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void V911S_send_packet(u8 bind)
{
    u8 channel=hopping_frequency_no;
    if(bind)
    {
        packet[0] = 0x42;
        packet[1] = 0x4E;
        packet[2] = 0x44;
        for(u8 i=0;i<5;i++)
            packet[i+3] = rx_tx_addr[i];
        for(u8 i=0;i<8;i++)
            packet[i+8] = hopping_frequency[i];
    }
    else
    {
        if(rf_ch_num&1)
        {
            if((hopping_frequency_no&1)==0)
                channel+=8;
            channel>>=1;
        }
        if(rf_ch_num&2)
            channel=7-channel;
        packet[ 0]=(rf_ch_num<<3)|channel;
        packet[ 1]=V911S_FLAG_EXPERT; // short press on left button
        packet[ 2]=GET_FLAG(CHANNEL_CALIB,V911S_FLAG_CALIB); // long  press on right button
        memset(packet+3, 0x00, V911S_PACKET_SIZE - 3);
        //packet[3..6]=trims TAER signed
        u16 ch=scale_channel(CHANNEL3 ,0,0x7FF);// throttle
        packet[ 7] = ch;
        packet[ 8] = ch>>8;
        ch=scale_channel(CHANNEL1 ,0x7FF,0);    // aileron
        packet[ 8]|= ch<<3;
        packet[ 9] = ch>>5;
        ch=scale_channel(CHANNEL2,0,0x7FF);     // elevator
        packet[10] = ch;
        packet[11] = ch>>8;
        ch=scale_channel(CHANNEL4,0x7FF,0);      // rudder
        packet[11]|= ch<<3;
        packet[12] = ch>>5;
    }
    
    // Power on, TX mode, 2byte CRC
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (!bind)
    {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[channel]);
        hopping_frequency_no++;
        hopping_frequency_no&=7;    // 8 RF channels
    }
    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, V911S_PACKET_SIZE);

    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void V911S_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((u8 *)"\x4B\x4E\x42\x4E\x44", 5);          // Bind address
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, V911S_RF_BIND_CHANNEL);    // Bind channel
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);    // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);     // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01); // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_250K);          // 250Kbps
    NRF24L01_SetPower(tx_power);
}

static void V911S_initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    u8 i,j;
    
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif

    if (Model.fixed_id) {
       for (i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    
    for(i=0, j=0; i<4; i++, j+=8)
        rx_tx_addr[i] = (lfsr >> j) & 0xff;
    
    rand32_r(&lfsr, 0);
    rx_tx_addr[4] = lfsr & 0xff;
    
    //channels
    u8 offset=rx_tx_addr[3]%5;             // 0-4
    for(u8 i=0;i<V911S_NUM_RF_CHANNELS;i++)
        hopping_frequency[i]=0x10+i*5+offset;
    if(!offset) hopping_frequency[0]++;
    
    // channels order
    rf_ch_num=rand32(0xfefefefe)&0x03;          // 0-3
}

static u16 V911S_callback()
{
    switch(phase) {
        case V911S_BIND:
            if (bind_counter == 0)
            {
                PROTOCOL_SetBindState(0);
                XN297_SetTXAddr(rx_tx_addr, 5);
                packet_period=V911S_PACKET_PERIOD;
                phase = V911S_DATA;
            }
            else
            {
                V911S_send_packet(1);
                bind_counter--;
                if(bind_counter==100)       // same as original TX...
                    packet_period=V911S_BIND_PACKET_PERIOD*3;
            }
            break;
        case V911S_DATA:
            V911S_send_packet(0);
            break;
    }
    return packet_period;
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    V911S_initialize_txid();
    tx_power = Model.tx_power;
    
    #ifdef V911S_ORIGINAL_ID
        rx_tx_addr[0]=0xA5;
        rx_tx_addr[1]=0xFF;
        rx_tx_addr[2]=0x70;
        rx_tx_addr[3]=0x8D;
        rx_tx_addr[4]=0x76;
        for(u8 i=0;i<V911S_NUM_RF_CHANNELS;i++)
            hopping_frequency[i]=0x10+i*5;
        hopping_frequency[0]++;
        rf_ch_num=0;
    #endif

    V911S_init();

    if(bind)
    {
        bind_counter = V911S_BIND_COUNT;
        packet_period= V911S_BIND_PACKET_PERIOD;
        PROTOCOL_SetBindState(((V911S_BIND_COUNT-100) * V911S_BIND_PACKET_PERIOD + V911S_BIND_PACKET_PERIOD*300) / 1000);
        phase = V911S_BIND;
    }
    else
    {
        XN297_SetTXAddr(rx_tx_addr, 5);
        packet_period= V911S_PACKET_PERIOD;
        phase = V911S_DATA;
    }
    hopping_frequency_no=0;
    
    CLOCK_StartTimer(V911S_INITIAL_WAIT, V911S_callback);
}

uintptr_t V911S_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 5;
        case PROTOCMD_DEFAULT_NUMCHAN: return 5;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
