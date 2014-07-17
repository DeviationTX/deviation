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
#else
#define BIND_COUNT 800
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
    NRF24L01_WriteReg(NRF24L01_00_CONFIG,
            BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
    NRF24L01_SetBitrate(0);                          // 1Mbps
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
}

// HiSky channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITH, channel data value is from 0 to 1000
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
        printf("ch[%d]=%d,  payload[%d]=%d\n", i, ch_data[i], i, payload[i]);
    printf("payload[8]=%d\n", payload[8]);
    printf("payload[9]=%d\n", payload[9]);
#endif
}

MODULE_CALLTYPE
static u16 hisky_cb()
{
    counter1ms++;
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

    for (u8 i = 0; i < TXID_SIZE; ++i) {
        rf_adr_buf[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }

    printf("Effective id: %02X%02X%02X%02X%02X\r\n",
        rf_adr_buf[0], rf_adr_buf[1], rf_adr_buf[2], rf_adr_buf[3], rf_adr_buf[4]);

    // Use LFSR to seed frequency hopping sequence after another
    // divergence round
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    calc_fh_channels(lfsr);

    printf("FH Seq: ");
    for (int i = 0; i < FREQUENCE_NUM; ++i) {
        printf("%d, ", hopping_frequency[i]);
    }
    printf("\r\n");
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
        case PROTOCMD_NUMCHAN: return (void *)6L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
