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
  #define CX10_Cmds PROTO_Cmds
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
#define dbgprintf printf
#else
#define BIND_COUNT 4360   // 6 seconds
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define PACKET_PERIOD   1316     // Timeout for callback in uSec
#define INITIAL_WAIT     500

#define FLAG_FLIP   0x0100
#define FLAG_LIGHT  0x0200

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

#define PAYLOADSIZE 15   // receive data pipes set to this size, but unused
#define PACKET_SIZE 15   // CX10 packets have 15-byte payload

static u8 packet[PACKET_SIZE];
static u16 counter;
static u32 packet_counter;
static u8 tx_power;
static u16 throttle, rudder, elevator, aileron, flags;
static u8 rx_tx_addr[] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc};

// frequency channel management
#define RF_BIND_CHANNEL 0x02
#define NUM_RF_CHANNELS    4
static u8 current_chan = 0;
static u8 chans[] = {0x16, 0x33, 0x40, 0x0e};

static u8 phase;
enum {
    CX10_INIT1 = 0,
    CX10_BIND2,
    CX10_DATA
};


// Bit vector from bit position
#define BV(bit) (1 << bit)

// XN297 emulation layer
static int xn297_addr_len;
static u8  xn297_tx_addr[5];
static u8  xn297_rx_addr[5];
static u8  is_xn297 = 0;
static const uint8_t xn297_scramble[] = {
  0xe3, 0xb1, 0x4b, 0xea, 0x85, 0xbc, 0xe5, 0x66,
  0x0d, 0xae, 0x8c, 0x88, 0x12, 0x69, 0xee, 0x1f,
  0xc7, 0x62, 0x97, 0xd5, 0x0b, 0x79, 0xca, 0xcc,
  0x1b, 0x5d, 0x19, 0x10, 0x24, 0xd3, 0xdc, 0x3f,
  0x8e, 0xc5, 0x2f};

static uint8_t bit_reverse(uint8_t b_in)
{
  uint8_t b_out = 0;
  for (int i = 0; i < 8; ++i) {
    b_out = (b_out << 1) | (b_in & 1);
    b_in >>= 1;
  }
  return b_out;
}


static const uint16_t polynomial = 0x1021;
static const uint16_t initial    = 0xb5d2;
static const uint16_t xorout     = 0x9ba7;
static uint16_t crc16_update(uint16_t crc, unsigned char a)
{
  crc ^= a << 8;
  for (int i = 0; i < 8; ++i) {
    if (crc & 0x8000) {
      crc = (crc << 1) ^ polynomial;
    } else {
      crc = crc << 1;
    }
  }
  return crc;
}


void XN297_SetTXAddr(uint8_t* addr, int len)
{
  if (len > 5) len = 5;
  if (len < 3) len = 3;
  if (is_xn297) {
    uint8_t buf[] = { 0, 0, 0, 0, 0 };
    memcpy(buf, addr, len);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, addr, 5);
  } else {
    uint8_t buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 }; // bytes for XN297 preamble 0xC710F55 (28 bit)
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 2);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, buf, 5);
    // Receive address is complicated. We need to use scrambled actual address as a receive address
    // but the TX code now assumes fixed 4-byte transmit address for preamble. We need to adjust it
    // first. Also, if the scrambled address begings with 1 nRF24 will look for preamble byte 0xAA
    // instead of 0x55 to ensure enough 0-1 transitions to tune the receiver. Still need to experiment
    // with receiving signals.
    xn297_addr_len = len;
    memcpy(xn297_tx_addr, addr, len);
  }
}


void XN297_WritePayload(uint8_t* msg, int len)
{
  uint8_t packet[32];
  if (is_xn297) {
    NRF24L01_WritePayload(msg, len);
  } else {
    for (int i = 0; i < xn297_addr_len; ++i) {
      packet[i] = xn297_tx_addr[xn297_addr_len-i-1] ^ xn297_scramble[i];
    }

    for (int i = 0; i < len; ++i) {
      // bit-reverse bytes in packet
      uint8_t b_out = bit_reverse(msg[i]);
      packet[xn297_addr_len+i] = b_out ^ xn297_scramble[xn297_addr_len+i];
    }
    int crc_ind = xn297_addr_len + len;
    uint16_t crc = initial;
    for (int i = 0; i < crc_ind; ++i) {
      crc = crc16_update(crc, packet[i]);
    }
    crc ^= xorout;
    packet[crc_ind++] = crc >> 8;
    packet[crc_ind++] = crc & 0xff;
    NRF24L01_WritePayload(packet, crc_ind);
  }
}

// End of XN297 emulation




// Channel values are servo time in ms, 1500ms is the middle,
// 1000 and 2000 are min and max values
static u16 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u16) ((ch * 500 / CHAN_MAX_VALUE) + 1500);
}


static void read_controls(u16* throttle, u16* rudder, u16* elevator, u16* aileron, u16* flags)
{
    // Protocol is registered AETRF, that is
    // Aileron is channel 1, Elevator - 2, Throttle - 3, Rudder - 4, Flip control - 5

    *aileron  = convert_channel(CHANNEL1);
    *elevator = convert_channel(CHANNEL2);
    *throttle = convert_channel(CHANNEL3);
    *rudder   = convert_channel(CHANNEL4);

    // Channel 5
    if (Channels[CHANNEL5] <= 0)
      *flags &= ~FLAG_FLIP;
    else
      *flags |= FLAG_FLIP;

    // Channel 6
    if (Channels[CHANNEL6] <= 0)
      *flags &= ~FLAG_LIGHT;
    else
      *flags |= FLAG_LIGHT;


    dbgprintf("ail %5d, ele %5d, thr %5d, rud %5d, flags 0x%x\n",
            *aileron, *elevator, *throttle, *rudder, *flags);
}


static void send_packet(u8 bind)
{
    if (bind) {
      packet[0]= 0xaa;
    } else {
      packet[0]= 0x55;
    }
    packet[1] = 0x0b;
    packet[2] = 0x06;
    packet[3] = 0x34;
    packet[4] = 0x12;

    read_controls(&throttle, &rudder, &elevator, &aileron, &flags);
    packet[5] = aileron & 0xff;
    packet[6] = (aileron >> 8) & 0xff;
    packet[7] = elevator & 0xff;
    packet[8] = (elevator >> 8) & 0xff;
    packet[9] = throttle & 0xff;
    packet[10] = (throttle >> 8) & 0xff;
    packet[11] = rudder & 0xff;
    packet[12] = (rudder >> 8) & 0xff;
    packet[13] = (flags >> 8) & 0xff;
    packet[14] = flags & 0xff;

    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0e);      // Power on, TX mode, 2byte CRC
    if (bind) {
      NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
      NRF24L01_WriteReg(NRF24L01_05_RF_CH, chans[current_chan++]);
      current_chan %= NUM_RF_CHANNELS;
    }
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, PACKET_SIZE);

    ++packet_counter;

//    radio.ce(HIGH);
//    delayMicroseconds(15);
    // It saves power to turn off radio after the transmission,
    // so as long as we have pins to do so, it is wise to turn
    // it back.
//    radio.ce(LOW);

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

static void cx10_init()
{
    NRF24L01_Initialize();

    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5); 
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7); 
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7); 


    XN297_SetTXAddr(rx_tx_addr, 5);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(Model.tx_power);

// This one should be emulated if we need to receive data as well
//    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);   // bytes of data payload for pipe 1

    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on


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

    // Implicit delay in callback
    // delay(50);
}


MODULE_CALLTYPE
static u16 cx10_callback()
{
    switch (phase) {
    case CX10_INIT1:
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = CX10_BIND2;
        break;

    case CX10_BIND2:
        if (counter == 0) {
            phase = CX10_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        } else {
            send_packet(1);
            counter -= 1;
        }
        break;

    case CX10_DATA:
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;
}


#if 0
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

    set_rx_tx_addr(lfsr);
}
#endif


static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
//    initialize_rx_tx_addr();
    packet_counter = 0;
    counter = BIND_COUNT;
    flags = 0;

    cx10_init();
    phase = CX10_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, cx10_callback);
}

const void *CX10_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 6L; // A, E, T, R, enable flip, enable light
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif

