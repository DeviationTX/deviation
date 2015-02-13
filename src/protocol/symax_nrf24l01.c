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

/*
   Main protocol compatible with Syma X5C-1, X11, X11C, X12.
   SymaX5C protocol option compatible with Syma X5C (original) and X2.
*/

#ifdef MODULAR
  //Allows the linker to properly relocate
  #define SYMAX_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

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
#define BIND_COUNT 4.360   // 6 seconds
#define FIRST_PACKET_DELAY  1
#define dbgprintf printf
#else
#define BIND_COUNT 345   // 1.5 seconds
#define FIRST_PACKET_DELAY  12000
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD        4000     // Timeout for callback in uSec
#define INITIAL_WAIT          500

#define FLAG_FLIP      0x01
#define FLAG_VIDEO     0x02
#define FLAG_PICTURE   0x04

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

#define PAYLOADSIZE 10       // receive data pipes set to this size, but unused
#define MAX_PACKET_SIZE 16   // X11,X12,X5C-1 10-byte, X5C 16-byte

static u8 packet[MAX_PACKET_SIZE];
static u8 packet_size;
static u16 counter;
static u32 packet_counter;
static u8 tx_power;
static u8 throttle, rudder, elevator, aileron, flags;
static u8 rx_tx_addr[5];

// frequency channel management
#define MAX_RF_CHANNELS    17
static u8 current_chan;
static u8 chans[MAX_RF_CHANNELS];
static u8 num_rf_channels;

static u8 phase;
enum {
    SYMAX_INIT1 = 0,
    SYMAX_BIND2,
    SYMAX_BIND3,
    SYMAX_DATA
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

static const char * const symax_opts[] = {
  _tr_noop("SymaX5C"),  _tr_noop("Off"), _tr_noop("On"), NULL,
  NULL
};
enum {
    PROTOOPTS_X5C = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);



static u8 checksum(u8 *data)
{
    u8 sum = data[0];

    for (int i=1; i < packet_size-1; i++)
        if (Model.proto_opts[PROTOOPTS_X5C])
            sum += data[i];
        else
            sum ^= data[i];
    
    return sum + (Model.proto_opts[PROTOOPTS_X5C] ? 0 : 0x55);
}


#define BABS(X) (((X) < 0) ? -(u8)(X) : (X))
// Channel values are sign + magnitude 8bit values
static u8 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u8) ((ch < 0 ? 0x80 : 0) | BABS(ch * 127 / CHAN_MAX_VALUE));
}


static void read_controls(u8* throttle, u8* rudder, u8* elevator, u8* aileron, u8* flags)
{
    // Protocol is registered AETRF, that is
    // Aileron is channel 1, Elevator - 2, Throttle - 3, Rudder - 4, Flip control - 5

    *aileron  = convert_channel(CHANNEL1);
    *elevator = convert_channel(CHANNEL2);
    *throttle = convert_channel(CHANNEL3);
    *throttle = *throttle & 0x80 ? 0xff - *throttle : 0x80 + *throttle;
    *rudder   = convert_channel(CHANNEL4);

    // Channel 5
    if (Channels[CHANNEL5] <= 0)
        *flags &= ~FLAG_FLIP;
    else
        *flags |= FLAG_FLIP;

    // Channel 6
    if (Channels[CHANNEL6] <= 0)
        *flags &= ~FLAG_PICTURE;
    else
        *flags |= FLAG_PICTURE;

    // Channel 7
    if (Channels[CHANNEL7] <= 0)
        *flags &= ~FLAG_VIDEO;
    else
        *flags |= FLAG_VIDEO;


//    dbgprintf("ail %5d, ele %5d, thr %5d, rud %5d, flags 0x%x\n",
//            *aileron, *elevator, *throttle, *rudder, *flags);
}


#define X5C_CHAN2TRIM(X) ((((X) & 0x80 ? 0xff - (X) : 0x80 + (X)) >> 2) + 0x20)

static void build_packet_x5c(u8 bind)
{
    if (bind) {
        memset(packet, 0, packet_size);
        packet[7] = 0xae;
        packet[8] = 0xa9;
        packet[14] = 0xc0;
        packet[15] = 0x17;
    } else {
        read_controls(&throttle, &rudder, &elevator, &aileron, &flags);

        packet[0] = throttle;
        packet[1] = rudder;
        packet[2] = elevator ^ 0x80;  // reversed from default
        packet[3] = aileron;
        packet[4] = X5C_CHAN2TRIM(rudder ^ 0x80);     // drive trims for extra control range
        packet[5] = X5C_CHAN2TRIM(elevator);
        packet[6] = X5C_CHAN2TRIM(aileron ^ 0x80);
        packet[7] = 0xae;
        packet[8] = 0xa9;
        packet[9] = 0x00;
        packet[10] = 0x00;
        packet[11] = 0x00;
        packet[12] = 0x00;
        packet[13] = 0x00;
        packet[14] = (flags & FLAG_VIDEO   ? 0x10 : 0x00) 
                   | (flags & FLAG_PICTURE ? 0x08 : 0x00)
                   | (flags & FLAG_FLIP    ? 0x01 : 0x00)
                   | 0x04;  // always high rates (bit 3 is rate control)
        packet[15] = checksum(packet);
    }
}


static void build_packet(u8 bind) {
    if (bind) {
        packet[0] = rx_tx_addr[4];
        packet[1] = rx_tx_addr[3];
        packet[2] = rx_tx_addr[2];
        packet[3] = rx_tx_addr[1];
        packet[4] = rx_tx_addr[0];
        packet[5] = 0xaa;
        packet[6] = 0xaa;
        packet[7] = 0xaa;
        packet[8] = 0x00;
    } else {
        read_controls(&throttle, &rudder, &elevator, &aileron, &flags);

        packet[0] = throttle;
        packet[1] = elevator;
        packet[2] = rudder;
        packet[3] = aileron;
        packet[4] = (flags & FLAG_VIDEO   ? 0x80 : 0x00) 
                  | (flags & FLAG_PICTURE ? 0x40 : 0x00);
        // use trims to extend controls
        packet[5] = (elevator >> 2) | 0xc0;  // always high rates (bit 7 is rate control)
        packet[6] = (rudder >> 2)   | (flags & FLAG_FLIP  ? 0x40 : 0x00);
        packet[7] = aileron >> 2;
        packet[8] = 0x00;
    }
    packet[9] = checksum(packet);
}


static void send_packet(u8 bind)
{
    if (Model.proto_opts[PROTOOPTS_X5C])
        build_packet_x5c(bind);
    else
        build_packet(bind);

    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x2e);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, chans[current_chan]);
    NRF24L01_FlushTx();

    NRF24L01_WritePayload(packet, packet_size);

#ifdef EMULATOR
    dbgprintf("pkt %d: chan 0x%x, bind %d, data %02x", packet_counter, chans[current_chan], bind, packet[0]);
    for(int i=1; i < packet_size; i++) dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif

    if (packet_counter++ % 2) {   // use each channel twice
        current_chan = (current_chan + 1) % num_rf_channels;
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


// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_rx_tx_addr()
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

    rx_tx_addr[4] = 0xa2;
    for (u8 i = 0; i < sizeof(rx_tx_addr)-1; ++i) {
        rx_tx_addr[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }
}

static void symax_init()
{
    const u8 bind_rx_tx_addr[] = {0xab,0xac,0xad,0xae,0xaf};
    const u8 rx_tx_addr_x5c[] = {0x6d,0x6a,0x73,0x73,0x73};   // X5C uses same address for bind and data

    NRF24L01_Initialize();

    NRF24L01_SetTxRxMode(TX_EN);

    NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes (even though not used?)
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xff); // 4mS retransmit t/o, 15 tries (retries w/o AA?)
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);

    if (Model.proto_opts[PROTOOPTS_X5C]) {
      NRF24L01_SetBitrate(NRF24L01_BR_1M);
      packet_size = 16;
    } else {
      NRF24L01_SetBitrate(NRF24L01_BR_250K);
      packet_size = 10;
    }

    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_08_OBSERVE_TX, 0x00);
    NRF24L01_WriteReg(NRF24L01_09_CD, 0x00);
    NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3); // LSB byte of pipe 2 receive address
    NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
    NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
    NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);   // bytes of data payload for pipe 1
    NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here

    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,
                                Model.proto_opts[PROTOOPTS_X5C] ? rx_tx_addr_x5c : bind_rx_tx_addr,
                                5);

    NRF24L01_ReadReg(NRF24L01_07_STATUS);

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
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xF9\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x06, (u8 *) "\x00\x00\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x07, (u8 *) "\x00\x00\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x08, (u8 *) "\x00\x00\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x09, (u8 *) "\x00\x00\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0A, (u8 *) "\x00\x00\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0B, (u8 *) "\x00\x00\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xFF\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xF9\x96\x82\x1B", 4);
    } else {
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    NRF24L01_FlushTx();
    NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x0e);
    NRF24L01_ReadReg(NRF24L01_00_CONFIG); 
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0c); 
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0e);  // power on
}

static void symax_init1()
{
    // write a strange first packet to RF channel 8 ...
    u8 first_packet[] = {0xf9, 0x96, 0x82, 0x1b, 0x20, 0x08, 0x08, 0xf2, 0x7d, 0xef, 0xff, 0x00, 0x00, 0x00, 0x00};
    u8 chans_bind[] = {0x4b, 0x30, 0x40, 0x2e};
    u8 chans_bind_x5c[] = {0x27, 0x1b, 0x39, 0x28, 0x24, 0x22, 0x2e, 0x36,
                           0x19, 0x21, 0x29, 0x14, 0x1e, 0x12, 0x2d, 0x18};

    NRF24L01_FlushTx();
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);
    NRF24L01_WritePayload(first_packet, 15);

    if (Model.proto_opts[PROTOOPTS_X5C]) {
      num_rf_channels = sizeof(chans_bind_x5c);
      memcpy(chans, chans_bind_x5c, num_rf_channels);
    } else {
      initialize_rx_tx_addr();   // make info available for bind packets
      num_rf_channels = sizeof(chans_bind);
      memcpy(chans, chans_bind, num_rf_channels);
    }
    current_chan = 0;
    packet_counter = 0;
}


// channels determined by last byte of tx address
static void set_channels(u8 address) {
  static const u8 start_chans_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
  static const u8 start_chans_2[] = {0x2a, 0x0a, 0x42, 0x22};
  static const u8 start_chans_3[] = {0x1a, 0x3a, 0x12, 0x32};

  u8 laddress = address & 0x1f;
  u8 i;
  u32 *pchans = (u32 *)chans;   // avoid compiler warning

  num_rf_channels = 4;

  if (laddress < 0x10) {
    if (laddress == 6) laddress = 7;
    for(i=0; i < num_rf_channels; i++) {
      chans[i] = start_chans_1[i] + laddress;
    }
  } else if (laddress < 0x18) {
    for(i=0; i < num_rf_channels; i++) {
      chans[i] = start_chans_2[i] + (laddress & 0x07);
    }
    if (laddress == 0x16) {
      chans[0] += 1;
      chans[1] += 1;
    }
  } else if (laddress < 0x1e) {
    for(i=0; i < num_rf_channels; i++) {
      chans[i] = start_chans_3[i] + (laddress & 0x07);
    }
  } else if (laddress == 0x1e) {
      *pchans = 0x38184121;
  } else {
      *pchans = 0x39194121;
  }
}


static void symax_init2()
{
    u8 chans_data_x5c[] = {0x1d, 0x2f, 0x26, 0x3d, 0x15, 0x2b, 0x25, 0x24,
                           0x27, 0x2c, 0x1c, 0x3e, 0x39, 0x2d, 0x22};

    if (Model.proto_opts[PROTOOPTS_X5C]) {
      num_rf_channels = sizeof(chans_data_x5c);
      memcpy(chans, chans_data_x5c, num_rf_channels);
    } else {
      set_channels(rx_tx_addr[0]);
      NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
    }
    current_chan = 0;
    packet_counter = 0;
}

MODULE_CALLTYPE
static u16 symax_callback()
{
    switch (phase) {
    case SYMAX_INIT1:
        symax_init1();
        phase = SYMAX_BIND2;
        return FIRST_PACKET_DELAY;
        break;

    case SYMAX_BIND2:
        counter = BIND_COUNT;
        phase = SYMAX_BIND3;
        send_packet(1);
        break;

    case SYMAX_BIND3:
        if (counter == 0) {
            symax_init2();
            phase = SYMAX_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case SYMAX_DATA:
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;
}



static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_counter = 0;
    flags = 0;

    symax_init();
    phase = SYMAX_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, symax_callback);
}

const void *SYMAX_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 8L; // A, E, T, R, special controls
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return symax_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;

        default: break;
    }
    return 0;
}
#endif

