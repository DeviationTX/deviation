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
  //Some versions of gcc apply this to definitions, others to calls
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

//=============================================================================
// CRTP (Crazy RealTime Protocol) Implementation
//=============================================================================

// Port IDs
enum {
    CRTP_PORT_CONSOLE = 0x00,
    CRTP_PORT_PARAM = 0x02,
    CRTP_PORT_COMMANDER = 0x03,
    CRTP_PORT_MEM = 0x04,
    CRTP_PORT_LOG = 0x05,
    CRTP_PORT_PLATFORM = 0x0D,
    CRTP_PORT_LINK = 0x0F,
};

// Channel definitions for the LOG port
enum {
    CRTP_LOG_CHAN_TOC = 0x00,
    CRTP_LOG_CHAN_SETTINGS = 0x01,
    CRTP_LOG_CHAN_LOGDATA = 0x02,
};

// Command definitions for the LOG port's TOC channel
enum {
    CRTP_LOG_TOC_CMD_ELEMENT = 0x00,
    CRTP_LOG_TOC_CMD_INFO = 0x01,
};

// Command definitions for the LOG port's CMD channel
enum {
    CRTP_LOG_SETTINGS_CMD_CREATE_BLOCK = 0x00,
    CRTP_LOG_SETTINGS_CMD_APPEND_BLOCK = 0x01,
    CRTP_LOG_SETTINGS_CMD_DELETE_BLOCK = 0x02,
    CRTP_LOG_SETTINGS_CMD_START_LOGGING = 0x03,
    CRTP_LOG_SETTINGS_CMD_STOP_LOGGING = 0x04,
    CRTP_LOG_SETTINGS_CMD_RESET_LOGGING = 0x05,
};

// Log variables types
enum {
    LOG_UINT8 = 0x01,
    LOG_UINT16 = 0x02,
    LOG_UINT32 = 0x03,
    LOG_INT8 = 0x04,
    LOG_INT16 = 0x05,
    LOG_INT32 = 0x06,
    LOG_FLOAT = 0x07,
    LOG_FP16 = 0x08,
};

#define CFLIE_TELEM_LOG_BLOCK_ID            0x01
#define CFLIE_TELEM_LOG_BLOCK_PERIOD_10MS   50 // 50*10 = 500ms

static inline u8 crtp_create_header(u8 port, u8 channel)
{
    return ((port)&0x0F)<<4 | (channel & 0x03);
}

//=============================================================================
// End CRTP implementation
//=============================================================================

// Address size
#define TX_ADDR_SIZE 5

// Timeout for callback in uSec, 10ms=10000us for Crazyflie
#define PACKET_PERIOD 10000


// Channel numbers
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

#define MAX_PACKET_SIZE 32  // CRTP is 32 bytes

static u8 tx_payload_len = 0; // Length of the packet stored in tx_packet
static u8 tx_packet[MAX_PACKET_SIZE]; // For writing Tx payloads
static u8 rx_payload_len = 0; // Length of the packet stored in rx_packet
static u8 rx_packet[MAX_PACKET_SIZE]; // For reading in ACK payloads

static u16 counter;
static u32 packet_counter;
static u8 tx_power, data_rate, rf_channel;

static u8 rx_tx_addr[TX_ADDR_SIZE];

static u8 phase;
enum {
    CFLIE_INIT_SEARCH = 0,
    CFLIE_INIT_TELEMETRY,
    CFLIE_INIT_DATA,
    CFLIE_SEARCH,
    CFLIE_DATA
};

static u8 telemetry_setup_state;
enum {
    CFLIE_TELEM_SETUP_STATE_INIT = 0,
    CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_INFO,
    CFLIE_TELEM_SETUP_STATE_ACK_CMD_GET_INFO,
    CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_ITEM,
    CFLIE_TELEM_SETUP_STATE_ACK_CMD_GET_ITEM,
    // It might be a good idea to add a state here
    // to send the command to reset the logging engine
    // to avoid log block ID conflicts. However, there
    // is not a conflict with the current defaults in
    // cfclient and I'd rather be able to log from the Tx
    // and cfclient simultaneously
    CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK,
    CFLIE_TELEM_SETUP_STATE_ACK_CONTROL_CREATE_BLOCK,
    CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_START_BLOCK,
    CFLIE_TELEM_SETUP_STATE_ACK_CONTROL_START_BLOCK,
    CFLIE_TELEM_SETUP_STATE_COMPLETE,
};

// State variables for the telemetry_setup_state_machine
static u8 toc_size;				// Size of the TOC read from the crazyflie
static u8 next_toc_variable;	// State variable keeping track of the next var to read
static u8 vbat_var_id;			// ID of the vbatMV variable
static u8 extvbat_var_id;		// ID of the extVbatMV variable

// Constants used for finding var IDs from the toc
static const char* pm_group_name = "pm";
static const char* vbat_var_name = "vbatMV";
static const u8 vbat_var_type = LOG_UINT16;
static const char* extvbat_var_name = "extVbatMV";
static const u8 extvbat_var_type = LOG_UINT16;

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

// Bit vector from bit position
#define BV(bit) (1 << bit)

// Packet ack status values
enum {
    PKT_PENDING = 0,
    PKT_ACKED,
    PKT_TIMEOUT
};
#define PACKET_CHKTIME 500      // time to wait if packet not yet acknowledged or timed out    

// Helper for sending a packet
// Assumes packet data has been put in tx_packet
// and tx_payload_len has been set correctly
static void send_packet()
{
    // clear packet status bits and Tx/Rx FIFOs
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();

    // Transmit the payload
    NRF24L01_WritePayload(tx_packet, tx_payload_len);

    ++packet_counter;

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

static u16 dbg_cnt = 0;
static u8 packet_ack()
{
    if (++dbg_cnt > 50) {
        dbgprintf("S: %02x\n", NRF24L01_ReadReg(NRF24L01_07_STATUS));
        dbg_cnt = 0;
    }
    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
    case BV(NRF24L01_07_TX_DS):
        rx_payload_len = NRF24L01_GetDynamicPayloadSize();
        if (rx_payload_len > MAX_PACKET_SIZE) {
            rx_payload_len = MAX_PACKET_SIZE;
        }
        NRF24L01_ReadPayload(rx_packet, rx_payload_len);
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
    s32 f_roll;
    s32 f_pitch;
    s32 f_yaw;
    s32 thrust_truncated;
    u16 thrust;

    struct CommanderPacker
    {
      float roll;
      float pitch;
      float yaw;
      uint16_t thrust;
    } __attribute__((packed)) cpkt;

    // Channels in AETR order
    // Roll, aka aileron, float +- 50.0 in degrees
    // float roll  = -(float) Channels[0]*50.0/10000;
    f_roll = -Channels[0] * FRAC_SCALE / (10000 / 50);

    // Pitch, aka elevator, float +- 50.0 degrees
    //float pitch = -(float) Channels[1]*50.0/10000;
    f_pitch = -Channels[1] * FRAC_SCALE / (10000 / 50);

    // Thrust, aka throttle 0..65535, working range 5535..65535
    // No space for overshoot here, hard limit Channel3 by -10000..10000
    thrust_truncated = Channels[2];
    if (thrust_truncated < CHAN_MIN_VALUE) {
      thrust_truncated = CHAN_MIN_VALUE;
    } else if (thrust_truncated > CHAN_MAX_VALUE) {
      thrust_truncated = CHAN_MAX_VALUE;
    }

    thrust = thrust_truncated*3L + 35535L;
    // Crazyflie needs zero thrust to unlock
    if (thrust < 6000)
      cpkt.thrust = 0;
    else
      cpkt.thrust = thrust;

    // Yaw, aka rudder, float +- 400.0 deg/s
    // float yaw   = -(float) Channels[3]*400.0/10000;
    f_yaw = - Channels[3] * FRAC_SCALE / (10000 / 400);
    frac2float(f_yaw, &cpkt.yaw);

    // Switch on/off?
    if (Channels[4] >= 0) {
        frac2float(f_roll, &cpkt.roll);
        frac2float(f_pitch, &cpkt.pitch);
    } else {
        // Rotate 45 degrees going from X to + mode or opposite.
        // 181 / 256 = 0.70703125 ~= sqrt(2) / 2
        s32 f_x_roll = (f_roll + f_pitch) * 181 / 256;
        frac2float(f_x_roll, &cpkt.roll);
        s32 f_x_pitch = (f_pitch - f_roll) * 181 / 256;
        frac2float(f_x_pitch, &cpkt.pitch);
    }

    // Construct and send packet
    tx_packet[0] = crtp_create_header(CRTP_PORT_COMMANDER, 0); // Commander packet to channel 0
    memcpy(&tx_packet[1], (char*) &cpkt, sizeof(cpkt));
    tx_payload_len = 1 + sizeof(cpkt);
    send_packet();

    // Print channels every 2 seconds or so
    if ((packet_counter & 0xFF) == 1) {
        dbgprintf("Raw channels: %d, %d, %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3],
               Channels[4], Channels[5], Channels[6], Channels[7]);
        dbgprintf("Roll %d, pitch %d, yaw %d, thrust %d\n",
               (int) f_roll*100/FRAC_SCALE, (int) f_pitch*100/FRAC_SCALE, (int) f_yaw*100/FRAC_SCALE, (int) thrust);

    }
}

// State machine for setting up telemetry
// returns 1 when the state machine has completed, 0 otherwise
static u8 telemetry_setup_state_machine()
{
    u8 state_machine_completed = 0;
    // A note on the design of this state machine:
    //
    // Responses from the crazyflie come in the form of ACK payloads.
    // There is no retry logic associated with ACK payloads, so it is possible
    // to miss a response from the crazyflie. To avoid this, the request
    // packet must be re-sent until the expected response is received. However,
    // re-sending the same request generates another response in the crazyflie
    // Rx queue, which can produce large backlogs of duplicate responses.
    //
    // To avoid this backlog but still guard against dropped ACK payloads,
    // transmit cmd packets (which don't generate responses themselves)
    // until an empty ACK payload is received (the crazyflie alternates between
    // 0xF3 and 0xF7 for empty ACK payloads) which indicates the Rx queue on the
    // crazyflie has been drained. If the queue has been drained and the
    // desired ACK has still not been received, it was likely dropped and the
    // request should be re-transmit.

    switch (telemetry_setup_state) {
    case CFLIE_TELEM_SETUP_STATE_INIT:
        toc_size = 0;
        next_toc_variable = 0;
        vbat_var_id = 0;
        extvbat_var_id = 0;
        telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_INFO;
        // fallthrough
    case CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_INFO:
        telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_ACK_CMD_GET_INFO;
        tx_packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC);
        tx_packet[1] = CRTP_LOG_TOC_CMD_INFO;
        tx_payload_len = 2;
        send_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_ACK_CMD_GET_INFO:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 3
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC)
                    && rx_packet[1] == CRTP_LOG_TOC_CMD_INFO) {
                // Received the ACK payload. Save the toc_size
                // and advance to the next state
                toc_size = rx_packet[2];
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_ITEM;
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_INFO;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_ITEM:
        telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_ACK_CMD_GET_ITEM;
        tx_packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC);
        tx_packet[1] = CRTP_LOG_TOC_CMD_ELEMENT;
        tx_packet[2] = next_toc_variable;
        tx_payload_len = 3;
        send_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_ACK_CMD_GET_ITEM:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 3
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC)
                    && rx_packet[1] == CRTP_LOG_TOC_CMD_ELEMENT
                    && rx_packet[2] == next_toc_variable) {
                // For every element in the TOC we must compare its
                // type (rx_packet[3]), group and name (back to back
                // null terminated strings starting with the fifth byte)
                // and see if it matches any of the variables we need
                // for logging
                //
                // Currently enabled for logging:
                //  - vbatMV (LOG_UINT16)
                //  - extVbatMV (LOG_UINT16)
                if(rx_packet[3] == vbat_var_type
                        && (0 == strcmp((char*)&rx_packet[4], pm_group_name))
                        && (0 == strcmp((char*)&rx_packet[4 + strlen(pm_group_name) + 1], vbat_var_name))) {
                    // Found the vbat element - save it for later
                    vbat_var_id = next_toc_variable;
                }

                if(rx_packet[3] == extvbat_var_type
                        && (0 == strcmp((char*)&rx_packet[4], pm_group_name))
                        && (0 == strcmp((char*)&rx_packet[4 + strlen(pm_group_name) + 1], extvbat_var_name))) {
                    // Found the extvbat element - save it for later
                    extvbat_var_id = next_toc_variable;
                }

                // Advance the toc variable counter
                // If there are more variables, read them
                // If not, move on to the next state
                next_toc_variable += 1;
                if(next_toc_variable >= toc_size) {
                    telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK;
                } else {
                    // There are more TOC elements to get
                    telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_ITEM;
                }
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_SEND_CMD_GET_INFO;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK:
        telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_ACK_CONTROL_CREATE_BLOCK;
        tx_packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS);
        tx_packet[1] = CRTP_LOG_SETTINGS_CMD_CREATE_BLOCK;
        tx_packet[2] = CFLIE_TELEM_LOG_BLOCK_ID; // Log block ID
        tx_packet[3] = vbat_var_type; // Variable type
        tx_packet[4] = vbat_var_id; // ID of the VBAT variable
        tx_packet[5] = extvbat_var_type; // Variable type
        tx_packet[6] = extvbat_var_id; // ID of the ExtVBat variable
        tx_payload_len = 7;
        send_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_ACK_CONTROL_CREATE_BLOCK:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 2
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS)
                    && rx_packet[1] == CRTP_LOG_SETTINGS_CMD_CREATE_BLOCK) {
                // Received the ACK payload. Advance to the next state
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_START_BLOCK;
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_START_BLOCK:
        telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_ACK_CONTROL_START_BLOCK;
        tx_packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS);
        tx_packet[1] = CRTP_LOG_SETTINGS_CMD_START_LOGGING;
        tx_packet[2] = CFLIE_TELEM_LOG_BLOCK_ID; // Log block ID 1
        tx_packet[3] = CFLIE_TELEM_LOG_BLOCK_PERIOD_10MS; // Log frequency in 10ms units
        tx_payload_len = 4;
        send_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_ACK_CONTROL_START_BLOCK:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 2
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS)
                    && rx_packet[1] == CRTP_LOG_SETTINGS_CMD_START_LOGGING) {
                // Received the ACK payload. Advance to the next state
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_COMPLETE;
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                telemetry_setup_state =
                        CFLIE_TELEM_SETUP_STATE_SEND_CONTROL_START_BLOCK;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_TELEM_SETUP_STATE_COMPLETE:
        state_machine_completed = 1;
        return state_machine_completed;
        break;
    }

    return state_machine_completed;
}

static int cflie_init()
{
    NRF24L01_Initialize();

    // CRC, radio on
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x01);              // Auto Acknowledgement for data pipe 0
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);          // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, TX_ADDR_SIZE-2); // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x13);         // 3 retransmits, 500us delay

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel);        // Defined by model id
    NRF24L01_SetBitrate(data_rate);                          // Defined by model id

    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);             // Clear data ready, data sent, and retransmit

    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, TX_ADDR_SIZE);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, TX_ADDR_SIZE);

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x01);       // Enable Dynamic Payload Length on pipe 0
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x06);     // Enable Dynamic Payload Length, enable Payload with ACK

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

static void update_telemetry()
{
    static u8 frameloss = 0;

    // Read and reset count of dropped packets
    frameloss += NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX) >> 4;
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel); // reset packet loss counter
    Telemetry.value[TELEM_DSM_FLOG_FRAMELOSS] = frameloss;
    TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);

    if (packet_ack() == PKT_ACKED) {
        // See if the ACK packet is a cflie log packet
        // A log data packet is a minimum of 5 bytes. Ignore anything less.
        if (rx_payload_len >= 5) {
            // Port 5 = log, Channel 2 = data
            if (rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_LOGDATA)) {
                // The log block ID
                if (rx_packet[1] == CFLIE_TELEM_LOG_BLOCK_ID) {
                    // Bytes 6 and 7 are the Vbat in mV units
                    u16 vBat;
                    memcpy(&vBat, &rx_packet[5], sizeof(u16));
                    Telemetry.value[TELEM_DSM_FLOG_VOLT2] = (s32) (vBat / 10); // The log value expects tenths of volts
                    TELEMETRY_SetUpdated(TELEM_DSM_FLOG_VOLT2);

                    // Bytes 8 and 9 are the ExtVbat in mV units
                    u16 extVBat;
                    memcpy(&extVBat, &rx_packet[7], sizeof(u16));
                    Telemetry.value[TELEM_DSM_FLOG_VOLT1] = (s32) (extVBat / 10); // The log value expects tenths of volts
                    TELEMETRY_SetUpdated(TELEM_DSM_FLOG_VOLT1);
                }
            }
        }
    }
}

MODULE_CALLTYPE
static u16 cflie_callback()
{
    switch (phase) {
    case CFLIE_INIT_SEARCH:
        send_search_packet();
        phase = CFLIE_SEARCH;
        break;
    case CFLIE_INIT_TELEMETRY:
        if (telemetry_setup_state_machine()) {
            phase = CFLIE_INIT_DATA;
        }
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
        update_telemetry();
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

        if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON) {
            return CFLIE_INIT_TELEMETRY;
        } else {
            return CFLIE_INIT_DATA;
        }
    } else {
        data_rate = NRF24L01_BR_250K;
        rf_channel = 0;
        return CFLIE_INIT_SEARCH;
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    phase = initialize_rx_tx_addr();
    telemetry_setup_state = CFLIE_TELEM_SETUP_STATE_INIT;
    packet_counter = 0;

    int delay = cflie_init();

    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_DSM);

    dbgprintf("cflie init\n");
    if (phase == CFLIE_INIT_SEARCH) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
    }
    CLOCK_StartTimer(delay, cflie_callback);
}

const void *CFlie_Cmds(enum ProtoCmds cmd)
{
    // dbgprintf("CFlie_Cmds %d\n", cmd);
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
            CLOCK_StopTimer();
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0);
            return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; // never Autobind // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 5L; // A, E, T, R, + or x mode,
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return cflie_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
        default: break;
    }
    return 0;
}
#endif
