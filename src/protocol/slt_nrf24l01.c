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
  #define SLT_Cmds PROTO_Cmds
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

#define PAYLOADSIZE 7
#define NFREQCHANNELS 15
#define TXID_SIZE 4

#ifdef EMULATOR
#define USE_FIXED_MFGID
#endif

static u8 packet[PAYLOADSIZE];
static u8 packet_sent;
static u8 tx_id[TXID_SIZE];
static u8 rf_ch_num;
static u16 counter;
static u32 packet_counter;
static u8 tx_power;


//
static u8 phase;
enum {
    SLT_INIT2 = 0,
    SLT_BIND,
    SLT_DATA1,
    SLT_DATA2,
    SLT_DATA3
};


static u8 rf_channels[NFREQCHANNELS];

// Bit vector from bit position
#define BV(bit) (1 << bit)

static const u8 init_vals[][2] = {
    {NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO)}, // 2-bytes CRC, radio off
    {NRF24L01_01_EN_AA, 0x00},      // No Auto Acknoledgement
    {NRF24L01_02_EN_RXADDR, 0x01},  // Enable data pipe 0
    {NRF24L01_03_SETUP_AW, 0x02},   // 4-byte RX/TX address
    {NRF24L01_04_SETUP_RETR, 0x00}, // Disable auto retransmit
    {NRF24L01_07_STATUS, 0x70},     // Clear data ready, data sent, and retransmit
    {NRF24L01_11_RX_PW_P0, 4},      // bytes of data payload for pipe 1
};


static void SLT_init()
{
    NRF24L01_Initialize();
    for (u32 i = 0; i < sizeof(init_vals) / sizeof(init_vals[0]); i++)
        NRF24L01_WriteReg(init_vals[i][0], init_vals[i][1]);
    NRF24L01_SetBitrate(NRF24L01_BR_250K);           // 256kbps
    NRF24L01_SetPower(Model.tx_power);
    u8 rx_tx_addr[] = {0xC3, 0xC3, 0xAA, 0x55};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 4);
    NRF24L01_FlushRx();


    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    // For detailed description of what's happening here see 
    //   http://www.inhaos.com/uploadfile/otherpic/AN0008-BK2423%20Communication%20In%20250Kbps%20Air%20Rate.pdf
    NRF24L01_Activate(0x53); // magic for BK2421/BK2423 bank switch
    printf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        printf("BK2421 detected\n");
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
        printf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    // Implicit delay in callback
    // delay(50);
}


static void SLT_init2()
{
    NRF24L01_FlushTx();
    packet_sent = 0;
    rf_ch_num = 0;

    // Turn radio power on
    u8 config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config);
    // Implicit delay in callback
    // delayMicroseconds(150);
}


static void set_tx_id(u32 id)
{
    for (int i = TXID_SIZE-1; i >= 0; --i) {
        tx_id[i] = id & 0xFF;
        id >>= 8;
    }

    // Frequency hopping sequence generation

    for (int i = 0; i < 4; ++i) {
        int next_i = (i+1) % 4; // is & 3 better than % 4 ?
        u8 base = i < 2 ? 0x03 : 0x10;
        rf_channels[i*4 + 0]  = (tx_id[i] & 0x3f) + base;
        rf_channels[i*4 + 1]  = (tx_id[i] >> 2) + base;
        rf_channels[i*4 + 2]  = (tx_id[i] >> 4) + (tx_id[next_i] & 0x03)*0x10 + base;
        if (i*4 + 3 < NFREQCHANNELS) // guard for 16 channel
            rf_channels[i*4 + 3]  = (tx_id[i] >> 6) + (tx_id[next_i] & 0x0f)*0x04 + base;
    }

    // unique
    for (int i = 0; i < NFREQCHANNELS; ++i) {
        u8 done = 0;
        while (!done) {
            done = 1;
            for (int j = 0; j < i; ++j) {
                if (rf_channels[i] == rf_channels[j]) {
                    done = 0;
                    rf_channels[i] += 7;
                    if (rf_channels[i] >= 0x50) {
                        rf_channels[i] = rf_channels[i] - 0x50 + 0x03;
                    }
                }
            }
        }
    }
    printf("Using id:%02X%02X%02X%02X fh: ", tx_id[0], tx_id[1], tx_id[2], tx_id[3]);
    for (int i = 0; i < NFREQCHANNELS; ++i) {
        printf("%02X ", rf_channels[i]);
    }
    printf("\n");
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_id, 4);
}


static s16 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (s16) ch;
}


static void read_controls(s16 *controls)
{
    // Protocol is registered AETRG, that is
    // Aileron is channel 0, Elevator - 1, Throttle - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than CHAN_MIN_VALUE or larger than
    // CHAN_MAX_VALUE. As we have no space here, we hard-limit
    // channels values by min..max range
    for (int i = 0; i < 6; ++i)
        controls[i] = convert_channel(i);

    // Print channels every second or so
/*
    if ((packet_counter & 0xFF) == 1) {
        printf("Raw channels: %d, %d, %d, %d, %d, %d\n",
               Channels[0], Channels[1], Channels[2], Channels[3], Channels[4], Channels[5]);
        printf("Aileron %d, elevator %d, throttle %d, rudder %d, gear %d, pitch %d\n",
               *aileron, *elevator, *throttle, *rudder, *gear, *pitch);
    }
*/
}


void wait_radio()
{
    if (packet_sent) {
        while (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_TX_DS))) ;
    }
    packet_sent = 0;
}


void send_data(u8 *data, u8 len)
{
    wait_radio();
    NRF24L01_FlushTx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_RX_DR) | BV(NRF24L01_07_MAX_RT));
    NRF24L01_WritePayload(data, len);
    //NRF24L01_PulseCE();
    packet_sent = 1;
}


static void build_packet()
{
    // Raw controls, limited by -10000..10000
    s16 controls[6]; // aileron, elevator, throttle, rudder, gear, pitch
    read_controls(controls);
    u8 e = 0; // byte where extension 2 bits for every 10-bit channel are packed
    for (int i = 0; i < 4; ++i) {
        u16 v = (long) controls[i] * 1023 / 20000L + 512;
        packet[i] = v;
        e = (e >> 2) | (u8) ((v >> 2) & 0xC0);
    }
    // Extra bits for AETR
    packet[4] = e;
    // 8-bit channels
    packet[5] = (long) controls[4] * 255 / 20000L + 128;
    packet[6] = (long) controls[5] * 255 / 20000L + 128;

    // Set radio channel - once per packet batch
    u8 rf_ch = rf_channels[rf_ch_num];
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
    if (++rf_ch_num >= NFREQCHANNELS) rf_ch_num = 0;
    //  Serial.print(rf_ch); Serial.write("\n");
}


static void send_packet()
{
    send_data(packet, sizeof(packet));
    ++packet_counter;
}


static void send_bind_packet()
{
    wait_radio();

    NRF24L01_SetPower(TXPOWER_100uW);
    static const u8 bind_addr[4] = {0x7E, 0xB8, 0x63, 0xA9};
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (u8 *)bind_addr, 4);

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x50);
    send_data(tx_id, sizeof(tx_id));

    // NB: we should wait until the packet's sent before changing TX address!
    wait_radio();
//    printf("Bind packet sent\n");

    NRF24L01_SetPower(tx_power);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_id, 4);
}


void checkTxPower()
{
    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (! rf_ch_num && tx_power != Model.tx_power) {
        // Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}


MODULE_CALLTYPE
static u16 SLT_callback()
{
    u16 delay_us = 20000; // 3 packets with 1ms intervals every 22ms
    switch (phase) {
    case SLT_INIT2:
        SLT_init2();
        MUSIC_Play(MUSIC_TELEMALARM1);
        phase = SLT_BIND;
        delay_us = 150;
        break;

    case SLT_BIND:
        send_bind_packet();
        phase = SLT_DATA1;
        delay_us = 19000;
        break;

    case SLT_DATA1:
        build_packet();
        send_packet();
        phase = SLT_DATA2;
        delay_us = 1000;
        break;

    case SLT_DATA2:
        send_packet();
        phase = SLT_DATA3;
        delay_us = 1000;
        break;

    case SLT_DATA3:
        send_packet();
        if (++counter >= 100) {
            counter = 0;
            phase = SLT_BIND;
            delay_us = 1000;
        } else {
            checkTxPower();
            phase = SLT_DATA1;
        }
        break;
    }
    return delay_us;
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id()
{
    u32 lfsr = 0xb2c54a2ful;

#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    printf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        printf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    printf("\r\n");
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (int i = 0; i < TXID_SIZE; ++i) rand32_r(&lfsr, 0);

    set_tx_id(lfsr);
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_counter = 0;
    counter = 0;
    SLT_init();
    phase = SLT_INIT2;

    initialize_tx_id();

    CLOCK_StartTimer(50000, SLT_callback);
}

const void *SLT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // Always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 6L; // A, E, T, R, G, P
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif //PROTO_HAS_NRF24L01
