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
 
//****************************************************************************
//  ESky protocol for small models since 2014 (150, 300, 150X, ...)
//****************************************************************************
// 
//  Relating threads (in order of relevance):
//
//    https://www.deviationtx.com/forum/6-general-discussions/6446-esky-150x-which-protocol
//    https://www.deviationtx.com/forum/builds/2458-devo10-esky-protocol-draft
//    https://www.deviationtx.com/forum/3-feedback-questions/4007-esky150-protocol-and-devo-6-8s
//
//****************************************************************************
 
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"
#include "telemetry.h"

#ifdef PROTO_HAS_NRF24L01
#include "iface_nrf24l01.h"

#ifdef MODULAR
  //Some versions of gcc apply this to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern u32 _data_loadaddr;
  const u32 protocol_type = (u32)&_data_loadaddr;

  //Allows the linker to properly relocate
  #define ESKY150_Cmds PROTO_Cmds
  #pragma long_calls
#endif

//================================================================================================
// Definitions
//================================================================================================

// Wait for RX chip stable - 10ms
#define INIT_WAIT_MS  10000

// Callback timeout period for sending bind packets, minimum 250
#define BINDING_PACKET_PERIOD  2000

// Timeout for sending data packets, in uSec, ESky2 protocol requires 4.8ms
#define SENDING_PACKET_PERIOD  4800

//ESky2 sends 1 data packet per frequency
#define PACKET_SEND_COUNT 1


// packets to be sent during binding
#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 5
#define dbgprintf printf
#else
#define BIND_COUNT 3000
#define dbgprintf if(0) printf
#endif 

#define PAYLOADSIZE 15

//ESky2 only uses 2 RF channels
#define RF_CH_COUNT 2
 
#define TX_ADDRESS_SIZE 4

#define NO_RF_CHANNEL_CHANGE -1

typedef int BOOL;
 
// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
    CHANNEL7
};
 
enum {
    PROTOOPTS_STARTBIND = 0,
    //PROTOOPTS_FIXEDRF,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);
 
enum {
    STARTBIND_NO  = 0,
    STARTBIND_YES = 1,
};

enum {
    FIXEDRF_NO = 0,
    FIXEDRF_YES = 1,
};
 
enum {
    STATE_PRE_BIND,
    STATE_BINDING,
    STATE_PRE_SEND,
    STATE_SENDING,
};

static const u8 u1_rx_addr[TX_ADDRESS_SIZE] = { 0x73, 0x73, 0x74, 0x63 }; //This RX address "sstc" is fixed for ESky2

//================================================================================================
// Private Function Prototypes
//================================================================================================
static void esky2_start_tx(BOOL bind_yes);
static u16  esky2_tx_callback();
 
static void esky2_init(u8 tx_addr[], u8 hopping_ch[]);
 
static void esky2_bind_init(u8 tx_addr[], u8 bind_packet[]);
 
static void esky2_calculate_frequency_hopping_channels(u32 seed, u8 hopping_channels[]);
static void esky2_calculate_tx_addr(u8 tx_addr[]);
static void esky2_send_packet(u8 packet[], s32 rf_ch);
 
static void esky2_send_init(u8 tx_addr[], u8 packet[]);
static void esky2_update_packet_control_data(u8 packet[], u8 hopping_ch[]);
 
static void esky2_read_controls(u16* throttle, u16* aileron, u16* elevator, u16* rudder, u16* flight_mode, u16* aux_ch6, u16* aux_ch7, u8* flags);
static u16  esky2_convert_channel(u8 num);
static u16  esky2_convert_2bit_channel(u8 num);
 
//================================================================================================
// Local Variables
//================================================================================================
static const char * const ESKY2_PROTOCOL_OPTIONS[] = {
  _tr_noop("Re-bind"),  _tr_noop("No"), _tr_noop("Yes"), NULL, 
//  _tr_noop("RF channels"), _tr_noop("random"), _tr_noop("fixed"), NULL,
  NULL
};
 
//Payload data buffer
static u8 packet_[PAYLOADSIZE];

static u8 hopping_channels_[RF_CH_COUNT];
 
//ESky2 uses 4 byte address
static u8 tx_addr_[TX_ADDRESS_SIZE];
 
static u32 tx_state_ = STATE_PRE_BIND;
static u32 tx_power_;
static u8 flags;
 
//================================================================================================
// Public Functions
//================================================================================================
const void *ESKY150_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:
        	esky2_start_tx((Model.proto_opts[PROTOOPTS_STARTBIND] == STARTBIND_YES));
            return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)((NRF24L01_Reset()) ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND:
            return (void *)0L; // Never Autobind
        case PROTOCMD_BIND:
        	esky2_start_tx(STARTBIND_YES);
			return 0;
        case PROTOCMD_NUMCHAN:
            return (void *)7L; // T, A, E, R, FMODE, AUX1, AUX2
        case PROTOCMD_DEFAULT_NUMCHAN:
            return (void *)4L;
        case PROTOCMD_CURRENT_ID: 
            return (Model.fixed_id) ? (void *)Model.fixed_id : 0;
        case PROTOCMD_GETOPTIONS: 
            return ESKY2_PROTOCOL_OPTIONS;
        case PROTOCMD_TELEMETRYSTATE: 
            return (void *)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
 
//================================================================================================
// Private Functions
//================================================================================================
static void esky2_start_tx(BOOL bind_yes)
{
    CLOCK_StopTimer();
    dbgprintf("ESky2 protocol started\n");
    esky2_init(tx_addr_, hopping_channels_);
    if(bind_yes)
    {
        PROTOCOL_SetBindState(BIND_COUNT * BINDING_PACKET_PERIOD / 1000); //msec
        tx_state_ = STATE_PRE_BIND;
    } else {
        tx_state_ = STATE_PRE_SEND;
    }
    CLOCK_StartTimer(INIT_WAIT_MS, esky2_tx_callback);
}

//-------------------------------------------------------------------------------------------------
// This function is an ISR called by hardware timer interrupt.
// It works at background to send binding packets or data packets.
// For ESky2, since there is no feedback from the receiver, only two type of flows:
//   STATE_INIT_BINDING -> STATE_BINDING -> STATE_INIT_SENDING -> STATE_SENDING
//   STATE_INIT_SENDING -> STATE_SENDING
//
// For 7E, this module is loaded into RAM, we need to set __attribute__((__long_call__))
//-------------------------------------------------------------------------------------------------
MODULE_CALLTYPE
static u16 esky2_tx_callback()
{
static s32 packet_sent = 0;
static s32 rf_ch_idx = 0;
    switch(tx_state_)
    {
    case STATE_PRE_BIND:
        esky2_bind_init(tx_addr_, packet_);
        tx_state_ = STATE_BINDING;
        packet_sent = 0;
        //Do once, no break needed
    case STATE_BINDING:
        if(packet_sent < BIND_COUNT)
        {
            packet_sent++;
            esky2_send_packet(packet_, NO_RF_CHANNEL_CHANGE);
            return BINDING_PACKET_PERIOD;
        } else {
            //Tell foreground interface binding is done
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
            tx_state_ = STATE_PRE_SEND;
           //Do once, no break needed
        }
    case STATE_PRE_SEND:
            packet_sent = 0;
            esky2_send_init(tx_addr_, packet_);
            rf_ch_idx = 0;
            tx_state_ = STATE_SENDING;
           //Do once, no break needed
    case STATE_SENDING:
        if(packet_sent >= PACKET_SEND_COUNT)
        {
            packet_sent = 0;
            rf_ch_idx++;
            if(rf_ch_idx >= RF_CH_COUNT) rf_ch_idx = 0;
            esky2_update_packet_control_data(packet_, hopping_channels_);
            esky2_send_packet(packet_, hopping_channels_[rf_ch_idx]);
        } else {
            esky2_send_packet(packet_, NO_RF_CHANNEL_CHANGE);
        }
        packet_sent++;
        return SENDING_PACKET_PERIOD;
    } //switch
 
    //Bad things happened, rest
    packet_sent = 0;
    tx_state_ = STATE_PRE_SEND;
    return SENDING_PACKET_PERIOD;
}
 
//-------------------------------------------------------------------------------------------------
// This function setup 24L01
// ESky2 uses one way communication, receiving only. 24L01 RX is never enabled.
//-------------------------------------------------------------------------------------------------
#define CRC_CONFIG (BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO))
 
static void esky2_init(u8 tx_addr[], u8 hopping_ch[])
{
   // Bit vector from bit position
    #define BV(bit) (1 << bit)
 
    esky2_calculate_tx_addr(tx_addr);
    esky2_calculate_frequency_hopping_channels(*((u32*)tx_addr), hopping_ch);
 
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);   // 4-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_SetBitrate(NRF24L01_BR_2M);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);   // bytes of data payload for pipe 0
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, u1_rx_addr, TX_ADDRESS_SIZE);


    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
    // Enable: Dynamic Payload Length, Payload with ACK , W_TX_PAYLOAD_NOACK
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, BV(NRF2401_1D_EN_DPL) | BV(NRF2401_1D_EN_ACK_PAY) | BV(NRF2401_1D_EN_DYN_ACK));

    
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
//        long nul = 0;
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        // It initializes all values needed for the 2Mbps mode 
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\x99\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x08, (u8 *) "\x00\x00\x00\x00", 4);        
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xDF\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
    } else {
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    tx_power_ = Model.tx_power;
    NRF24L01_SetPower(Model.tx_power);
    
    NRF24L01_FlushTx();
    // Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    
}
 
//-------------------------------------------------------------------------------------------------
// This function generates "random" RF hopping frequency channel numbers. From SPI traces the
// original TX always sets for channelx 0x22 and 0x4a - let's leave it to the user what he prefers.
//-------------------------------------------------------------------------------------------------
static void esky2_calculate_frequency_hopping_channels(u32 seed, u8 hopping_channels[])
{
    // Use channels 2..79
    u8 first = 0x22;
    u8 second = 0x4a;
    //if ( Model.proto_opts[PROTOOPTS_FIXEDRF] == FIXEDRF_NO ) {
      first = seed % 37 + 2;
      second = first + 40;
    //}

    hopping_channels[0] = first;
    hopping_channels[1] = second;
    dbgprintf("Using channels %02d and %02d\n", first, second);
}
 
//-------------------------------------------------------------------------------------------------
// This function generate RF TX packet address
// ESky2 can remember the binding parameters; we do not need rebind when power up.
// This requires the address must be repeatable for a specific RF ID at power up.
// The ARM Device ID is used as seed, so controllers with the same RF ID have their
// own specific addresses.
//-------------------------------------------------------------------------------------------------
static void esky2_calculate_tx_addr(u8 tx_addr[])
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

    tx_addr[0] = (lfsr >> 24) & 0xFF;
    tx_addr[1] = (lfsr >> 16) & 0xFF;
    tx_addr[2] = (lfsr >> 8) & 0xFF;
    tx_addr[3] = (lfsr >> 0) & 0xFF;

    dbgprintf("TX ID %02X %02X %02X %02X\n", tx_addr[0], tx_addr[1], tx_addr[2], tx_addr[3]);
}
 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Following functions are called by timer ISR. When a foreground function is preempted by the ISR,
// if the shared data are in the middle of changing, the data lose integrity.
// Related data are:
//   Channels[]     - These are keeping change, may be.
//   Model.tx_power - This is not important, the value will be stable eventually
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//-------------------------------------------------------------------------------------------------
// This function init 24L01 regs and packet data for binding
//
// Send tx address to the ESky2 receiver during binding.
// It seems that ESky2 can remember these parameters, no binding needed after power up.
//
// Bind uses fixed TX address "sstc", 2 Mbps data rate and channel 1
//-------------------------------------------------------------------------------------------------
static void esky2_bind_init(u8 tx_addr[], u8 bind_packet[])
{
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, u1_rx_addr, TX_ADDRESS_SIZE);
    bind_packet[0]  = tx_addr[0];
    bind_packet[1]  = tx_addr[1];
    bind_packet[2]  = tx_addr[2];
    bind_packet[3]  = tx_addr[3]; 
    bind_packet[4]  = u1_rx_addr[0];
    bind_packet[5]  = u1_rx_addr[1];
    bind_packet[6]  = u1_rx_addr[2];
    bind_packet[7]  = u1_rx_addr[3];
    bind_packet[8]  = tx_addr[0];
    bind_packet[9]  = tx_addr[1];
    bind_packet[10] = tx_addr[2];
    bind_packet[11] = tx_addr[3]; 
    bind_packet[12] = 0;
    bind_packet[13] = 0;
    bind_packet[14] = 0;
 
    //Send the first packet
    esky2_send_packet(packet_, 1);
}
 
static void esky2_send_packet(u8 packet[], s32 rf_ch)
{
    if(rf_ch > 0)
    {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, (u8)rf_ch);
    }
    NRF24L01_WritePayload(packet, PAYLOADSIZE);
}
 
//-------------------------------------------------------------------------------------------------
// Initialize 24L01 for sending packets
// If Model.proto_opts[PROTOOPTS_STARTBIND] is not selected, this is the entry point after esky2_init()
//-------------------------------------------------------------------------------------------------
static void esky2_send_init(u8 tx_addr[], u8 packet[])
{
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_addr, TX_ADDRESS_SIZE);
 
    esky2_update_packet_control_data(packet, hopping_channels_);
}
 

//-------------------------------------------------------------------------------------------------
// Update control data to be sent
// Do it once per frequency.
//-------------------------------------------------------------------------------------------------
static void esky2_update_packet_control_data(u8 packet[], u8 hopping_ch[])
{
    u16 throttle, rudder, elevator, aileron, flight_mode, aux_ch6, aux_ch7;
    esky2_read_controls(&throttle, &aileron, &elevator, &rudder, &flight_mode, &aux_ch6, &aux_ch7, &flags);
     
      packet[0]  = hopping_ch[0];
      packet[1]  = hopping_ch[1];
      packet[2]  = ((flight_mode << 6) & 0xC0) | ((aux_ch7 << 4) & 0x30) | ((throttle >> 8) & 0xFF);
      packet[3]  = throttle & 0xFF;
      packet[4]  = ((aux_ch6 >> 4) & 0xF0) | ((aileron >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
      packet[5]  = aileron  & 0xFF;
      packet[6]  = (aux_ch6 & 0xF0) | ((elevator >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
      packet[7]  = elevator & 0xFF;
      packet[8]  = ((aux_ch6 << 4) & 0xF0) | ((rudder >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
      packet[9]  = rudder & 0xFF;
      // The next 4 Bytes are sint8 trim values (TAER). As trims are already included within normal outputs, these values are set to zero.
      packet[10] = 0x00;
      packet[11] = 0x00;
      packet[12] = 0x00;
      packet[13] = 0x00;
      //calculate checksum:
      u8 sum = 0;
      for (int i = 0; i < 14; ++i) sum += packet[i];
      packet[14] = sum;

    if(tx_power_ != (u32)Model.tx_power)
    {
       NRF24L01_SetPower(Model.tx_power);
       tx_power_ = Model.tx_power;
    }
}
 
//-------------------------------------------------------------------------------------------------
// This function is the data interface between the remote control and ESky2 RX,
//-------------------------------------------------------------------------------------------------
static void esky2_read_controls(u16* throttle, u16* aileron, u16* elevator, u16* rudder,
                          u16* flight_mode, u16* aux_ch6, u16* aux_ch7, u8* flags)
{
    (void) flags;
    // Protocol is registered TAERG, that is
    // Throttle is channel 0, Aileron - 1, Elevator - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range

    *throttle    = esky2_convert_channel(CHANNEL1);
    *aileron     = 3000 - esky2_convert_channel(CHANNEL2); //channel is reversed
    *elevator    = esky2_convert_channel(CHANNEL3);
    *rudder      = 3000 - esky2_convert_channel(CHANNEL4); //channel is reversed
    *flight_mode = esky2_convert_2bit_channel(CHANNEL5);
    *aux_ch6     = esky2_convert_channel(CHANNEL6);
    *aux_ch7     = esky2_convert_2bit_channel(CHANNEL7);

    // Print channels every now and then
    if (0) { // (total_packets & 0x3FF) == 1) {
        dbgprintf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        dbgprintf("Aileron %d, elevator %d, throttle %d, rudder %d\n",
               (s16) *aileron, (s16) *elevator, (s16) *throttle, (s16) *rudder);
    }
}
 
//-------------------------------------------------------------------------------------------------
// Convert channel: Channel values are servo time in ms, 1500ms is the middle,
// 1000 and 2000 are min and max values
//-------------------------------------------------------------------------------------------------

static u16 esky2_convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u16) ((ch * 500 / CHAN_MAX_VALUE) + 1500);
}

static u16 esky2_convert_2bit_channel(u8 num)
{
    s32 ch = Channels[num];
    u16 ch_out;
    if (ch >= 0)
    { 
      if (ch >= CHAN_MAX_VALUE/2) ch_out = 3; else ch_out = 2;       
    } else if (ch < CHAN_MIN_VALUE/2) ch_out = 0; else ch_out = 1;       

    if (Model.num_channels <= num) ch_out = 0; //set unused channels to zero, for compatibility with older 4 channel models

    return ch_out; 
}
 
#endif //PROTO_HAS_NRF24L01
