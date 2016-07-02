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
  #define HiSky_Cmds PROTO_Cmds
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
#define BIND_COUNT 800
#define dbgprintf if(0) printf
#endif

static int counter;

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define TXID_SIZE 5

#define FREQUENCE_NUM  20
// available frequency must be in between 2402 and 2477
static u8 hopping_frequency[FREQUENCE_NUM];
static u8 hopping_frequency_no;

static const u8  binding_adr_rf[5]={0x12,0x23,0x23,0x45,0x78}; // fixed binding ids for all planes
// rf_adr_buf can be used for fixed id
static u8 rf_adr_buf[5]; // ={0x13,0x88,0x46,0x57,0x76};

static u8 binding_idx;
static u8 bind_buf_arry[4][10];

static unsigned int ch_data[8];
static u8 payload[10];
static u8 counter1ms;

static const char * const hisky_opts[] = {
    _tr_noop("Format"), _tr_noop("Default"), "HK310 3Ch", NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
};

enum {
    PROTOOPTS_FORMAT_DEFAULT = 0,
    PROTOOPTS_FORMAT_HK310, // 3 channel RXs, HKR3000/3100, XY3000/3100 ...
};

enum {
    STICKS = 0,
    FAILSAFE,
};

// HiSky protocol uses TX id as an address for nRF24L01, and uses frequency hopping sequence
// which does not depend on this id and is passed explicitly in binding sequence. So we are free
// to generate this sequence as we wish. It should be in the range [02..77]
static void calc_fh_channels(u32 seed)
{
    int idx = 0;
    u32 rnd = seed;
    while (idx < FREQUENCE_NUM) {
        int i;
        int count_2_26 = 0, count_27_50 = 0, count_51_74 = 0;
        rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

        // Use least-significant byte. 73 is prime, so channels 76..77 are unused
        u8 next_ch = ((rnd >> 8) % 73) + 2;
        // Keep the distance 2 between the channels - either odd or even
        if (((next_ch ^ seed) & 0x01 )== 0)
            continue;
        // Check that it's not duplicate and spread uniformly
        for (i = 0; i < idx; i++) {
            if(hopping_frequency[i] == next_ch)
                break;
            if(hopping_frequency[i] <= 26)
                count_2_26++;
            else if (hopping_frequency[i] <= 50)
                count_27_50++;
            else
                count_51_74++;
        }
        if (i != idx)
            continue;
        if ((next_ch <= 26 && count_2_26 < 8)
          ||(next_ch >= 27 && next_ch <= 50 && count_27_50 < 8)
          ||(next_ch >= 51 && count_51_74 < 8))
        {
            hopping_frequency[idx++] = next_ch;
        }
    }
}

// for HiSky hk310 protocol, the transmitter always generates hop channels in sequencial order. 
// The transmitter only generates the first hop channel between 0 and 49. So the channel range is from 0 to 69.
static void calc_hk310_fh_channels(u32 seed)
{
    u8 idx = 0;
    u8 chan = seed % 50;
    while(idx < FREQUENCE_NUM) {
        hopping_frequency[idx++] = chan++;
    }
}

static void build_binding_packet(void)
{
    u8 i;
    unsigned int  sum;
    u8 sum_l,sum_h;

    counter1ms = 0;
    hopping_frequency_no = 0;

    sum = 0;
    for(i=0;i<5;i++)
      sum += rf_adr_buf[i];
    sum_l = (u8)sum;
    sum >>= 8;
    sum_h = (u8)sum;
    bind_buf_arry[0][0] = 0xff;
    bind_buf_arry[0][1] = 0xaa;
    bind_buf_arry[0][2] = 0x55;
    for(i=3;i<8;i++)
      bind_buf_arry[0][i] = rf_adr_buf[i-3];

    for(i=1;i<4;i++)
    {
      bind_buf_arry[i][0] = sum_l;
      bind_buf_arry[i][1] = sum_h;
      bind_buf_arry[i][2] = i-1;
    }
    for(i=0;i<7;i++)
    bind_buf_arry[1][i+3] = hopping_frequency[i];
    for(i=0;i<7;i++)
    bind_buf_arry[2][i+3] = hopping_frequency[i+7];
    for(i=0;i<6;i++)
    bind_buf_arry[3][i+3] = hopping_frequency[i+14];

    binding_idx = 0;
}

static void hisky_init()
{
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable p0 rx
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rf_adr_buf, 5);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 10); // payload size = 10
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81); // binding packet must be set in channel 81

    // 2-bytes CRC, radio off
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG,
            BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_HK310)
        NRF24L01_SetBitrate(NRF24L01_BR_250K); // 250Kbps (works with nRF24L01+ only)
    else
        NRF24L01_SetBitrate(NRF24L01_BR_1M);   // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit


    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    // For detailed description of what's happening here see 
    //   http://www.inhaos.com/uploadfile/otherpic/AN0008-BK2423%20Communication%20In%20250Kbps%20Air%20Rate.pdf
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
}

// HiSky channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITH, channel data value is from 0 to 1000
 // Channel 7 - Gyro mode, 0 - 6 axis, 3 - 3 axis 
static void build_ch_data()
{
    s32 temp;
    u8 i;
    for (i = 0; i< 8; i++) {
        if (i >= Model.num_channels)
            ch_data[i] = 500; // any data between 0 to 1000 is ok
        else {
            temp = (s32)Channels[i] * 450/CHAN_MAX_VALUE + 500; // max/min servo range is +-125%
            if (i == 2) // It is clear that hisky's thro stick is made reversely, so I adjust it here on purpose
                temp = 1000 -temp;
            if (i == 6)
                temp = Channels[i] <= 0 ? 0 : 3;
            if (temp < 0)
                ch_data[i] = 0;
            else if (temp > 1000)
                ch_data[i] = 1000;
            else
                ch_data[i] = (unsigned int)temp;
        }

        payload[i] = (u8)ch_data[i];
    }

    payload[8]  = (u8)((ch_data[0]>>8)&0x0003);
    payload[8] |= (u8)((ch_data[1]>>6)&0x000c);
    payload[8] |= (u8)((ch_data[2]>>4)&0x0030);
    payload[8] |= (u8)((ch_data[3]>>2)&0x00c0);

    payload[9]  = (u8)((ch_data[4]>>8)&0x0003);
    payload[9] |= (u8)((ch_data[5]>>6)&0x000c);
    payload[9] |= (u8)((ch_data[6]>>4)&0x0030);
    payload[9] |= (u8)((ch_data[7]>>2)&0x00c0);
#ifdef EMULATOR
    for (i = 0; i < 8; i++)
        dbgprintf("ch[%d]=%d,  payload[%d]=%d\n", i, ch_data[i], i, payload[i]);
    dbgprintf("payload[8]=%d\n", payload[8]);
    dbgprintf("payload[9]=%d\n", payload[9]);
#endif
}

// Stick values are sent as direct timer values for an 8051 timer with a 750 ns clock
static u16 convert_hk310_channel(u8 num)
{
    // -/+ 125% mixer scale = 1000-2000us
    u16 val = ((s32)Channels[num] * 400 / CHAN_MAX_VALUE) + 1500;
    return 0xffff-(4*val)/3;
}

static u16 convert_failsafe_channel(u8 num)
{
    // -/+ 125 failsafe value = 1000-2000us
    u16 val = ((s32)Model.limits[num].failsafe + 125) * 4 + 1000;
    return 0xffff-(4*val)/3;
}

static void build_hk310_ch_data(u8 packet_type)
{
    u8 index=0;
    u16 value;
    if(packet_type == STICKS) {
        for(u8 ch=0; ch<3; ch++) {
            value = convert_hk310_channel(ch);
            payload[index++] = value & 0xff;
            payload[index++] = value >> 8;
        }
        payload[7] = 0x55;
        payload[8] = 0x67;
    }
    else if(packet_type == FAILSAFE) {
        u8 failsafe_enabled = 0;
        for(u8 ch=0; ch<3; ch++) {
            if(Model.limits[ch].flags & CH_FAILSAFE_EN) {
                failsafe_enabled = 1;
                value = convert_failsafe_channel(ch);
            }
            else if(ch<2) {
                value =  0xf82f; // 1500us
            }
            payload[index++] = value & 0xff;
            payload[index++] = value >> 8;
        }
        payload[7] = 0xaa;
        payload[8] = failsafe_enabled ? 0x5a : 0x5b;
    }
}

MODULE_CALLTYPE
static u16 hisky_cb()
{
    counter1ms++;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_HK310) {
        switch(counter1ms) {
            case 1:
                NRF24L01_SetPower(Model.tx_power);
                counter1ms = 2;
                break;
            case 3:
                if(!counter)
                    NRF24L01_WritePayload(payload,10);
                break;
            case 4:
                counter1ms = 6; // hops channel every 5ms
                break;
            case 7:
                if(hopping_frequency_no != 0)
                    build_hk310_ch_data(STICKS);
                else
                    build_hk310_ch_data(FAILSAFE);
                counter1ms = 8; 
                break;
        }
    }
    if(counter1ms==1)
        NRF24L01_FlushTx();
    else if(counter1ms==2) {
        if (counter>0) {
            NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (u8 *)binding_adr_rf, 5);
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);
        }
    }else if(counter1ms==3) {
        if (counter >0)
        {
            counter--;
            if (! counter) { // binding finished, change tx add
                PROTOCOL_SetBindState(0);
            }
            NRF24L01_WritePayload(bind_buf_arry[binding_idx],10);
            binding_idx++;
            if (binding_idx >= 4)
                binding_idx = 0;
        }

    } else if (counter1ms==4) {
        if (counter > 0)
            NRF24L01_FlushTx();
    }else if(counter1ms==5)
        NRF24L01_SetPower(Model.tx_power);
    else if (counter1ms == 6) {
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
        hopping_frequency_no++;
        if (hopping_frequency_no >= FREQUENCE_NUM)
            hopping_frequency_no = 0;
    }
    else if (counter1ms == 7) {
        build_ch_data();
    }
    else if(counter1ms>8){
        counter1ms = 0;
        NRF24L01_WritePayload(payload,10);
    }
#ifdef EMULATOR
    return 100;
#else
    return 1000;  // send 1 binding packet and 1 data packet per 9ms
#endif
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id()
{
    u32 lfsr = 0x7649eca9ul;

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
    for (int i = 0; i < TXID_SIZE; ++i) rand32_r(&lfsr, 0);

    for (u8 i = 0; i < TXID_SIZE; ++i) {
        rf_adr_buf[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }

    dbgprintf("Effective id: %02X%02X%02X%02X%02X\r\n",
        rf_adr_buf[0], rf_adr_buf[1], rf_adr_buf[2], rf_adr_buf[3], rf_adr_buf[4]);

    // Use LFSR to seed frequency hopping sequence after another
    // divergence round
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_DEFAULT) {
        calc_fh_channels(lfsr);
    }
    else if(Model.proto_opts[PROTOOPTS_FORMAT] == PROTOOPTS_FORMAT_HK310) {
        calc_hk310_fh_channels(lfsr);
    }
    dbgprintf("FH Seq: ");
    for (int i = 0; i < FREQUENCE_NUM; ++i) {
        dbgprintf("%d, ", hopping_frequency[i]);
    }
    dbgprintf("\r\n");
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    initialize_tx_id();

    build_binding_packet();
    hisky_init();

    if(bind || ! Model.fixed_id) {
        counter = BIND_COUNT;
        PROTOCOL_SetBindState((BIND_COUNT > 200 ? BIND_COUNT : 800) * 10); //8 seconds binding time
    } else {
        counter = 0;
    }


    CLOCK_StartTimer(1000, hisky_cb);
}

const void *HiSky_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)7L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return hisky_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
