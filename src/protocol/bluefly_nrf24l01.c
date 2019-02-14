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
  #define BlueFly_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter

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

#define FREQUENCY_NUM 15
#define PAYLOAD_SIZE  12
// available frequency must be in between 2402 and 2477
static u8 hopping_frequency_start;
static u8 hopping_frequency_no;

static const u8  binding_adr_rf[TXID_SIZE]={0x32,0xaa,0x45,0x45,0x78}; // fixed binding ids for all planes
// rf_adr_buf can be used for fixed id
static u8 rf_adr_buf[TXID_SIZE]; // ={0x13,0x88,0x46,0x57,0x76};

static u8 bind_payload[PAYLOAD_SIZE];

static unsigned int ch_data[8];
static u8 payload[PAYLOAD_SIZE];
static u8 counter1ms;


// BlueFly protocol uses TX id as an address for nRF24L01, and uses linear
// frequency hopping sequence the first channel of which is passed in
// binding packet, it's 2 to 46 which makes the frequency channel span
// 2 to 74
static void calc_fh_channels(u32 seed)
{
    hopping_frequency_start = ((seed >> 8) % 47) + 2;
}


static void build_binding_packet(void)
{
    int i;
    for (i = 0; i < TXID_SIZE; ++i)
      bind_payload[i] = rf_adr_buf[i];
    bind_payload[i++] = hopping_frequency_start;
    for (; i < PAYLOAD_SIZE; ++i) bind_payload[i] = 0x55;
}

static void bluefly_init()
{
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable p0 rx
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rf_adr_buf, 5);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOAD_SIZE); // payload size = 12
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81); // binding packet must be set in channel 81

    // 2-bytes CRC, radio on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG,
            BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
    NRF24L01_SetBitrate(NRF24L01_BR_250K);           // BlueFly - 250kbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
}

// HiSky channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITCH, channel data value is from 0 to 1000
static void build_ch_data()
{
    s32 temp;
    int i;
    for (i = 0; i< 8; ++i) {
        if (i >= Model.num_channels)
            ch_data[i] = 500; // any data between 0 to 1000 is ok
        else {
            temp = (s32)Channels[i] * 300/CHAN_MAX_VALUE + 500; // 200-800 range
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

    unsigned char l, h, t;
    l = h = 0xff;
    for (int i=0; i<10; ++i) {
        h ^= payload[i];
        h ^= h >> 4;
        t = h;
        h = l;
        l = t;
        t = (l<<4) | (l>>4);
        h ^= ((t<<2) | (t>>6)) & 0x1f;
        h ^= t & 0xf0;
        l ^= ((t<<1) | (t>>7)) & 0xe0;
    }
    // Checksum
    payload[10] = h; 
    payload[11] = l;
#ifdef EMULATOR
    for (i = 0; i < 8; i++)
        printf("ch[%d]=%d,  payload[%d]=%d\n", i, ch_data[i], i, payload[i]);
    printf("payload[8]=%d\n", payload[8]);
    printf("payload[9]=%d\n", payload[9]);
#endif
}

static u16 bluefly_cb()
{
    switch(counter1ms++) {

    case 0:
        build_ch_data();
        break;
    case 1:
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency_start + hopping_frequency_no*2);
        hopping_frequency_no++;
        hopping_frequency_no %= 15;
        NRF24L01_FlushTx();
        NRF24L01_WritePayload(payload, PAYLOAD_SIZE);
        break;
    case 2:
        break;
    case 3:
        if (counter>0) {
            counter--;
            if (! counter) {
                PROTOCOL_SetBindState(0);
            }
            NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, binding_adr_rf, 5);
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);
            NRF24L01_FlushTx();
            NRF24L01_WritePayload(bind_payload, PAYLOAD_SIZE);
        }
        break;
    case 4:
        break;
    case 5:
        NRF24L01_SetPower(Model.tx_power);
        /* FALLTHROUGH */
    default:
        counter1ms = 0;
        break;
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
    build_binding_packet();
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    initialize_tx_id();

    bluefly_init();

    if(bind || ! Model.fixed_id) {
        counter = BIND_COUNT;
        PROTOCOL_SetBindState((BIND_COUNT > 200 ? BIND_COUNT : 800) * 10); //8 seconds binding time
    } else {
        counter = 0;
    }

    CLOCK_StartTimer(1000, bluefly_cb);
}

uintptr_t BlueFly_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT: initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 0;  // Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 6;
        case PROTOCMD_DEFAULT_NUMCHAN: return 6;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
