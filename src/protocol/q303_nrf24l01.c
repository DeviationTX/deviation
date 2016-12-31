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
  #define Q303_Cmds PROTO_Cmds
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
    #define BIND_COUNT 4
    #define dbgprintf printf
#else
    #define BIND_COUNT 2335
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define PACKET_SIZE  10
#define Q303_PACKET_PERIOD   1500  // Timeout for callback in uSec
#define CX35_PACKET_PERIOD   3000
#define INITIAL_WAIT     500
#define RF_BIND_CHANNEL 0x02
#define NUM_RF_CHANNELS    4

static u8 packet[PACKET_SIZE];
static u8 phase;
static u16 bind_counter;
static u32 packet_counter;
static u8 tx_power;
static u8 tx_addr[5];
static u8 current_chan;
static u8 txid[4];
static u8 rf_chans[4];
static u16 packet_period;

enum {
    BIND,
    DATA
};

static const char * const q303_opts[] = {
    _tr_noop("Format"), "Q303", "CX35", NULL, 
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_Q303 = 0,
    FORMAT_CX35,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// For code readability
enum {
    CHANNEL1 = 0,   // Aileron
    CHANNEL2,       // Elevator
    CHANNEL3,       // Throttle
    CHANNEL4,       // Rudder
    CHANNEL5,       // Altitude Hold
    CHANNEL6,       // Flip
    CHANNEL7,       // Still Camera
    CHANNEL8,       // Video Camera
    CHANNEL9,       // Headless
    CHANNEL10,      // RTH
    CHANNEL11,      // Gimbal control (3 pos on Q303, full range on CX35)
};

#define CHANNEL_AHOLD    CHANNEL5  // Q303
#define CHANNEL_ARM      CHANNEL5  // CX35
#define CHANNEL_FLIP     CHANNEL6
#define CHANNEL_SNAPSHOT CHANNEL7
#define CHANNEL_VIDEO    CHANNEL8
#define CHANNEL_HEADLESS CHANNEL9
#define CHANNEL_RTH      CHANNEL10
#define CHANNEL_GIMBAL   CHANNEL11

// flags going to packet[8] (Q303)
#define FLAG_HIGHRATE 0x03
#define FLAG_AHOLD    0x40
#define FLAG_RTH      0x80

// flags going to packet[9] (Q303)
#define FLAG_GIMBAL_DN 0x04
#define FLAG_GIMBAL_UP 0x20
#define FLAG_HEADLESS  0x08
#define FLAG_FLIP      0x80
#define FLAG_SNAPSHOT  0x10
#define FLAG_VIDEO     0x01

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

static u8 cx35_lastButton()
{
    #define BTN_TAKEOFF  1
    #define BTN_DESCEND  2
    #define BTN_SNAPSHOT 4
    #define BTN_VIDEO    8
    #define BTN_RTH      16

    #define CMD_RATE     0x09
    #define CMD_TAKEOFF  0x0e
    #define CMD_DESCEND  0x0f
    #define CMD_SNAPSHOT 0x0b
    #define CMD_VIDEO    0x0c
    #define CMD_RTH      0x11
    
    static u8 cx35_btn_state;
    static u8 command;
    // simulate 2 keypress on rate button just after bind
    if(packet_counter < 50) {
        cx35_btn_state = 0;
        packet_counter++;
        command = 0x00; // startup
    }
    else if(packet_counter < 150) {
        packet_counter++;
        command = CMD_RATE; // 1st keypress
    }
    else if(packet_counter < 250) {
        packet_counter++;
        command |= 0x20; // 2nd keypress
    }
    
    // descend
    else if(!(GET_FLAG(CHANNEL_ARM, 1)) && !(cx35_btn_state & BTN_DESCEND)) {
        cx35_btn_state |= BTN_DESCEND;
        cx35_btn_state &= ~BTN_TAKEOFF;
        command = CMD_DESCEND;
    }
        
    // take off
    else if(GET_FLAG(CHANNEL_ARM,1) && !(cx35_btn_state & BTN_TAKEOFF)) {
        cx35_btn_state |= BTN_TAKEOFF;
        cx35_btn_state &= ~BTN_DESCEND;
        command = CMD_TAKEOFF;
    }
    
    // RTH
    else if(GET_FLAG(CHANNEL_RTH,1) && !(cx35_btn_state & BTN_RTH)) {
        cx35_btn_state |= BTN_RTH;
        if(command == CMD_RTH)
            command |= 0x20;
        else
            command = CMD_RTH;
    }
    else if(!(GET_FLAG(CHANNEL_RTH,1)) && (cx35_btn_state & BTN_RTH)) {
        cx35_btn_state &= ~BTN_RTH;
        if(command == CMD_RTH)
            command |= 0x20;
        else
            command = CMD_RTH;
    }
    
    // video
    else if(GET_FLAG(CHANNEL_VIDEO,1) && !(cx35_btn_state & BTN_VIDEO)) {
        cx35_btn_state |= BTN_VIDEO;
        if(command == CMD_VIDEO)
            command |= 0x20;
        else
            command = CMD_VIDEO;
    }
    else if(!(GET_FLAG(CHANNEL_VIDEO,1)) && (cx35_btn_state & BTN_VIDEO)) {
        cx35_btn_state &= ~BTN_VIDEO;
        if(command == CMD_VIDEO)
            command |= 0x20;
        else
            command = CMD_VIDEO;
    }
    
    // snapshot
    else if(GET_FLAG(CHANNEL_SNAPSHOT,1) && !(cx35_btn_state & BTN_SNAPSHOT)) {
        cx35_btn_state |= BTN_SNAPSHOT;
        if(command == CMD_SNAPSHOT)
            command |= 0x20;
        else
            command = CMD_SNAPSHOT;
    }
    
    if(!(GET_FLAG(CHANNEL_SNAPSHOT,1)))
        cx35_btn_state &= ~BTN_SNAPSHOT;
    
    return command;
}

static void send_packet(u8 bind)
{
    u16 aileron, elevator, throttle, rudder, slider;
    if(bind) {
        packet[0] = 0xaa;
        memcpy(&packet[1], txid, 4);
        memset(&packet[5], 0, 5);
    }
    else {
        aileron  = scale_channel(CHANNEL1, 1000, 0);
        elevator = scale_channel(CHANNEL2, 1000, 0);
        throttle = scale_channel(CHANNEL3, 0, 1000);
        rudder   = scale_channel(CHANNEL4, 0, 1000);
        
        if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_CX35)
            aileron = 1000 - aileron;
        
        packet[0] = 0x55;
        packet[1] = aileron >> 2;          // 8 bits
        packet[2] = (aileron & 0x03) << 6  // 2 bits
                  | (elevator >> 4);       // 6 bits
        packet[3] = (elevator & 0x0f) << 4 // 4 bits
                  | (throttle >> 6);       // 4 bits
        packet[4] = (throttle & 0x3f) << 2 // 6 bits 
                  | (rudder >> 8);         // 2 bits
        packet[5] = rudder & 0xff;         // 8 bits
        
        switch(Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_Q303:
                packet[6] = 0x10; // trim(s) ?
                packet[7] = 0x10; // trim(s) ?
                packet[8] = 0x03  // high rate (0-3)
                          | GET_FLAG(CHANNEL_AHOLD, FLAG_AHOLD)
                          | GET_FLAG(CHANNEL_RTH, FLAG_RTH);
                packet[9] = 0x40 // always set
                          | GET_FLAG(CHANNEL_HEADLESS, FLAG_HEADLESS)
                          | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP)
                          | GET_FLAG(CHANNEL_SNAPSHOT, FLAG_SNAPSHOT)
                          | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO);
                if(Channels[CHANNEL_GIMBAL] < CHAN_MIN_VALUE / 2)
                    packet[9] |= FLAG_GIMBAL_DN;
                else if(Channels[CHANNEL_GIMBAL] > CHAN_MAX_VALUE / 2)
                    packet[9] |= FLAG_GIMBAL_UP;
                break;
            case FORMAT_CX35:
                slider = scale_channel(CHANNEL_GIMBAL, 731, 342);
                packet[6] = slider >> 2;
                packet[7] = ((slider & 3) << 6)
                          | 0x3e; // ?? 6 bit left (always 111110 ?)
                packet[8] = 0x80; // always set
                packet[9] = cx35_lastButton();
                break;
        }
    }
    
    // Power on, TX mode, CRC enabled
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? RF_BIND_CHANNEL : rf_chans[current_chan++]);
    current_chan %= NUM_RF_CHANNELS;

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

static void q303_init()
{
    const u8 bind_address[] = {0xcc,0xcc,0xcc,0xcc,0xcc};
    
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_CX35) {
        XN297_SetScrambledMode(XN297_SCRAMBLED);
        NRF24L01_SetBitrate(NRF24L01_BR_1M);
    }
    else { // Q303
        XN297_SetScrambledMode(XN297_UNSCRAMBLED);
        NRF24L01_SetBitrate(NRF24L01_BR_250K);
    }
    XN297_SetTXAddr(bind_address, 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
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
static u16 q303_callback()
{
    switch (phase) {
        case BIND:
            if (bind_counter == 0) {
                tx_addr[0] = 0x55;
                memcpy(&tx_addr[1], txid, 4);
                XN297_SetTXAddr(tx_addr, 5);
                phase = DATA;
                packet_counter = 0;
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
    return packet_period;
}

static void initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    u8 i,j,offset;
    
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

    for(i=0; i<4; i++)
        txid[i] = (lfsr >> (i*8)) & 0xff;
    offset = txid[0] & 3;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_CX35) {        
        for(i=0; i<NUM_RF_CHANNELS; i++)
            rf_chans[i] = 0x14 + i*3 + offset;
    }
    else{ // Q303
        for(i=0; i<NUM_RF_CHANNELS; i++)
            rf_chans[i] = 0x46 + i*2 + offset;
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    initialize_txid();
    q303_init();
    bind_counter = BIND_COUNT;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q303)
        packet_period = Q303_PACKET_PERIOD;
    else if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_CX35)
        packet_period = CX35_PACKET_PERIOD;
    current_chan = 0;
    PROTOCOL_SetBindState(BIND_COUNT * packet_period / 1000);
    phase = BIND;
    CLOCK_StartTimer(INITIAL_WAIT, q303_callback);
}

const void *Q303_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 11L; // A, E, T, R, altitude hold, flip, photo, video, headless, RTH, gimbal
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)11L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return q303_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif

