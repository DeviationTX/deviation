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
#include "telemetry.h"

#ifdef PROTO_HAS_NRF24L01

#ifdef EMULATOR
    #define USE_FIXED_MFGID
    #define DM002_BIND_COUNT 20
    #define PACKET_PERIOD 150
    #define dbgprintf printf
#else
    #define PACKET_PERIOD 6100 // Timeout for callback in uSec
    #define DM002_BIND_COUNT 655 // 4 seconds
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define DM002_PACKET_PERIOD     6100 // Timeout for callback in uSec
#define DM002_INITIAL_WAIT      500
#define DM002_PACKET_SIZE       12   // packets have 12-byte payload
#define DM002_RF_BIND_CHANNEL   0x27
#define DM002_NUM_RF_CHANNEL    4
#define DM002_ADDRESS_SIZE      5

static u8 packet[DM002_PACKET_SIZE];
static u8 hopping_frequency[DM002_NUM_RF_CHANNEL];
static u8 hopping_frequency_no;
static u8 rx_tx_addr[DM002_ADDRESS_SIZE];
static u8 tx_power;
static u8 phase;
static u8 packet_count;
static u16 bind_counter;


enum DM002_FLAGS {
    // flags going to packet[9]
    DM002_FLAG_FLIP     = 0x01, 
    DM002_FLAG_LED      = 0x02, 
    DM002_FLAG_MEDIUM   = 0x04, 
    DM002_FLAG_HIGH     = 0x08, 
    DM002_FLAG_RTH      = 0x10,
    DM002_FLAG_HEADLESS = 0x20,
    DM002_FLAG_CAMERA1  = 0x40,
    DM002_FLAG_CAMERA2  = 0x80,
};

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Leds
    CHANNEL6,     // Flip
    CHANNEL7,     // Camera 1
    CHANNEL8,     // Camera 2
    CHANNEL9,     // Headless
    CHANNEL10,    // RTH
};

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_CAMERA1     CHANNEL7
#define CHANNEL_CAMERA2     CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10

enum {
    BIND,
    DATA
};

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
#define GET_FLAG_INV(ch, mask) (Channels[ch] <= 0 ? mask : 0)

static void DM002_send_packet(u8 bind)
{
    memcpy(&packet[5],(uint8_t *)"\x00\x7F\x7F\x7F\x00\x00\x00",7);
    if(bind)
    {
        packet[0] = 0xAA;
        packet[1] = rx_tx_addr[0]; 
        packet[2] = rx_tx_addr[1];
        packet[3] = rx_tx_addr[2];
        packet[4] = rx_tx_addr[3];
    }
    else
    {
        packet[0]=0x55;
        // Throttle : 0 .. 200
        packet[1]=scale_channel(CHANNEL3,0,200);
        // Other channels min 0x57, mid 0x7F, max 0xA7
        packet[2] = scale_channel(CHANNEL4,0xA7,0x57);     // rudder
        packet[3] = scale_channel(CHANNEL1, 0xA7,0x57);  // aileron
        packet[4] = scale_channel(CHANNEL2, 0xA7, 0x57); // elevator
        // Features
        packet[9] =   DM002_FLAG_HIGH // high rate
                    | GET_FLAG(CHANNEL_FLIP,    DM002_FLAG_FLIP)
                    | GET_FLAG_INV(CHANNEL_LED, DM002_FLAG_LED)
                    | GET_FLAG(CHANNEL_CAMERA1, DM002_FLAG_CAMERA1)
                    | GET_FLAG(CHANNEL_CAMERA2, DM002_FLAG_CAMERA2)
                    | GET_FLAG(CHANNEL_HEADLESS,DM002_FLAG_HEADLESS)
                    | GET_FLAG(CHANNEL_RTH,     DM002_FLAG_RTH);
        // Packet counter
        if(packet_count&0x03)
        {
            packet_count++;
            hopping_frequency_no++;
            hopping_frequency_no&=4;
        }
        packet_count&=0x0F;
        packet[10] = packet_count;
        packet_count++;
    }
    //CRC
    for(uint8_t i=0;i<DM002_PACKET_SIZE-1;i++)
        packet[11]+=packet[i];
    
    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (bind)
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, DM002_RF_BIND_CHANNEL);
    else
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, DM002_PACKET_SIZE);

    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void DM002_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((uint8_t *)"\x26\xA8\x67\x35\xCC", DM002_ADDRESS_SIZE);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
}

static u16 dm002_callback()
{
    switch(phase) {
        case BIND:
            if(bind_counter == 0) {
                phase = DATA;
                XN297_SetTXAddr(rx_tx_addr, DM002_ADDRESS_SIZE);
            }
            else {
                DM002_send_packet(1);
                bind_counter--;
            }
            break;
            
        case DATA:
            DM002_send_packet(0);
            break;
    }
    return DM002_PACKET_PERIOD;
}

static void DM002_initialize_txid()
{
    // Not figured out txid / rf channels yet
    // Only 3 IDs/RFs are available, model txid (even/odd) is used to switch between them
    switch(Model.fixed_id % 3) {
        case 0:
            memcpy(hopping_frequency,(uint8_t *)"\x34\x39\x43\x48",DM002_NUM_RF_CHANNEL);
            memcpy(rx_tx_addr,(uint8_t *)"\x47\x93\x00\x00\xD5",DM002_ADDRESS_SIZE);
            break;
        case 1:
            memcpy(hopping_frequency,(uint8_t *)"\x35\x39\x3B\x3D",DM002_NUM_RF_CHANNEL);
            memcpy(rx_tx_addr,(uint8_t *)"\xAC\xA1\x00\x00\xD5",DM002_ADDRESS_SIZE);
            break;
        case 2:
            memcpy(hopping_frequency,(uint8_t *)"\x32\x37\x41\x46",DM002_NUM_RF_CHANNEL);
            memcpy(rx_tx_addr,(uint8_t *)"\x92\x45\x01\x00\xD5",DM002_ADDRESS_SIZE);
            break;
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    DM002_initialize_txid();
    DM002_init();
    packet_count = 0;
    hopping_frequency_no = 0;
    bind_counter = DM002_BIND_COUNT;
    phase = BIND;
    CLOCK_StartTimer(DM002_INITIAL_WAIT, dm002_callback);
}

uintptr_t DM002_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;  // always Autobind
        case PROTOCMD_BIND: initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 10;
        case PROTOCMD_DEFAULT_NUMCHAN: return 10;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return 0;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
