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
    #define dbgprintf printf
#else
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT    500
#define PACKET_PERIOD   2400
#define RF_BIND_CHANNEL 2
#define PAYLOAD_SIZE    15

static u8 packet[PAYLOAD_SIZE];
static u8 rf_chans[4];
static u8 current_chan;
static u8 phase;
static u8 tx_power;
static u8 rxid;
static u8 txid[2];

enum {
    BIND1,
    BIND2,
    DATA
};

// For code readability
enum {
    CHANNEL1 = 0,   // Aileron
    CHANNEL2,       // Elevator
    CHANNEL3,       // Throttle
    CHANNEL4,       // Rudder
    CHANNEL5,       // 
    CHANNEL6,       // Flip
};

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

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

static void send_packet(u8 bind)
{
    packet[0] = txid[0];
    if(bind) {
        packet[1] = 0x55;
        packet[2] = rf_chans[0];
        packet[3] = rf_chans[1];
        packet[4] = rf_chans[2];
        packet[5] = rf_chans[3];
        memset(&packet[6], 0, 7);
        packet[13] = 0xaa;
    }
    else {
        packet[1] = 0x01 | GET_FLAG(CHANNEL6, 0x40); // flip
        packet[2] = scale_channel(CHANNEL1, 200, 0); // aileron
        packet[3] = scale_channel(CHANNEL2, 0, 200); // elevator
        packet[4] = scale_channel(CHANNEL4, 200, 0); // rudder
        packet[5] = scale_channel(CHANNEL3, 0, 200); // throttle
        packet[6] = 0xaa;
        packet[7] = 0x02; // max rate
        packet[8] = 0x00;
        packet[9] = 0x00;
        packet[10]= 0x00;
        packet[11]= 0x00;
        packet[12]= 0x00;
        packet[13]= rxid;
    }
    packet[14] = txid[1];
    
    // Power on, TX mode, CRC enabled
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? RF_BIND_CHANNEL : rf_chans[(current_chan++)/2]);
    current_chan %= 8;

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WriteEnhancedPayload(packet, PAYLOAD_SIZE, 0, 0x3c7d);
    
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void gw008_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((u8*)"\xcc\xcc\xcc\xcc\xcc", 5);
    XN297_SetRXAddr((u8*)"\xcc\xcc\xcc\xcc\xcc", 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOAD_SIZE+2); // payload + 2 bytes for pcf
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);    // Set feature bits on
    NRF24L01_Activate(0x73);
}

static void initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    u8 i,j;
    
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for(i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif

    if(Model.fixed_id) {
       for (i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for(i=0; i<sizeof(lfsr); ++i) 
        rand32_r(&lfsr, 0);

    for(i=0; i<4; i++)
        rf_chans[i] = 0x10 + ((lfsr >> (i*8)) % 0x37);
    
    txid[0] = lfsr & 0xff;
    txid[1] = lfsr >> 8;
}

static u16 gw008_callback()
{
    switch(phase) {
        case BIND1:
            if((NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) &&   // RX fifo data ready
                XN297_ReadEnhancedPayload(packet, PAYLOAD_SIZE) == PAYLOAD_SIZE && // check payload size
                packet[0] == txid[0] && packet[14] == txid[1]) { // check tx id
                NRF24L01_SetTxRxMode(TXRX_OFF);
                NRF24L01_SetTxRxMode(TX_EN);
                rxid = packet[13];
                PROTOCOL_SetBindState(0);
                phase = DATA;
            } else {
                NRF24L01_SetTxRxMode(TXRX_OFF);
                NRF24L01_SetTxRxMode(TX_EN);
                send_packet(1);
                phase = BIND2;
                return 300;
            }
            break;
        case BIND2:
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_FlushRx();
            NRF24L01_SetTxRxMode(RX_EN);
            XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) 
                          | BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_PRIM_RX)); 
            phase = BIND1;
            return 5000;
            break;
        case DATA:
            send_packet(0);
            break;
    }
    return PACKET_PERIOD;
}

static void initialize()
{
    CLOCK_StopTimer();
    initialize_txid();
    tx_power = Model.tx_power;
    phase = BIND1;
    gw008_init();
    current_chan = 0;
    PROTOCOL_SetBindState(0xffffffff);
    CLOCK_StartTimer(INITIAL_WAIT, gw008_callback);
}

uintptr_t GW008_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;  // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 6;  // A, E, T, R, N/A, flip
        case PROTOCMD_DEFAULT_NUMCHAN: return 6;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return 0;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
