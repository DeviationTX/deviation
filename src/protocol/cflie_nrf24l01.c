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

/* NB: Not implemented
   Uncomment define below to enable telemetry. Also add
   CFlie protocol to TELEMETRY_SetTypeByProtocol to
   set type to DSM.
   */
//#define CFLIE_TELEMETRY


#ifdef MODULAR
  //Allows the linker to properly relocate
  #define CFlie_Cmds PROTO_Cmds
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
#define BIND_COUNT 5
#define dbgprintf printf
#else
#define BIND_COUNT 60
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
//#define dbgprintf if(0) printf
#define dbgprintf printf
#endif


// Address size
#define TX_ADDR_SIZE 5

// Timeout for callback in uSec, 10ms=10000us for Crazyflie
#define PACKET_PERIOD 10000


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

#define PAYLOADSIZE 8       // receive data pipes set to this size, but unused
#define MAX_PACKET_SIZE 9   // YD717 packets have 8-byte payload, Syma X4 is 9

//static u8 packet[MAX_PACKET_SIZE];

static u16 counter;
static u32 packet_counter;
static u8 tx_power, data_rate, rf_channel;

static u8 rx_tx_addr[TX_ADDR_SIZE];

static u8 phase;
enum {
    CFLIE_INIT_SEARCH = 0,
    CFLIE_INIT_DATA,
    CFLIE_SEARCH,
    CFLIE_DATA
};

#ifdef CFLIE_TELEMETRY
static const char * const cflie_opts[] = {
  _tr_noop("Telemetry"),  _tr_noop("Off"), _tr_noop("On"), NULL,
  NULL
};
enum {
    PROTOOPTS_TELEMETRY = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_OFF 0
#define TELEM_ON 1
#endif

// Bit vector from bit position
#define BV(bit) (1 << bit)

// Packet ack status values
enum {
    PKT_PENDING = 0,
    PKT_ACKED,
    PKT_TIMEOUT
};
#define PACKET_CHKTIME 500      // time to wait if packet not yet acknowledged or timed out    

static u16 dbg_cnt = 0;
static u8 packet_ack()
{
    if (++dbg_cnt > 50) {
        dbgprintf("S: %02x\n", NRF24L01_ReadReg(NRF24L01_07_STATUS));
        dbg_cnt = 0;
    }
    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
    case BV(NRF24L01_07_TX_DS):
        return PKT_ACKED;
    case BV(NRF24L01_07_MAX_RT):
        return PKT_TIMEOUT;
    }
    return PKT_PENDING;
}

static void set_rate_channel(u8 rate, u8 channel)
{
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, channel);     // Defined by model id
    NRF24L01_SetBitrate(rate);             // Defined by model id
}

static void send_search_packet()
{
    uint8_t buf[1];
    buf[0] = 0xff;
    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();

    if (rf_channel++ > 125) {
        rf_channel = 0;
        switch(data_rate) {
        case NRF24L01_BR_250K:
            data_rate = NRF24L01_BR_1M;
            break;
        case NRF24L01_BR_1M:
            data_rate = NRF24L01_BR_2M;
            break;
        case NRF24L01_BR_2M:
            data_rate = NRF24L01_BR_250K;
            break;
        }
    }

    set_rate_channel(data_rate, rf_channel);

    NRF24L01_WritePayload(buf, sizeof(buf));

    ++packet_counter;
}

// Frac 16.16
#define FRAC_MANTISSA 16
#define FRAC_SCALE (1 << FRAC_MANTISSA)

// Convert fractional 16.16 to float32
static void frac2float(s32 n, float* res)
{
    if (n == 0) {
        *res = 0.0;
        return;
    }
    u32 m = n < 0 ? -n : n;
    int i;
    for (i = (31-FRAC_MANTISSA); (m & 0x80000000) == 0; i--, m <<= 1) ;
    m <<= 1; // Clear implicit leftmost 1
    m >>= 9;
    u32 e = 127 + i;
    if (n < 0) m |= 0x80000000;
    m |= e << 23;
    *((u32 *) res) = m;
}

static void send_cmd_packet()
{
    // Commander packet, 15 bytes
    uint8_t buf[15];
    float x_roll, x_pitch, yaw;
  
    // Channels in AETR order

    // Roll, aka aileron, float +- 50.0 in degrees
    // float roll  = -(float) Channels[0]*50.0/10000;
    s32 f_roll = -Channels[0] * FRAC_SCALE / (10000 / 50);

    // Pitch, aka elevator, float +- 50.0 degrees
    //float pitch = -(float) Channels[1]*50.0/10000;
    s32 f_pitch = -Channels[1] * FRAC_SCALE / (10000 / 50);

    // Thrust, aka throttle 0..65535, working range 5535..65535
    // No space for overshoot here, hard limit Channel3 by -10000..10000
    s32 ch = Channels[2];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    uint16_t thrust  = ch*3L + 35535L;

    // Yaw, aka rudder, float +- 400.0 deg/s
    // float yaw   = -(float) Channels[3]*400.0/10000;
    s32 f_yaw = - Channels[3] * FRAC_SCALE / (10000 / 400);
    frac2float(f_yaw, &yaw);
  
    // Convert + to X. 181 / 256 = 0.70703125 ~= sqrt(2) / 2
    s32 f_x_roll = (f_roll + f_pitch) * 181 / 256;
    frac2float(f_x_roll, &x_roll);
    s32 f_x_pitch = (f_pitch - f_roll) * 181 / 256;
    frac2float(f_x_pitch, &x_pitch);

    int bufptr = 0;
    buf[bufptr++] = 0x30; // Commander packet to channel 0
    memcpy(&buf[bufptr], (char*) &x_roll, 4); bufptr += 4;
    memcpy(&buf[bufptr], (char*) &x_pitch, 4); bufptr += 4;
    memcpy(&buf[bufptr], (char*) &yaw, 4); bufptr += 4;
    memcpy(&buf[bufptr], (char*) &thrust, 2); bufptr += 2;


    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();

    NRF24L01_WritePayload(buf, sizeof(buf));

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
    // Print channels every 2 seconds or so
    if ((packet_counter & 0xFF) == 1) {
        dbgprintf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        dbgprintf("Roll %d, pitch %d, yaw %d, thrust %d\n",
               (int) f_x_roll*100/FRAC_SCALE, (int) f_x_pitch*100/FRAC_SCALE, (int) f_yaw*100/FRAC_SCALE, (int) thrust);

    }
}

static int cflie_init()
{
    NRF24L01_Initialize();

    // CRC, radio on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP)); 
//    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);              // No Auto Acknowledgement
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x01);              // Auto Acknowledgement for data pipe 0
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);          // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, TX_ADDR_SIZE-2); // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x13);         // 3 retransmits, 500us delay

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel);        // Defined by model id
    NRF24L01_SetBitrate(data_rate);                           // Defined by model id

    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);             // Clear data ready, data sent, and retransmit

/*
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
*/

    NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00);  // Just in case, no real bits to write here

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x01);       // Enable Dynamic Payload Length on pipe 0
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x06);     // Enable Dynamic Payload Length, enable Payload with ACK


//    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, TX_ADDR_SIZE);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, TX_ADDR_SIZE);

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
        long nul = 0;
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\xF9\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x06, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x07, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x08, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x09, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0A, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0B, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC7\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
    } else {
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    // 50ms delay in callback
    return 50000;
}


#ifdef CFLIE_TELEMETRY
static void update_telemetry() {
  static u8 frameloss = 0;

  frameloss += NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX) >> 4;
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel);   // reset packet loss counter

  Telemetry.p.dsm.flog.frameloss = frameloss;
//  Telemetry.p.dsm.flog.volt[0] = read battery voltage from ack payload
  TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);
}
#endif


MODULE_CALLTYPE
static u16 cflie_callback()
{
    switch (phase) {
    case CFLIE_INIT_SEARCH:
        send_search_packet();
        phase = CFLIE_SEARCH;
        break;
    case CFLIE_INIT_DATA:
        send_cmd_packet();
        phase = CFLIE_DATA;
        break;
    case CFLIE_SEARCH:
        switch (packet_ack()) {
        case PKT_PENDING:
            return PACKET_CHKTIME;                 // packet send not yet complete
        case PKT_ACKED:
            phase = CFLIE_DATA;
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
            break;
        case PKT_TIMEOUT:
            send_search_packet();
            counter = BIND_COUNT;
        }
        break;

    case CFLIE_DATA:
#ifdef CFLIE_TELEMETRY
        update_telemetry();
#endif
        if (packet_ack() == PKT_PENDING)
            return PACKET_CHKTIME;         // packet send not yet complete
        send_cmd_packet();
        break;
    }
    return PACKET_PERIOD;                  // Packet at standard protocol interval
}


// Generate address to use from TX id and manufacturer id (STM32 unique id)
static u8 initialize_rx_tx_addr()
{
    rx_tx_addr[0] = 
    rx_tx_addr[1] = 
    rx_tx_addr[2] = 
    rx_tx_addr[3] = 
    rx_tx_addr[4] = 0xE7; // CFlie uses fixed address



    if (Model.fixed_id) {
        rf_channel = Model.fixed_id % 100;
        switch (Model.fixed_id / 100) {
        case 0:
            data_rate = NRF24L01_BR_250K;
            break;
        case 1:
            data_rate = NRF24L01_BR_1M;
            break;
        case 2:
            data_rate = NRF24L01_BR_2M;
            break;
        default:
            break;
        }
        return CFLIE_INIT_DATA;
    } else {
        data_rate = NRF24L01_BR_250K;
        rf_channel = 0;
        return CFLIE_INIT_SEARCH;
//        return CFLIE_INIT_DATA;
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    phase = initialize_rx_tx_addr();
    packet_counter = 0;

    int delay = cflie_init();

#ifdef CFLIE_TELEMETRY
    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_DSM);
#endif
    dbgprintf("cflie init\n");
    if (phase == CFLIE_INIT_SEARCH) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
    }
    CLOCK_StartTimer(delay, cflie_callback);
}

const void *CFlie_Cmds(enum ProtoCmds cmd)
{
    dbgprintf("CFlie_Cmds %d\n", cmd);
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
            CLOCK_StopTimer();
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0);
            return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; // never Autobind // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 4L; // A, E, T, R
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)4L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
#ifdef CFLIE_TELEMETRY
        case PROTOCMD_GETOPTIONS: return cflie_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
#else
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
#endif
        default: break;
    }
    return 0;
}
#endif

