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
    #define BIND_COUNT 4
    #define dbgprintf printf
#else
    #define BIND_COUNT 500
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD       4525
#define INITIAL_WAIT        500
#define RF_BIND_CHANNEL     0x3c
#define ADDRESS_LENGTH      5
#define NUM_RF_CHANNELS     4
#define PACKET_SIZE         15

static const u8 bind_address[ADDRESS_LENGTH] = {0x55,0x42,0x9C,0x8F,0xC9};
static u8 tx_addr[ADDRESS_LENGTH];
static u8 rf_chans[NUM_RF_CHANNELS];
static u8 packet[PACKET_SIZE];
static u8 phase;
static u16 bind_counter;
static u8 tx_power;
static u8 current_chan;

enum {
    BIND,
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
    CHANNEL7,       //
    CHANNEL8,       //
    CHANNEL9,       // Headless
    CHANNEL10,      // RTH
};

#define CHANNEL_FLIP     CHANNEL6
#define CHANNEL_HEADLESS CHANNEL9
#define CHANNEL_RTH      CHANNEL10

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
static void send_packet(u8 bind)
{
    packet[0] = tx_addr[1];
    if(bind) {
        packet[1] = 0xaa;
        memcpy(&packet[2], rf_chans, NUM_RF_CHANNELS);
        memcpy(&packet[6], tx_addr, ADDRESS_LENGTH);
    }
    else {
        packet[1] = 0x01
                  | GET_FLAG(CHANNEL_RTH, 0x04)
                  | GET_FLAG(CHANNEL_HEADLESS, 0x10)
                  | GET_FLAG(CHANNEL_FLIP, 0x40);
        packet[2] = scale_channel(CHANNEL1, 0xc8, 0x00); // aileron
        packet[3] = scale_channel(CHANNEL2, 0x00, 0xc8); // elevator
        packet[4] = scale_channel(CHANNEL4, 0xc8, 0x00); // rudder
        packet[5] = scale_channel(CHANNEL3, 0x00, 0xc8); // throttle
        packet[6] = 0xaa;
        packet[7] = 0x02; // rate (0-2)
        packet[8] = 0x00;
        packet[9] = 0x00;
        packet[10]= 0x00;
    }
    packet[11] = 0x00;
    packet[12] = 0x00;
    packet[13] = 0x56; 
    packet[14] = tx_addr[2];
    
    // Power on, TX mode, CRC enabled
    HS6200_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? RF_BIND_CHANNEL : rf_chans[current_chan++]);
    current_chan %= NUM_RF_CHANNELS;
    
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    
    // transmit packet twice in a row without waiting for
    // the first one to complete, seems to help the hs6200
    // demodulator to start decoding.
    HS6200_WritePayload(packet, PACKET_SIZE);
    HS6200_WritePayload(packet, PACKET_SIZE);
    
    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void e012_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    HS6200_SetTXAddr(bind_address, ADDRESS_LENGTH);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1 Mbps
    NRF24L01_SetPower(tx_power);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
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

    // tx address
    for(i=0; i<4; i++)
        tx_addr[i] = (lfsr >> (i*8)) & 0xff;
    rand32_r(&lfsr, 0);
    tx_addr[4] = lfsr & 0xff;
    
    // rf channels
    rand32_r(&lfsr, 0);
    for(i=0; i<NUM_RF_CHANNELS; i++) {
        rf_chans[i] = 0x10 + (((lfsr >> (i*8)) & 0xff) % 0x32); 
    }
}

static u16 e012_callback()
{
    switch (phase) {
        case BIND:
            if (bind_counter == 0) {
                HS6200_SetTXAddr(tx_addr, 5);
                phase = DATA;
                PROTOCOL_SetBindState(0);
            } else {
                send_packet(1);
                bind_counter--;
            }
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
    tx_power = Model.tx_power;
    initialize_txid();
    e012_init();
    bind_counter = BIND_COUNT;
    current_chan = 0;
    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    phase = BIND;
    CLOCK_StartTimer(INITIAL_WAIT, e012_callback);
}

uintptr_t E012_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;  // always Autobind
        case PROTOCMD_BIND: initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 10;  // A, E, T, R, n/a , flip, n/a, n/a, headless, RTH
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
