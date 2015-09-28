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
  #define KN_Cmds PROTO_Cmds
  #pragma long_calls
#endif

//================================================================================================
// Definitions
//================================================================================================
#define LIMIT(x, hi, lo) do{if((x)>(hi)) (x)=(hi); else if((x)<(lo)) (x)=(lo);} while(0)

// Wait for RX chip stable - 10ms
#define INIT_WAIT_MS  10000

//Payload(16 bytes) plus overhead(10 bytes) is 208 bits, takes about 0.4ms or 0.2ms
//to send for the rate of 500kb/s and 1Mb/s respectively.

// Callback timeout period for sending bind packets, minimum 250
#define BINDING_PACKET_PERIOD  1000

// Timeout for sending data packets, in uSec, KN protocol requires 2ms
#define WL_SENDING_PACKET_PERIOD  2000
// Timeout for sending data packets, in uSec, KNFX protocol requires 1.2 ms
#define FX_SENDING_PACKET_PERIOD  1200


// packets to be sent during binding, last 0.5 seconds in WL Toys and 0.2 seconds in Feilun
#ifdef EMULATOR
#define USE_FIXED_MFGID
#define WL_BIND_COUNT 5
#define FX_BIND_COUNT 5
#else
#define WL_BIND_COUNT 500
#define FX_BIND_COUNT 200 
#endif 



#define PAYLOADSIZE 16

//24L01 has 126 RF channels, can we use all of them?
#define MAX_RF_CHANNEL 73

//KN protocol for WL Toys changes RF frequency every 10 ms, repeats with only 4 channels.
//Feilun variant uses only 2 channels, so we will just repeat the hopping channels later
#define RF_CH_COUNT 4

//KN protocol for WL Toys sends 4 data packets every 2ms per frequency, plus a 2ms gap.
#define WL_PACKET_SEND_COUNT 5
//KN protocol for Feilun sends 8 data packets every 1.2ms per frequency, plus a 0.3ms gap.
#define FX_PACKET_SEND_COUNT 8
 
#define TX_ADDRESS_SIZE 5

#define NO_RF_CHANNEL_CHANGE -1

typedef int BOOL;
 
typedef struct {
    u8 dr       :1;  //1 - full range
    u8 th_hld   :1;  //1 - hold
    u8 mod_3d   :1;  //1 - 3D
    u8 rsvd     :3;
    u8 mod_3axil:1;  //0 - 6axil,  1 - 3axis
    u8 rsvd1    :1;
} ContorlFlags_S;
 
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
    CHANNEL11
};
 
enum {
    PROTOOPTS_STARTBIND = 0,
    PROTOOPTS_USE1MBPS,
    PROTOOPTS_FORMAT,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);
 
enum {
    STARTBIND_NO  = 0,
    STARTBIND_YES = 1,
};
 
enum {
    USE1MBPS_NO  = 0,
    USE1MBPS_YES = 1,
};

enum {
    FORMAT_WLTOYS = 0,
    FORMAT_FEILUN,
};
 
enum {
    STATE_PRE_BIND,
    STATE_BINDING,
    STATE_PRE_SEND,
    STATE_SENDING,
};
//================================================================================================
// Private Function Prototypes
//================================================================================================
static void kn_start_tx(BOOL bind_yes);
static u16  kn_tx_callback();
 
static void kn_init(u8 tx_addr[], u8 hopping_ch[]);
 
static void kn_bind_init(u8 tx_addr[], u8 hopping_ch[], u8 bind_packet[]);
 
static void kn_calculate_freqency_hopping_channels(u32 seed, u8 hopping_channels[], u8 tx_addr[]);
static void kn_calculate_tx_addr(u8 tx_addr[]);
static void kn_send_packet(u8 packet[], s32 rf_ch);
 
static void kn_send_init(u8 tx_addr[], u8 packet[]);
static void kn_update_packet_control_data(u8 packet[], s32 packet_count, s32 rf_ch_idx);
static void kn_update_packet_send_count(u8 packet[], s32 packet_sent, s32 rf_ch_idx );
 
static void kn_read_controls(u16* throttle, u16* aileron, u16* elevator, u16* rudder, ContorlFlags_S* flags);
static u16  kn_convert_channel(u8 num);
 
//================================================================================================
// Local Variables
//================================================================================================
static const char * const KN_PROTOCOL_OPTIONS[] = {
  _tr_noop("Re-bind"),  _tr_noop("No"), _tr_noop("Yes"), NULL,
  "1Mbps",  _tr_noop("No"), _tr_noop("Yes"), NULL,
  _tr_noop("Format"), "WLToys", "Feilun", NULL, 
  NULL
};
 
//Payload data buffer
static u8 packet_[PAYLOADSIZE];
static u8 hopping_channels_[RF_CH_COUNT];

static u16 sending_packet_period;
static u16 bind_count;
static u8 packet_send_count;
 
//KN only use 4 of 5 bytes address, the 5th byte has fixed value 'K'
static u8 tx_addr_[TX_ADDRESS_SIZE];
 
static u32 tx_state_ = STATE_PRE_BIND;
static u32 tx_power_;
 
//================================================================================================
// Public Functions
//================================================================================================
const void *KN_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:
        	kn_start_tx((Model.proto_opts[PROTOOPTS_STARTBIND] == STARTBIND_YES));
            return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)((NRF24L01_Reset()) ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND:
            return (void *)0L; // Never Autobind
        case PROTOCMD_BIND:
        	kn_start_tx(STARTBIND_YES);
			return 0;
        case PROTOCMD_NUMCHAN:
            return (void *)11L; // T, R, E, A, DR, TH, Flight mode, 6G, trim T, R, E and V977 side DR
        case PROTOCMD_DEFAULT_NUMCHAN:
            return (void *)11L;
        case PROTOCMD_CURRENT_ID: 
            return (Model.fixed_id) ? (void *)Model.fixed_id : 0;
        case PROTOCMD_GETOPTIONS: 
            return KN_PROTOCOL_OPTIONS;
        case PROTOCMD_TELEMETRYSTATE: 
            return (void *)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
 
//================================================================================================
// Private Functions
//================================================================================================
static void kn_start_tx(BOOL bind_yes)
{
    CLOCK_StopTimer();
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
       case FORMAT_WLTOYS:
            sending_packet_period = WL_SENDING_PACKET_PERIOD;
            bind_count = WL_BIND_COUNT;
            packet_send_count = WL_PACKET_SEND_COUNT;
            printf("KN protocol: WL Toys\n");
            break;
        
       case FORMAT_FEILUN:
            sending_packet_period = FX_SENDING_PACKET_PERIOD;
            bind_count =FX_BIND_COUNT;
            packet_send_count = FX_PACKET_SEND_COUNT;
            printf("KN protocol: Feilun\n");
            break;
    }
 
    kn_init(tx_addr_, hopping_channels_);
    if(bind_yes)
    {
        PROTOCOL_SetBindState(bind_count * BINDING_PACKET_PERIOD / 1000); //msec
        tx_state_ = STATE_PRE_BIND;
    } else {
        tx_state_ = STATE_PRE_SEND;
    }
    CLOCK_StartTimer(INIT_WAIT_MS, kn_tx_callback);
}

//-------------------------------------------------------------------------------------------------
// This function is an ISR called by hardware timer interrupt.
// It works at background to send binding packets or data packets.
// For KN, since there is no feedback from the receiver, only two type of flows:
//   STATE_INIT_BINDING -> STATE_BINDING -> STATE_INIT_SENDING -> STATE_SENDING
//   STATE_INIT_SENDING -> STATE_SENDING
//
// For 7E, this module is loaded into RAM, we need to set __attribute__((__long_call__))
//-------------------------------------------------------------------------------------------------
MODULE_CALLTYPE
static u16 kn_tx_callback()
{
static s32 packet_sent = 0;
static s32 rf_ch_idx = 0;
    switch(tx_state_)
    {
    case STATE_PRE_BIND:
        kn_bind_init(tx_addr_, hopping_channels_, packet_);
        tx_state_ = STATE_BINDING;
        packet_sent = 0;
        //Do once, no break needed
    case STATE_BINDING:
        if(packet_sent < bind_count)
        {
            packet_sent++;
            kn_send_packet(packet_, NO_RF_CHANNEL_CHANGE);
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
            kn_send_init(tx_addr_, packet_);
            rf_ch_idx = 0;
            tx_state_ = STATE_SENDING;
           //Do once, no break needed
    case STATE_SENDING:
        if(packet_sent >= packet_send_count)
        {
            packet_sent = 0;
            rf_ch_idx++;
            if(rf_ch_idx >= RF_CH_COUNT) rf_ch_idx = 0;
            kn_update_packet_control_data(packet_, 0, rf_ch_idx);
            kn_send_packet(packet_, hopping_channels_[rf_ch_idx]);
        } else {
            kn_update_packet_send_count(packet_, packet_sent, rf_ch_idx);
            kn_send_packet(packet_, NO_RF_CHANNEL_CHANGE);
        }
        packet_sent++;
        return sending_packet_period;
    } //switch
 
    //Bad things happened, rest
    packet_sent = 0;
    tx_state_ = STATE_PRE_SEND;
    return sending_packet_period;
}
 
//-------------------------------------------------------------------------------------------------
// This function setup 24L01
// V977 uses one way communication, receiving only. 24L01 RX is never enabled.
// V977 needs payload length in the packet. We should configure 24L01 to enable Packet Control Field(PCF)
//   Some RX reg settings are actually for enable PCF
// 
//-------------------------------------------------------------------------------------------------
 
static void kn_init(u8 tx_addr[], u8 hopping_ch[])
{
   // Bit vector from bit position
    #define BV(bit) (1 << bit)
 
    kn_calculate_tx_addr(tx_addr);
    //use tx_addr as seed to randomize the RF hopping channels or to calculate channels for Feilun variant
    kn_calculate_freqency_hopping_channels(*((u32*)tx_addr), hopping_ch, tx_addr);
 
    NRF24L01_Initialize();
 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 0x00);   // bytes of RX data payload for pipe 0. Any value is ok, we do not use RX anyway
 
    NRF24L01_Activate(0x73);
    //As PTX that transmits to a PRX with DPL enabled, DPL_P0 must be set
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
 
    // Enable: Dynamic Payload Length to enable PCF
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, BV(NRF2401_1D_EN_DPL)); 
 
    tx_power_ = Model.tx_power;
    NRF24L01_SetPower(Model.tx_power);
 
    NRF24L01_FlushTx();
    // Turn radio power on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
}
 
//-------------------------------------------------------------------------------------------------
// This function generates "random" RF hopping frequency channel numbers.
// These numbers must be repeatable for a specific seed
// The generated number range is from 0 to MAX_RF_CHANNEL. No repeat or adjacent numbers
//
// For Feilun variant, the channels are calculated from TXID, and since only 2 channels are used
// we copy them to fill up to MAX_RF_CHANNEL
//-------------------------------------------------------------------------------------------------
static void kn_calculate_freqency_hopping_channels(u32 seed, u8 hopping_channels[], u8 tx_addr[])
{
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
       case FORMAT_WLTOYS:;
            s32 idx = 0;
            u32 rnd = seed;
            while (idx < RF_CH_COUNT) {
              s32 i;
              rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization
              // Drop least-significant byte for better randomization. Start from 1
              u8 next_ch = (rnd >> 8) % MAX_RF_CHANNEL + 1;
              // Check that it's not duplicate nor adjacent
              for (i = 0; i < idx; i++) {
        	  u8 ch = hopping_channels[i];
                  if( (ch <= next_ch + 1) && (ch >= next_ch - 1) ) break;
              }
              if (i != idx)
              continue;
              hopping_channels[idx++] = next_ch;
            }
            break;
        
       case FORMAT_FEILUN:
            hopping_channels[0] = tx_addr[0] + tx_addr[1] + tx_addr[2] + tx_addr[3] - 256; 
            hopping_channels[1] = hopping_channels[0] + 32;
            hopping_channels[2] = hopping_channels[0];
            hopping_channels[3] = hopping_channels[1];
            break;

    }
    printf("FQ ch %02u %02u %02u %02u\n", hopping_channels[0], hopping_channels[1], hopping_channels[2], hopping_channels[3]);

}
 
//-------------------------------------------------------------------------------------------------
// This function generate RF TX packet address
// V977 can remember the binding parameters; we do not need rebind when power up.
// This requires the address must be repeatable for a specific RF ID at power up.
// The ARM Device ID is used as seed, so controllers with the same RF ID have their
// own specific addresses.
//-------------------------------------------------------------------------------------------------
static void kn_calculate_tx_addr(u8 tx_addr[])
{
    u32 rnd;
    s32 i;
    MCU_SerialNumber((u8*)&rnd, 4);
    if(Model.fixed_id != 0) rnd *= Model.fixed_id;
    for (i=0; i<8; i++) {
      rnd = (rnd * 0x3eeb2c54a2f) + 0xE0F3AD019660;
    }

    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
       case FORMAT_WLTOYS:
            *((u32*)tx_addr) = rnd;
            break;
        
       case FORMAT_FEILUN:
            // Generate TXID with sum of minimum 256 and maximum 256+MAX_RF_CHANNEL-32
            tx_addr[0] = rnd % 256;
            tx_addr[1] = 1 + rnd % (MAX_RF_CHANNEL-33);
            rnd = (rnd * 0x3eeb2c54a2f) + 0xE0F3AD019660;
            if (tx_addr[0] + tx_addr[1] < 256)
              tx_addr[2] = 1 + rnd % (MAX_RF_CHANNEL-33);
            else
              tx_addr[2] = 0x00;
            tx_addr[3] = 0x00;
             while((tx_addr[0] + tx_addr[1] + tx_addr[2] + tx_addr[3])<257) tx_addr[3] = tx_addr[3] + 1 + (rnd % (MAX_RF_CHANNEL-33));
            break;

    }
    //The 5th byte is a constant, must be 'K'
    tx_addr[4] = 'K';
    printf("TX ID %02X %02X %02X %02X\n", tx_addr[0], tx_addr[1], tx_addr[2], tx_addr[3]);
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
// Send tx address, hopping table (for Wl Toys), and data rate to the KN receiver during binding.
// It seems that KN can remember these parameters, no binding needed after power up.
//
// Bind uses fixed TX address "KNDZK", 1 Mbps data rate and channel 83
//-------------------------------------------------------------------------------------------------
static void kn_bind_init(u8 tx_addr[], u8 hopping_ch[], u8 bind_packet[])
{
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (const u8*)"KNDZK", 5U);
    bind_packet[0]  = 'K';
    bind_packet[1]  = 'N';
    bind_packet[2]  = 'D';
    bind_packet[3]  = 'Z';
    //Use first four bytes of tx_addr
    bind_packet[4]  = tx_addr[0];
    bind_packet[5]  = tx_addr[1];
    bind_packet[6]  = tx_addr[2];
    bind_packet[7]  = tx_addr[3];
 
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
       case FORMAT_WLTOYS:
            bind_packet[8]  = hopping_ch[0];
            bind_packet[9]  = hopping_ch[1];
            bind_packet[10] = hopping_ch[2];
            bind_packet[11] = hopping_ch[3];
            break;

       case FORMAT_FEILUN:
            bind_packet[8]  = 0x00;
            bind_packet[9]  = 0x00;
            bind_packet[10] = 0x00;
            bind_packet[11] = 0x00;
            break;
    }
    bind_packet[12] = 0x00;
    bind_packet[13] = 0x00;
    bind_packet[14] = 0x00;
    bind_packet[15] = (Model.proto_opts[PROTOOPTS_USE1MBPS] == USE1MBPS_YES) ? 0x01 : 0x00;
 
    //Set address and RF channel and send the first packet
    kn_send_packet(packet_, 83);
}
 
static void kn_send_packet(u8 packet[], s32 rf_ch)
{
    if(rf_ch > 0)
    {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, (u8)rf_ch);
    }
    // Binding and sending period are 1 ms and 2 ms, the TX FIFO is guaranteed empty.
    NRF24L01_WritePayload(packet, PAYLOADSIZE);
}
 
//-------------------------------------------------------------------------------------------------
// Initialize 24L01 for sending packets
// If Model.proto_opts[PROTOOPTS_STARTBIND] is not selected, this is the entry point after kn_init()
//-------------------------------------------------------------------------------------------------
static void kn_send_init(u8 tx_addr[], u8 packet[])
{
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_addr, TX_ADDRESS_SIZE);
     
    u8 bit_rate = (Model.proto_opts[PROTOOPTS_USE1MBPS] == USE1MBPS_YES) ? NRF24L01_BR_1M : NRF24L01_BR_250K;
    NRF24L01_SetBitrate(bit_rate);
 
    kn_update_packet_control_data(packet, 0, 0);
}
 
//-------------------------------------------------------------------------------------------------
// Update sending count and RF channel index.
// The protocol sends 4 data packets every 2ms per frequency, plus a 2ms gap.
// Each data packet need a packet number and RF channel index
// 
// 
//-------------------------------------------------------------------------------------------------
static void kn_update_packet_send_count(u8 packet[], s32 packet_sent, s32 rf_ch_idx )
{
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
       case FORMAT_WLTOYS:
            packet[13] = (packet_sent << 5) | (rf_ch_idx << 2);
            break;

       case FORMAT_FEILUN:
            packet[13] = 0x00;
            break;
    }

}

//-------------------------------------------------------------------------------------------------
// Update control data to be sent
// Do it once per frequency, so the same values will be sent 4 times
// KN uses 4 10-bit data channels plus a 8-bit switch channel
//
// The packet[0] is used for pitch/throttle, the relation is hard coded, not changeable.
// We can change the throttle/pitch range though.
//
// How to use trim? V977 stock controller can trim 6-axis mode to eliminate the draft.
//-------------------------------------------------------------------------------------------------
static void kn_update_packet_control_data(u8 packet[], s32 packet_count, s32 rf_ch_idx)
{
    u16 throttle, aileron, elevator, rudder;
    ContorlFlags_S flags;
    kn_read_controls(&throttle, &aileron, &elevator, &rudder, &flags);
     
    packet[0]  = (throttle >> 8) & 0xFF;
    packet[1]  = throttle & 0xFF;
    packet[2]  = (aileron >> 8) & 0xFF;
    packet[3]  = aileron  & 0xFF;
    packet[4]  = (elevator >> 8) & 0xFF;
    packet[5]  = elevator & 0xFF;
    packet[6]  = (rudder >> 8) & 0xFF;
    packet[7]  = rudder & 0xFF;
    // Trims, middle is 0x64 (100) range 0-200
    packet[8]  = Channels[CHANNEL9] /101 + 100; // 0x64; // T
    packet[9]  = Channels[CHANNEL10] /101 + 100; // 0x64; // A
    packet[10] = Channels[CHANNEL11] /101 + 100; // 0x64; // E
    packet[11] = 0x64; // R
 
    packet[12] = *((u8*)&flags);
    
    switch( Model.proto_opts[PROTOOPTS_FORMAT]) {
       case FORMAT_WLTOYS:
            packet[13] = (packet_count << 5) | (rf_ch_idx << 2);
            break;

       case FORMAT_FEILUN:
            packet[13] = 0x00;
            break;
    }

    packet[14] = 0x00;
    packet[15] = 0x00;
 
    if(tx_power_ != (u32)Model.tx_power)
    {
       NRF24L01_SetPower(Model.tx_power);
       tx_power_ = Model.tx_power;
    }
}
 
//-------------------------------------------------------------------------------------------------
// This function is the data interface between the remote control and V977 RX,
// it converts values of the control channels into the format accepted by V977 RX
//
// Unlike other helis, V977 has a hard coded throttle pitch relation. 
// In normal mode, we can only control throttle, the pitch is set by V977. 
// In 3D mode, the throttle is fixed, we can only control the pitch.
//
// V977 RX data model (output):
// 4 10-bit analog channels registered as TAER (Thro/Pit, Ail, Ele, Rud), 0 to 1023
// 1 8-bit digital channel (4 bit are used) for Throttle hold, DR, 3D mode and 6Axis/3Axis mode
//
// We uses 11 channels as inputs:
// ch1 to 4: T,A,E,R, -10000 to + 10000
// ch5: DR
// ch6: Throttle hold
// ch7: Flight Mode = -10000: 3 Axis normal, 10000: 3 Axis 3D
// ch8: 6G
// ch9 - ch11: for the trims of Thro/Pit, Ail, Ele. 
//-------------------------------------------------------------------------------------------------
static void kn_read_controls(u16* throttle, u16* aileron, u16* elevator, u16* rudder, ContorlFlags_S* flags)
{
    *throttle = kn_convert_channel(CHANNEL1);
    *aileron  = kn_convert_channel(CHANNEL2);
    *elevator = kn_convert_channel(CHANNEL3);
    *rudder   = kn_convert_channel(CHANNEL4);
 
    if (Channels[CHANNEL5] <= 0) flags->dr = 0;
    else flags->dr = 1;

    if (Channels[CHANNEL6] <= 0) flags->th_hld = 0;
    else flags->th_hld = 1;

    if (Channels[CHANNEL7] <= 0) flags->mod_3d = 0;
    else flags->mod_3d = 1;

    if (Channels[CHANNEL8] <= 0) flags->mod_3axil = 0;
    else flags->mod_3axil = 1;
}
 
//-------------------------------------------------------------------------------------------------
// Convert +/-10000 to 0 - 1023 with floor and ceiling
//-------------------------------------------------------------------------------------------------

static u16 kn_convert_channel(u8 num)
{
    s16 ch = Channels[num];
    LIMIT(ch, CHAN_MAX_VALUE, CHAN_MIN_VALUE);
    return (u16) ((ch * 511 / CHAN_MAX_VALUE) + 512);
}
 
#endif //PROTO_HAS_NRF24L01
