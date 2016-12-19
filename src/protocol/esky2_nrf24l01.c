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
  #define ESKY_NEW_Cmds PROTO_Cmds
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
  #define BIND_COUNT 10
  #define PACKET_PERIOD    450
  #define dbgprintf printf
#else
  #define BIND_COUNT 1000
  // Timeout for callback in uSec, 4.8ms=4800us for new ESky protocol
  #define PACKET_PERIOD 4800
  #define PACKET_CHKTIME  100 // Time to wait for packet to be sent (no ACK, so very short)
  #define dbgprintf if(0) printf  //do not use the printf, to save a lot of memory and don't have int problems.
#endif


#define PAYLOADSIZE 15
#define NFREQCHANNELS 2
#define TXID_SIZE 4
#define ADDR_SIZE 4



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


static u8 packet[PAYLOADSIZE];
static u8 packet_sent;
static u8 tx_id[TXID_SIZE];
static u8 rf_ch_num;
static u8 packet_count;
// static u16 bind_counter;
static u32 total_packets;
static u8 tx_power;
static u16 throttle, rudder, elevator, aileron, flight_mode, aux_ch6, aux_ch7;
static u8 flags;
static u8 hopping_frequency[NFREQCHANNELS];


static const u8 u1_rx_addr[ADDR_SIZE] = { 0x73, 0x73, 0x74, 0x63 }; //This RX address "sstc" is fixed for the new ESKY-Protocol

//
static u8 phase;
enum {
    ESKY_NEW_INIT2 = 0,
    ESKY_NEW_DATA
};



// Bit vector from bit position
#define BV(bit) (1 << bit)

// Packet ack status values
enum {
    PKT_PENDING = 0,
    PKT_ACKED,
    PKT_TIMEOUT
};

static u8 packet_ack()
{
    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
    case BV(NRF24L01_07_TX_DS):
        return PKT_ACKED;
    case BV(NRF24L01_07_MAX_RT):
        return PKT_TIMEOUT;
    }
    return PKT_PENDING;
}

// 2-bytes CRC
#define CRC_CONFIG (BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO))

static u16 esky_new_init()
{
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, ADDR_SIZE-2);   // 4-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_SetBitrate(NRF24L01_BR_2M);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, u1_rx_addr, ADDR_SIZE);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_id, ADDR_SIZE);


    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);   // bytes of data payload for pipe 0


    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
    // Enable: Dynamic Payload Length, Payload with ACK , W_TX_PAYLOAD_NOACK
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, BV(NRF2401_1D_EN_DPL) | BV(NRF2401_1D_EN_ACK_PAY) | BV(NRF2401_1D_EN_DYN_ACK));

    // Check for Beken BK2425 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2425 bank switch
    dbgprintf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        dbgprintf("BK2425 detected\n");
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

    // Delay 50 ms
    return 50000u;
}


static u16 esky_new_init2()
{
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    packet_sent = 0;
    packet_count = 0;
    rf_ch_num = 0;

    // Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG | BV(NRF24L01_00_PWR_UP));
    // delayMicroseconds(150);
    return 150u;
}


static void calc_fh_channels(u32 seed)
{
    // Use channels 2..79
    u8 first = seed % 37 + 2;
    u8 second = first + 40;
    hopping_frequency[0] = first;  // 0x22;
    hopping_frequency[1] = second; // 0x4a;
    dbgprintf("Using channels %02d and %02d\n", first, second);
}


static void set_tx_id(u32 id)
{
    tx_id[0] = (id >> 24) & 0xFF;
    tx_id[1] = (id >> 16) & 0xFF;
    tx_id[2] = (id >> 8) & 0xFF;
    tx_id[3] = (id >> 0) & 0xFF;
    dbgprintf("TX id %02X %02X %02X %02X\n", tx_id[0], tx_id[1], tx_id[2], tx_id[3]);
    calc_fh_channels(id);
}

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

static u16 convert_2bit_channel(u8 num)
{
    s32 ch = Channels[num];
    u16 ch_out
    if (ch >= 0)
    { 
      if (ch >= CHAN_MAX_VALUE/2) ch_out = 3; else ch_out = 2;       
    } else if (ch < CHAN_MIN_VALUE/2) ch_out = 0; else ch_out = 1;       

    return ch_out; 
}

static void read_controls(u16* throttle, u16* aileron, u16* elevator, u16* rudder,
                          u16* flight_mode, u16* aux_ch6, u16* aux_ch7, u8* flags)
{
    (void) flags;
    // Protocol is registered TAERG, that is
    // Throttle is channel 0, Aileron - 1, Elevator - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range

    *throttle    = convert_channel(CHANNEL1);
    *aileron     = convert_channel(CHANNEL2);
    *elevator    = convert_channel(CHANNEL3);
    *rudder      = convert_channel(CHANNEL4);
    *flight_mode = convert_2bit_channel(CHANNEL5);
    *aux_ch6     = convert_channel(CHANNEL6);
    *aux_ch7     = convert_2bit_channel(CHANNEL7);

    // Print channels every now and then
    if (0) { // (total_packets & 0x3FF) == 1) {
        dbgprintf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        dbgprintf("Aileron %d, elevator %d, throttle %d, rudder %d\n",
               (s16) *aileron, (s16) *elevator, (s16) *throttle, (s16) *rudder);
    }
}



static void send_packet(u8 bind)
{
    u8 rf_ch = 1; // bind channel
    
    if (bind) {
      // Bind packet
      packet[0]  = tx_id[0];
      packet[1]  = tx_id[1];
      packet[2]  = tx_id[2];
      packet[3]  = tx_id[3]; 
      packet[4]  = u1_rx_addr[0];
      packet[5]  = u1_rx_addr[1];
      packet[6]  = u1_rx_addr[2];
      packet[7]  = u1_rx_addr[3];
      packet[8]  = tx_id[0];
      packet[9]  = tx_id[1];
      packet[10] = tx_id[2];
      packet[11] = tx_id[3]; 
      packet[12] = 0;
      packet[13] = 0;
      packet[14] = 0;
    } else {
      rf_ch = hopping_frequency[rf_ch_num];
      rf_ch_num = 1 - rf_ch_num;
  
      read_controls(&throttle, &aileron, &elevator, &rudder, &flight_mode, &aux_ch6, &aux_ch7, &flags);
  
      packet[0]  = hopping_frequency[0];
      packet[1]  = hopping_frequency[1];
      packet[2]  = ((flight_mode << 6) && 0xC0) | ((aux_ch7 << 4) && 0x30) | ((throttle >> 8) & 0xFF);
      packet[3]  = throttle & 0xFF;
      packet[4]  = ((aux_ch6 >> 4) & 0xF0) | ((aileron >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
      packet[5]  = aileron  & 0xFF;
      packet[6]  = (aux_ch6 & 0xF0) | ((elevator >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
      packet[7]  = elevator & 0xFF;
      packet[8]  = ((aux_ch6 << 4) & 0xF0) | ((rudder >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
      packet[9]  = rudder & 0xFF;
      // The next 4 Bytes don't seem to be used. They are constant 00 D8 18 F8 for Esky 150 and 00 00 40 00 for ESKY 150X
      packet[10] = 0x00;
      packet[11] = 0x00;
      packet[12] = 0x00;
      packet[13] = 0x00;
      //calculate checksum:
      u8 sum = 0;
      for (int i = 0; i < 14; ++i) sum += packet[i];
      packet[14] = sum;
    }

    packet_sent = 0;
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, sizeof(packet));
    ++total_packets;
    packet_sent = 1;

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
    if (! rf_ch_num && tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}


MODULE_CALLTYPE
static u16 esky_new_callback()
{
    u16 timeout = PACKET_PERIOD;
    switch (phase) {
    case ESKY_NEW_INIT2:
        timeout = esky_new_init2();
        phase = ESKY_NEW_DATA;
        break;
    case ESKY_NEW_DATA:
        if (packet_count == 4)
            packet_count = 0;
        else {
            if (packet_sent && packet_ack() != PKT_ACKED) {
                dbgprintf("Packet not sent yet\n");
                return PACKET_CHKTIME;
            }
            send_packet(0);
        }
        break;
    }
    return timeout;
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id()
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

    set_tx_id(lfsr);
}

static void initialize(u8 bind)
{
    (void) bind;
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    total_packets = 0;
    u16 timeout = esky_new_init();
    initialize_tx_id();

    CLOCK_StartTimer(timeout, esky_new_callback);
}

const void *ESKY_NEW_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:
            initialize(0);
            return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; // No Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 7L; // T, A, E, R, FMODE, AUX, AUX
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)4L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return (void *) 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
