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
  #define ESKY_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

#ifdef MODULAR
  //Some versions of gcc apply this to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#define BIND_COUNT 1000
// Timeout for callback in uSec, 3ms ~300 packets/s
#define PACKET_PERIOD 3333
#define PACKET_CHKTIME  100 // Time to wait for packet to be sent (no ACK, so very short)

#define PAYLOADSIZE 13

static u8 packet[PAYLOADSIZE];
static u8 packet_sent;
static u8 tx_id[4];

static u8 rf_ch_num; // index into channel array
static u8 rf_channels[6]; // 3 repeats on 2 channels
static u8 end_bytes[6];


static u16 counter;
static u32 packet_counter;

static u8 tx_power;
static u16 input[6]; // aileron, elevator, throttle, rudder, gyro, pitch

static u8 channel_code;


//
static u8 phase;
enum {
    ESKY_INIT2 = 0,
    ESKY_INIT2_NO_BIND,
    ESKY_BIND,
    ESKY_DATA
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

static void set_bind_address()
{
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);     // 3-byte RX/TX address for bind packets
    u8 rx_tx_addr[] = {0x00, 0x00, 0x00};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 3);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_tx_addr, 3);
}


static void set_data_address()
{
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);     // 4-byte RX/TX address for regular packets
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, tx_id, 4);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    tx_id, 4);
}


static void esky_init(u8 bind)
{
    NRF24L01_Initialize();

    // 2-bytes CRC, radio off
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);            // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);        // Enable data pipe 0
    if (bind) {
        set_bind_address();
    } else {
        set_data_address();
    }
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);          // No auto retransmission
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 50);              // Channel 50 for bind packets
    NRF24L01_SetBitrate(0);                                // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);           // Clear data ready, data sent, and retransmit
//    NRF24L01_WriteReg(NRF24L01_08_OBSERVE_TX, 0x00);     // no write bits in this field
//    NRF24L01_WriteReg(NRF24L01_00_CD, 0x00);             // same
//    NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3);       // LSB byte of pipe 2 receive address
//    NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
//    NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
//    NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);  // bytes of data payload for pipe 0
    NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00);      // Just in case, no real bits to write here

    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    printf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        printf("BK2421 detected\n");
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
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC7\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
    } else {
        printf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    // Implicit delay in callback
    // delay(50);
}

static void esky_init2()
{
    NRF24L01_FlushTx();
    packet_sent = 0;
    rf_ch_num = 0;
    u32 channel_ord = rand32_r(0, 0) % 74;
    channel_code = 10 + (u8) channel_ord;
    u8 channel1, channel2;
    channel1 = 10 + (u8) ((37 + channel_ord*5) % 74);
    channel2 = 10 + (u8) ((channel_ord*5) % 74) ;
    printf("channel code %d, channel1 %d, channel2 %d\n", (int) channel_code, (int) channel1, (int) channel2);

    rf_channels[0] = channel1;
    rf_channels[1] = channel1;
    rf_channels[2] = channel1;
    rf_channels[3] = channel2;
    rf_channels[4] = channel2;
    rf_channels[5] = channel2;

    end_bytes[0] = 6;
    end_bytes[1] = channel1*2;
    end_bytes[2] = channel2*2;
    end_bytes[3] = 6;
    end_bytes[4] = channel1*2;
    end_bytes[5] = channel2*2;

    // Turn radio power on
    u8 config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config);
    // Implicit delay in callback
    // delayMicroseconds(150);
}

static void set_tx_id(u32 id)
{
    tx_id[0] = (id >> 16) & 0xFF;
    tx_id[1] = (id >> 8) & 0xFF;
    tx_id[2] = (id >> 0) & 0xFF;
    tx_id[3] = 0xBB;
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


static void read_controls(u16* values)
{
    // Protocol is registered AETRG, that is
    // Aileron is channel 0, Elevator - 1, Throttle - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range
    for (u8 i = 0; i < 6; i++) {
        values[i] = convert_channel(i);
    }

    // Print channels every second or so
    if (0) { // (packet_counter & 0xFF) == 1) {
        printf("Raw channels: %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2],
               Channels[3], Channels[4], Channels[5]);
        printf("Aileron %d, elevator %d, throttle %d, rudder %d, gyro %d, pitch %d\n",
               (s16) values[0], (s16) values[1], (s16) values[2],
               (s16) values[3], (s16) values[4], (s16) values[5]);
    }
}

static void send_packet(u8 bind)
{
    u8 rf_ch = 50; // bind channel
    if (bind) {
        // Bind packet
        packet[0]  = tx_id[2];
        packet[1]  = tx_id[1];
        packet[2]  = tx_id[0];
        packet[3]  = channel_code; // encodes pair of channels to transmit on
        packet[4]  = 0x18;
        packet[5]  = 0x29;
        packet[6]  = 0;
        packet[7]  = 0;
        packet[8]  = 0;
        packet[9]  = 0;
        packet[10] = 0;
        packet[11] = 0;
        packet[12] = 0;
    } else {
        // Regular packet

        // Each data packet is repeated 3 times on one channel, and 3 times on another channel
        // For arithmetic simplicity, channels are repeated in rf_channels array

        if (rf_ch_num == 0) read_controls(input);
        rf_ch = rf_channels[rf_ch_num];
        u8 end_byte = end_bytes[rf_ch_num];
        rf_ch_num += 1;
        if (rf_ch_num > 6) rf_ch_num = 0;

        for (int i = 0; i < 6; i++) {
            packet[i*2]    = (u8) (input[i] >> 8);
            packet[i*2+1]  = (u8) (input[i] & 0xFF);
        }
        packet[12] = end_byte;

    }
    packet_sent = 0;
    //  Serial.print(rf_ch); Serial.write("\n");
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, sizeof(packet));
    ++packet_counter;
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
static u16 esky_callback()
{
    switch (phase) {
    case ESKY_INIT2:
        esky_init2();
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = ESKY_BIND;
        return 150;
        break;
    case ESKY_INIT2_NO_BIND:
        esky_init2();
        phase = ESKY_DATA;
        return 150;
        break;
    case ESKY_BIND:
        if (packet_sent && packet_ack() != PKT_ACKED) {
            printf("Packet not sent yet\n");
            return PACKET_CHKTIME;
        }
        send_packet(1);
        if (--counter == 0) {
            phase = ESKY_DATA;
            set_data_address();
            PROTOCOL_SetBindState(0);
            MUSIC_Play(MUSIC_DONE_BINDING);
        }
        break;
    case ESKY_DATA:
        if (packet_sent && packet_ack() != PKT_ACKED) {
            printf("Packet not sent yet\n");
            return PACKET_CHKTIME;
        }
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_counter = 0;
    phase = bind ? ESKY_INIT2 : ESKY_INIT2_NO_BIND;
    if (bind) {
        counter = BIND_COUNT;
        PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000); // msec
    }

    u32 id = 0xb2c54a2f;
    if (Model.fixed_id) {
        id ^= Model.fixed_id + (Model.fixed_id << 16);
    } else {
        /*
        u32* stm32id = (uint32_t*) 0x1FFFF7E8;
        id ^= *stm32id++;
        id ^= *stm32id++;
        id ^= *stm32id;
        */
        id = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) % 999999;
    }
    set_tx_id(id);
    esky_init(bind);
    CLOCK_StartTimer(50000, esky_callback);
}

const void *ESKY_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT: return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 6L; // AETRGP
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)-1;
        case PROTOCMD_SET_TXPOWER:
            tx_power = Model.tx_power;
            NRF24L01_SetPower(tx_power); // Is it needed?
            break;
        default: break;
    }
    return 0;
}
#endif
