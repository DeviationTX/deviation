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
    #define BIND_COUNT 300
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD       4500 // stock Tx=9000, but let's send more packets ...
#define INITIAL_WAIT        500
#define RF_CHANNEL          0x2d // 2445 MHz
#define ADDRESS_LENGTH      5
#define PACKET_SIZE         10 // bind packet = 9

static const u8 bind_address[ADDRESS_LENGTH] = {0x62, 0x54, 0x79, 0x38, 0x53};
static u8 tx_addr[ADDRESS_LENGTH];
static u8 packet[PACKET_SIZE];
static u8 phase;
static u16 bind_counter;
static u8 tx_power;
static u8 armed, arm_flags;
static u8 arm_channel_previous;

enum {
    BIND,
    DATA
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
    CHANNEL10,
};

#define CHANNEL_ARM      CHANNEL5
#define CHANNEL_LED      CHANNEL6
#define CHANNEL_FLIP     CHANNEL7
#define CHANNEL_HEADLESS CHANNEL9
#define CHANNEL_RTH      CHANNEL10

// flags packet[6]
#define FLAG_DISARM     0x80
#define FLAG_ARM        0x40

// flags packet[7]
#define FLAG_FLIP       0x80
#define FLAG_HEADLESS   0x10
#define FLAG_RTH        0x08
#define FLAG_LED        0x04
#define FLAG_EXPERT     0x02
#define FLAG_INTERMEDIATE 0x01

// Bit vector from bit position
#define BV(bit) (1 << bit)

static void e015_init()
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
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
    NRF24L01_Activate(0x73);
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static u8 scale_channel(u8 ch, u8 destMin, u8 destMax)
{
    s32 chanval = Channels[ch];
    s32 range = (s32) destMax - (s32) destMin;

    if (chanval < CHAN_MIN_VALUE)
        chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE)
        chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

static void check_arming(s32 channel_value) {
    u8 arm_channel = channel_value > 0;

    if (arm_channel != arm_channel_previous) {
        arm_channel_previous = arm_channel;
        if (arm_channel) {
            armed = 1;
            arm_flags ^= FLAG_ARM;
        } else {
            armed = 0;
            arm_flags ^= FLAG_DISARM;
        }
    }
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    if(bind) {
        packet[0] = 0x18;
        packet[1] = 0x04;
        packet[2] = 0x06;
        // data phase address
        packet[3] = tx_addr[0];
        packet[4] = tx_addr[1];
        packet[5] = tx_addr[2];
        packet[6] = tx_addr[3];
        packet[7] = tx_addr[4];
        // checksum
        packet[8] = packet[3];
        for(u8 i=4; i<8; i++)
            packet[8] += packet[i];
    }
    else {
        check_arming(Channels[CHANNEL_ARM]);
        packet[0] = scale_channel(CHANNEL3, 0, 225); // throttle
        packet[1] = scale_channel(CHANNEL4, 225, 0); // rudder
        packet[2] = scale_channel(CHANNEL1, 0, 225); // aileron
        packet[3] = scale_channel(CHANNEL2, 225, 0); // elevator
        packet[4] = 0x20; // elevator trim
        packet[5] = 0x20; // aileron trim
        packet[6] = arm_flags;
        packet[7] = FLAG_EXPERT
                  | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP)
                  | GET_FLAG(CHANNEL_LED, FLAG_LED)
                  | GET_FLAG(CHANNEL_HEADLESS, FLAG_HEADLESS)
                  | GET_FLAG(CHANNEL_RTH, FLAG_RTH);
        packet[8] = 0;
        // checksum
        packet[9] = packet[0];
        for(u8 i=1; i<9; i++)
            packet[9] += packet[i];
    }
    
    // Power on, TX mode, CRC enabled
    HS6200_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
   
    // transmit packet twice in a row without waiting for
    // the first one to complete, seems to help the hs6200
    // demodulator to start decoding.
    HS6200_WritePayload(packet, bind ? 9 : PACKET_SIZE);
    HS6200_WritePayload(packet, bind ? 9 : PACKET_SIZE);
    
    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static u16 e015_callback()
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
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    initialize_txid();
    e015_init();
    bind_counter = BIND_COUNT;
    phase = BIND;
    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    armed = 0;
    arm_flags = 0;
    arm_channel_previous = Channels[CHANNEL_ARM] > 0;
    CLOCK_StartTimer(INITIAL_WAIT, e015_callback);
}

uintptr_t E015_Cmds(enum ProtoCmds cmd)
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
