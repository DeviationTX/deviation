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
#include "telemetry.h"

#ifdef PROTO_HAS_NRF24L01

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
    CHANNEL11,
    CHANNEL12,
    CHANNEL13,
};

#define CHANNEL_FMODE       CHANNEL9
#define CHANNEL_FLIP        CHANNEL10
#define CHANNEL_VIDEON      CHANNEL11
#define CHANNEL_VIDEOOFF    CHANNEL12 // Q200
#define CHANNEL_PICTURE     CHANNEL12 // Q100 & MR100
#define CHANNEL_CALIBRATE   CHANNEL13 // Q100 & Q200

//#define SLT_Q200_FORCE_ID
#define SLT_PAYLOADSIZE_V1 7
#define SLT_PAYLOADSIZE_V2 11
#define SLT_NFREQCHANNELS 15
#define SLT_TXID_SIZE 4

#ifdef EMULATOR
#define USE_FIXED_MFGID
#endif

static u8 packet[SLT_PAYLOADSIZE_V2];
static u8 rf_channels[SLT_NFREQCHANNELS];
static u8 packet_sent;
static u8 tx_id[SLT_TXID_SIZE];
static u8 rf_ch_num;
static u32 packet_count;
static u8 tx_power;
static u8 phase;

static const char * const slt_opts[] = {
    _tr_noop("Format"), "V1", "V2", "Q100", "Q200", "MR100", NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum {
    FORMAT_V1,
    FORMAT_V2,
    FORMAT_Q100,
    FORMAT_Q200,
    FORMAT_MR100
};

enum{
    // flags going to packet[6] (Q200)
    FLAG_Q200_FMODE = 0x20,
    FLAG_Q200_VIDON = 0x10,
    FLAG_Q200_FLIP  = 0x08,
    FLAG_Q200_VIDOFF= 0x04,
};

enum{
    // flags going to packet[6] (MR100 & Q100)
    FLAG_MR100_FMODE    = 0x20,
    FLAG_MR100_FLIP     = 0x04,
    FLAG_MR100_VIDEO    = 0x02,
    FLAG_MR100_PICTURE  = 0x01,
};

enum {
    SLT_BUILD=0,
    SLT_DATA1,
    SLT_DATA2,
    SLT_DATA3,
    SLT_BIND1,
    SLT_BIND2
};

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
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_V1)
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (u8*)"\xC3\xC3\xAA\x55", SLT_TXID_SIZE);
    else // V2, Q200
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (u8*)"\x7E\xB8\x63\xA9", SLT_TXID_SIZE);
    NRF24L01_FlushRx();
}


static void SLT_init2()
{
    NRF24L01_FlushTx();
    packet_sent = 0;
    rf_ch_num = 0;

    // Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
    u8 config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config);
    // Implicit delay in callback
    // delayMicroseconds(150);
}


static void set_tx_id(u32 id)
{
    for (int i = SLT_TXID_SIZE-1; i >= 0; --i) {
        tx_id[i] = id & 0xFF;
        id >>= 8;
    }

    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q200) { //Q200: Force high part of the ID otherwise it won't bind
        tx_id[0]=0x01;
        tx_id[1]=0x02;
        #ifdef SLT_Q200_FORCE_ID    // ID taken from TX dumps
            tx_id[0]=0x01;tx_id[1]=0x02;tx_id[2]=0x6A;tx_id[3]=0x31;
        /*  tx_id[0]=0x01;tx_id[1]=0x02;tx_id[2]=0x0B;tx_id[3]=0x57;*/
        #endif  
    }
    
    // Frequency hopping sequence generation

    for (int i = 0; i < 4; ++i) {
        int next_i = (i+1) % 4; // is & 3 better than % 4 ?
        u8 base = i < 2 ? 0x03 : 0x10;
        rf_channels[i*4 + 0]  = (tx_id[i] & 0x3f) + base;
        rf_channels[i*4 + 1]  = (tx_id[i] >> 2) + base;
        rf_channels[i*4 + 2]  = (tx_id[i] >> 4) + (tx_id[next_i] & 0x03)*0x10 + base;
        if (i*4 + 3 < SLT_NFREQCHANNELS) // guard for 16 channel
            rf_channels[i*4 + 3]  = (tx_id[i] >> 6) + (tx_id[next_i] & 0x0f)*0x04 + base;
    }

    // unique
    u8 max_freq=0x50;  //V1 sure, V2?
    if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_Q200)
        max_freq=45;
    for (u8 i = 0; i < SLT_NFREQCHANNELS; ++i)
    {
        if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_Q200 && rf_channels[i] >= max_freq)
            rf_channels[i] = rf_channels[i] - max_freq + 0x03;
        u8 done = 0;
        while (!done)
        {
            done = 1;
            for (u8 j = 0; j < i; ++j)
                if (rf_channels[i] == rf_channels[j])
                {
                    done = 0;
                    rf_channels[i] += 7;
                    if (rf_channels[i] >= max_freq)
                        rf_channels[i] = rf_channels[i] - max_freq + 0x03;
                }
        }
    }
    printf("Using id:%02X%02X%02X%02X fh: ", tx_id[0], tx_id[1], tx_id[2], tx_id[3]);
    for (int i = 0; i < SLT_NFREQCHANNELS; ++i) {
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
    for (int i = 0; i < 8; ++i)
        if(i < Model.num_channels)
            controls[i] = convert_channel(i);

    // Print channels every second or so
/*
    if ((packet_count & 0xFF) == 1) {
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

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

static void build_packet()
{
    static uint8_t calib_counter = 0;
    // Raw controls, limited by -10000..10000
    s16 controls[8]; // aileron, elevator, throttle, rudder, gear, pitch
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
    if(Model.proto_opts[PROTOOPTS_FORMAT] != FORMAT_V1) {
        if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q200) 
            packet[6] = GET_FLAG(CHANNEL_FMODE , FLAG_Q200_FMODE)
                      | GET_FLAG(CHANNEL_FLIP, FLAG_Q200_FLIP)
                      | GET_FLAG(CHANNEL_VIDEON, FLAG_Q200_VIDON)
                      | GET_FLAG(CHANNEL_VIDEOOFF, FLAG_Q200_VIDOFF);
        else if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_MR100 || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q100)
            packet[6] =  GET_FLAG(CHANNEL_FMODE , FLAG_MR100_FMODE)
                        |GET_FLAG(CHANNEL_FLIP, FLAG_MR100_FLIP)
                        |GET_FLAG(CHANNEL_VIDEON, FLAG_MR100_VIDEO)  // Does not exist on the Q100 but...
                        |GET_FLAG(CHANNEL_PICTURE, FLAG_MR100_PICTURE); // Does not exist on the Q100 but...
        packet[7]=(long) controls[6] * 255 / 20000L + 128;
        packet[8]=(long) controls[7] * 255 / 20000L + 128;
        packet[9]=0xAA;     //unknown
        packet[10]=0x00;    //unknown
        
        if((Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q100 || Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q200) && GET_FLAG(CHANNEL_CALIBRATE, 1))
        {//Calibrate
            packet[9]=0x77;         //enter calibration
            if(calib_counter>=20 && calib_counter<=25)  // 7 packets for Q100 / 3 packets for Q200
                packet[10]=0x20;    //launch calibration
            calib_counter++;
            if(calib_counter>250) calib_counter=250;
        }
        else
            calib_counter=0;
    }
    // Set radio channel - once per packet batch
    u8 rf_ch = rf_channels[rf_ch_num];
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
    if (++rf_ch_num >= SLT_NFREQCHANNELS) rf_ch_num = 0;
    //  Serial.print(rf_ch); Serial.write("\n");
}


static void send_packet(u8 len)
{
    send_data(packet, len);
}


static void send_bind_packet()
{
    wait_radio();

    NRF24L01_SetPower(TXPOWER_100uW);
    static const u8 bind_addr[4] = {0x7E, 0xB8, 0x63, 0xA9};
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (u8 *)bind_addr, 4);

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x50);
    memcpy((void*)packet,(void*)tx_id,SLT_TXID_SIZE);
    if(phase==SLT_BIND2)
        send_packet(SLT_TXID_SIZE);
    else // SLT_BIND1
        send_packet(SLT_PAYLOADSIZE_V2);

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
    if (tx_power != Model.tx_power) {
        // Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

#define SLT_TIMING_BUILD        1000
#define SLT_V1_TIMING_PACKET    1000
#define SLT_V2_TIMING_PACKET    2042
#define SLT_V1_TIMING_BIND2     1000
#define SLT_V2_TIMING_BIND1     6507
#define SLT_V2_TIMING_BIND2     2112
static u16 SLT_callback()
{
    switch (phase)
    {
        case SLT_BUILD:
            build_packet();
            phase++;
            return SLT_TIMING_BUILD;
        case SLT_DATA1:
        case SLT_DATA2:
            phase++;
            if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1)
            {
                send_packet(SLT_PAYLOADSIZE_V1);
                return SLT_V1_TIMING_PACKET;
            }
            else //V2
            {
                send_packet(SLT_PAYLOADSIZE_V2);
                return SLT_V2_TIMING_PACKET;
            }
        case SLT_DATA3:
            if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1)
                send_packet(SLT_PAYLOADSIZE_V1);
            else //V2
                send_packet(SLT_PAYLOADSIZE_V2);
            if (++packet_count >= 100)
            {// Send bind packet
                packet_count = 0;
                if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1)
                {
                    phase=SLT_BIND2;
                    return SLT_V1_TIMING_BIND2;
                }
                else //V2
                {
                    phase=SLT_BIND1;
                    return SLT_V2_TIMING_BIND1;
                }
            }
            else
            {// Continue to send normal packets
                checkTxPower(); // Set tx_power
                phase = SLT_BUILD;
                if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1)
                    return 20000-SLT_TIMING_BUILD;
                else //V2
                    return 13730-SLT_TIMING_BUILD;
            }
        case SLT_BIND1:
            send_bind_packet();
            phase++;
            return SLT_V2_TIMING_BIND2;
        case SLT_BIND2:
            send_bind_packet();
            phase = SLT_BUILD;
            if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1)
                return 20000-SLT_TIMING_BUILD-SLT_V1_TIMING_BIND2;
            else //V2
                return 13730-SLT_TIMING_BUILD-SLT_V2_TIMING_BIND1-SLT_V2_TIMING_BIND2;
    }
    return 19000;
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
    for (int i = 0; i < SLT_TXID_SIZE; ++i) rand32_r(&lfsr, 0);

    set_tx_id(lfsr);
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_count = 0;
    SLT_init();
    phase = SLT_BIND1;
    initialize_tx_id();
    SLT_init2();
    
    CLOCK_StartTimer(50000, SLT_callback);
}

uintptr_t SLT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;  // Always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 13;  // A, E, T, R, G, P, 7, 8, (Q200) Mode, Flip, VidOn, VidOff/Picture, Calibrate
        case PROTOCMD_DEFAULT_NUMCHAN: return 13;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)slt_opts;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif //PROTO_HAS_NRF24L01
