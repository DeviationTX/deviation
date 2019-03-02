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
    #define dbgprintf printf
#else
    #define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT    500
#define PACKET_INTERVAL 6840
#define WRITE_WAIT      2000
#define TX_PAYLOAD_SIZE 24
#define RX_PAYLOAD_SIZE 16
#define NUM_RF_CHANNELS 15
#define ADDRESS_SIZE    5

static u8 packet[TX_PAYLOAD_SIZE];
static u8 current_chan;
static u8 rx_tx_addr[ADDRESS_SIZE];
static u8 phase;
static u8 tx_power;
static u8 armed, arm_flags;
static u8 arm_channel_previous;
static u8 rf_chans[NUM_RF_CHANNELS];
static u8 txid[3];
static u8 tx_hash;

static const u8 bind_chans[NUM_RF_CHANNELS] = {0x1A,0x23,0x2C,0x35,0x3E,0x17,0x20,0x29,0x32,0x3B,0x14,0x1D,0x26,0x2F,0x38}; // bugs 3 mini

// haven't figured out txid<-->rf channel mapping yet
static const struct {
    u8 txid[3];
    u8 tx_hash; // 2nd byte of final address
    u8 rf_chans[NUM_RF_CHANNELS];
} tx_rf_map[] = {
    // dumped from Tx #1 (C0ckpitvue 777)
    {{0xA8,0xE6,0x32},
      0x6c,
     {0x22,0x2f,0x3a,0x14,0x20,0x2d,0x38,0x18,0x26,0x32,0x11,0x1d,0x29,0x35,0x17}},
    // dumped from Tx #2 (DPyro)
    {{0xdd,0xab,0xfd},
      0x9e,
     {0x3d,0x34,0x2b,0x22,0x19,0x40,0x37,0x2e,0x25,0x1c,0x3a,0x31,0x28,0x1f,0x16}},
    // dumped from Tx #3 (goebish)
    {{0x90,0x9e,0x4a},
      0x3d,
     {0x12,0x20,0x2f,0x1a,0x28,0x38,0x14,0x23,0x32,0x1c,0x2c,0x3b,0x17,0x26,0x34}},
    // dumped from Tx #4 (aszasza)
    {{0x20,0x28,0xBA},
      0xb3,
     {0x13,0x25,0x37,0x1F,0x31,0x17,0x28,0x3A,0x1C,0x2E,0x22,0x33,0x19,0x2B,0x3D}}
};
#define NUM_TX_RF_MAPS (sizeof(tx_rf_map)/sizeof(tx_rf_map[0]))

enum {
    BIND1,
    BIND2,
    DATA1,
    DATA2
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
    CHANNEL10
};

#define CHANNEL_ARM         CHANNEL5
#define CHANNEL_LED         CHANNEL6
#define CHANNEL_FLIP        CHANNEL7
#define CHANNEL_PICTURE     CHANNEL8
#define CHANNEL_VIDEO       CHANNEL9
#define CHANNEL_ANGLE       CHANNEL10

// flags packet[12]
#define FLAG_FLIP    0x08    // automatic flip
#define FLAG_MODE    0x04    // low/high speed select (set is high speed)
#define FLAG_VIDEO   0x02    // toggle video
#define FLAG_PICTURE 0x01    // toggle picture

// flags packet[13]
#define FLAG_LED     0x80    // enable LEDs
#define FLAG_ARM     0x40    // arm (toggle to turn on motors)
#define FLAG_DISARM  0x20    // disarm (toggle to turn off motors)
#define FLAG_ANGLE   0x02    // angle/acro mode (set is angle mode)

static const char *const bugs3mini_opts[] = {
    _tr_noop("RX Id"), "-32768", "32767", "1", NULL,
    NULL
};

enum {
    PROTOOPTS_RXID = 0,
    LAST_PROTO_OPT
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 checksum = 0x6d;
    for(u8 i=1; i < TX_PAYLOAD_SIZE; i++)
        checksum ^= packet[i];
    return checksum;
}

static void bugs3mini_init()
{
    tx_power = Model.tx_power;
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, RX_PAYLOAD_SIZE); // bytes of data payload for rx pipe 1
    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    NRF24L01_SetPower(tx_power);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on
}

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
    check_arming(Channels[CHANNEL_ARM]);  // sets globals arm_flags and armed
    
    u16 aileron = scale_channel(CHANNEL1, 500, 0);
    u16 elevator = scale_channel(CHANNEL2, 0, 500);
    u16 throttle = armed ? scale_channel(CHANNEL3, 0, 500) : 0;
    u16 rudder = scale_channel(CHANNEL4, 500, 0);
    
    packet[1] = txid[0];
    packet[2] = txid[1];
    packet[3] = txid[2];
    if(bind) {
        packet[4] = 0x00;
        packet[5] = 0x7d;
        packet[6] = 0x7d;
        packet[7] = 0x7d;
        packet[8] = 0x20;
        packet[9] = 0x20;
        packet[10]= 0x20;
        packet[11]= 0x40;
        packet[12]^= 0x40; // alternating freq hopping flag
        packet[13]= 0x60;
        packet[14]= 0x00;
        packet[15]= 0x00;
    }
    else {
        packet[4] = throttle >> 1;
        packet[5] = rudder >> 1;
        packet[6] = elevator >> 1;
        packet[7] = aileron >> 1;
        packet[8] = 0x20 | (aileron << 7);
        packet[9] = 0x20 | (elevator << 7);
        packet[10]= 0x20 | (rudder << 7);
        packet[11]= 0x40 | (throttle << 7);
        packet[12]= 0x80 | (packet[12] ^ 0x40) // bugs 3 H doesn't have 0x80 ?
                  | FLAG_MODE
                  | GET_FLAG(CHANNEL_PICTURE, FLAG_PICTURE)
                  | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO);
        if(armed)
            packet[12] |= GET_FLAG(CHANNEL_FLIP, FLAG_FLIP);
        packet[13] = arm_flags
                   | GET_FLAG(CHANNEL_LED, FLAG_LED)
                   | GET_FLAG(CHANNEL_ANGLE, FLAG_ANGLE);
        
        packet[14] = 0;
        packet[15] = 0; // 0x53 on bugs 3 H ?
    }
    packet[0] = checksum();
    
    if(!(packet[12]&0x40)) {
        current_chan++;
        if(current_chan >= NUM_RF_CHANNELS)
            current_chan = 0;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? bind_chans[current_chan] : rf_chans[current_chan]);
    }
    
    // Power on, TX mode, 2byte CRC
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, TX_PAYLOAD_SIZE);
    
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

// compute final address for the rxid received during bind
// thanks to Pascal for the function!
static void make_address()
{
    u8 start, length, index;
    u8 rxid_high = Model.proto_opts[PROTOOPTS_RXID] >> 8;
    u8 rxid_low  = Model.proto_opts[PROTOOPTS_RXID] & 0xff;
    
    static const u16 end[]={
        0x2d9e, 0x95a4, 0x9c5c, 0xb4a6, 0xa9ce, 0x562b, 0x3e73, 0xb895, 0x6a82, 0x9437, 0x3d5a,
        0x4bb2, 0x6949, 0xc224, 0x6b3d, 0x23c6, 0x9ea3, 0xa498, 0x5c9e, 0xa652, 0xce76, 0x2b4b, 0x733a };
    
    if(rxid_high==0x00 || rxid_high==0xFF)
        rx_tx_addr[0]=0x52;
    else
        rx_tx_addr[0]=rxid_high;
    
    rx_tx_addr[1]=tx_hash;
    
    if(rxid_low==0x00 || rxid_low==0xFF)
        rx_tx_addr[2]=0x66;
    else
        rx_tx_addr[2]=rxid_low;
    
    for(u8 end_idx=0;end_idx<23;end_idx++)
    {
        //calculate sequence start
        if(end_idx<=7)
            start=end_idx;
        else
            start=(end_idx-7)*16+7;
        //calculate sequence length
        if(end_idx>6)
        {
            if(end_idx>15)
                length=(23-end_idx)<<1;
            else
                length=16;
        }
        else
            length=(end_idx+1)<<1;
        //scan for a possible solution using the current end
        for(u8 i=0;i<length;i++)
        {
            index=(i>>1)*7+(((i+1)>>1)<<3);
            index=start+index-rxid_high;
            if(index==rxid_low)
            {
                rx_tx_addr[3]=end[end_idx]>>8;
                rx_tx_addr[4]=end[end_idx];
                return;
            }
        }
    }
    // Something wrong happened if we arrive here....
}

static void update_telemetry() {
    u8 checksum = 0x6d;
    for(u8 i=1; i<12; i++) {
        checksum += packet[i];
    }
    if(packet[0] == checksum) {
        Telemetry.value[TELEM_FRSKY_RSSI] = packet[3];
        TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
        if(packet[11] & 0x80)
            Telemetry.value[TELEM_FRSKY_VOLT1] = 840; // Ok
        else if(packet[11] & 0x40)
            Telemetry.value[TELEM_FRSKY_VOLT1] = 710; // Warning
        else
            Telemetry.value[TELEM_FRSKY_VOLT1] = 640; // Critical
        TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
    }
}

static u16 bugs3mini_callback()
{
    switch(phase) {
        case BIND1:
            if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) { // RX fifo data ready
                XN297_ReadPayload(packet, RX_PAYLOAD_SIZE);
                Model.proto_opts[PROTOOPTS_RXID] = (u16)packet[1]<<8 | packet[2]; // store rxid into protocol options
                NRF24L01_SetTxRxMode(TXRX_OFF);
                NRF24L01_SetTxRxMode(TX_EN);
                make_address();
                XN297_SetTXAddr(rx_tx_addr, 5);
                XN297_SetRXAddr(rx_tx_addr, 5);
                phase = DATA1;
                PROTOCOL_SetBindState(0);
                return PACKET_INTERVAL;
            }
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            send_packet(1);
            phase = BIND2;
            return WRITE_WAIT;
        case BIND2:
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(RX_EN);
            NRF24L01_FlushRx();
            XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) 
                          | BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_PRIM_RX));
            phase = BIND1;
            return PACKET_INTERVAL - WRITE_WAIT;
        case DATA1:
            if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) { // RX fifo data ready
                // read only 12 bytes to not overwrite channel change flag
                XN297_ReadPayload(packet, 12);
                update_telemetry();
            }
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            send_packet(0);
            phase = DATA2;
            return WRITE_WAIT;
        case DATA2:
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(RX_EN);
            NRF24L01_FlushRx();
            XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) 
                          | BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_PRIM_RX));
            phase = DATA1;
            return PACKET_INTERVAL - WRITE_WAIT;
    }
    return PACKET_INTERVAL;
}

static void initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    
    memcpy(txid, tx_rf_map[lfsr % NUM_TX_RF_MAPS].txid, sizeof(txid));
    memcpy(rf_chans, tx_rf_map[lfsr % NUM_TX_RF_MAPS].rf_chans, sizeof(rf_chans));
    tx_hash = tx_rf_map[lfsr % NUM_TX_RF_MAPS].tx_hash;
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    initialize_txid();
    memset(packet, (u8)0, sizeof(packet));
    bugs3mini_init();
    if(bind) {
        phase = BIND1;
        XN297_SetTXAddr((const u8*)"mjxRC", 5);
        XN297_SetRXAddr((const u8*)"mjxRC", 5);
        PROTOCOL_SetBindState(0xFFFFFFFF);
    }
    else {
        make_address();
        XN297_SetTXAddr(rx_tx_addr, 5);
        XN297_SetRXAddr(rx_tx_addr, 5);
        phase = DATA1;
    }
    armed = 0;
    arm_flags = FLAG_DISARM;    // initial value from captures
    arm_channel_previous = Channels[CHANNEL_ARM] > 0;
    CLOCK_StartTimer(INITIAL_WAIT, bugs3mini_callback);
}

uintptr_t BUGS3MINI_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 10;
        case PROTOCMD_DEFAULT_NUMCHAN: return 10;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)bugs3mini_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_FRSKY;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
