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
#include "config/tx.h"
#include "telemetry.h"

#ifdef PROTO_HAS_CC2500

static const char * const radiolink_opts[] = {
  _tr_noop("Format"),  "Surface", "Air", "Dumbo", NULL,
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  NULL
};

enum {
    PROTOOPTS_FORMAT,
    PROTOOPTS_FREQ,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum {
    FORMAT_SURFACE,
    FORMAT_AIR,
    FORMAT_DUMBO,
};


#define RLINK_TX_PACKET_LEN 33
#define RLINK_RX_PACKET_LEN 15
#define RLINK_TX_ID_LEN     4
#define RLINK_HOP           16
#ifdef EMULATOR
    #define dbgprintf printf
    #define RLINK_TIMING_PROTO  2000
    #define RLINK_TIMING_RFSEND 1050
    #define RLINK_TIMING_CHECK  200
#else
    #define RLINK_TIMING_PROTO  20000-100       // -100 for compatibility with R8EF
    #define RLINK_TIMING_RFSEND 10500
    #define RLINK_TIMING_CHECK  2000
#endif

enum {
    RLINK_DATA  = 0x00,
    RLINK_RX1   = 0x01,
    RLINK_RX2   = 0x02,
};


static uint32_t pps_timer;
static uint16_t pps_counter;
static uint8_t  hopping_frequency[50];
static uint8_t  rx_tx_addr[5];
static uint8_t  phase;
static uint8_t  rf_ch_num;
static uint8_t  packet[50];
static uint8_t  packet_count;
#define TELEMETRY_BUFFER_SIZE 32
static uint8_t packet_in[TELEMETRY_BUFFER_SIZE];   //telemetry receiving packets
static s8 freq_offset;
static uint32_t RLINK_rand1;
static uint32_t RLINK_rand2;

static uint32_t RLINK_prng_next(uint32_t r)
{
    return 0xA5E2A705 * r + 0x754DB79B;
}

static void RLINK_init_random(uint32_t id)
{
    uint32_t result = id;

    RLINK_rand2 = result;
    for (uint8_t i=0; i<31; i++)
        result = RLINK_prng_next(result);
    RLINK_rand1 = result;
}

static uint8_t RLINK_next_random_swap()
{
    uint8_t result = (RLINK_rand2 >> 16) + RLINK_rand2 + (RLINK_rand1 >> 16) + RLINK_rand1;

    RLINK_rand2 = RLINK_prng_next(RLINK_rand2);
    RLINK_rand1 = RLINK_prng_next(RLINK_rand1);

    return result & 0x0F;
}

static uint32_t RLINK_compute_start_id(uint32_t id)
{
    return id * 0xF65EF9F9u + 0x2EDDF6CAu;
}

static void RLINK_shuffle_freqs(uint32_t seed)
{
    RLINK_init_random(seed);

    for(uint8_t i=0; i<RLINK_HOP; i++)
    {
        uint8_t r   = RLINK_next_random_swap();
        uint8_t tmp = hopping_frequency[r];
        hopping_frequency[r] = hopping_frequency[i];
        hopping_frequency[i] = tmp;
    }
}

// Convert 32b id to rx_tx_addr
static void set_rx_tx_addr(u32 id)
{
  rx_tx_addr[0] = (id >> 24) & 0xFF;
  rx_tx_addr[1] = (id >> 16) & 0xFF;
  rx_tx_addr[2] = (id >>  8) & 0xFF;
  rx_tx_addr[3] = (id >>  0) & 0xFF;
  rx_tx_addr[4] = (rx_tx_addr[2]&0xF0)|(rx_tx_addr[3]&0x0F);
}

static uint32_t get_tx_id()
{
    u32 lfsr = 0x7649eca9ul;

    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
    for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
        rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    return rand32_r(&lfsr, 0);
}

static void RLINK_hop()
{
    uint8_t inc=3*(rx_tx_addr[0]&3);
    
    // init hop table
    for(uint8_t i=0; i<RLINK_HOP; i++)
        hopping_frequency[i] = (12*i) + inc;

    // shuffle
    RLINK_shuffle_freqs(RLINK_compute_start_id(rx_tx_addr[0] + (rx_tx_addr[1] << 8)));
    RLINK_shuffle_freqs(RLINK_compute_start_id(rx_tx_addr[2] + (rx_tx_addr[3] << 8)));

    // replace one of the channel randomely
    rf_ch_num = rx_tx_addr[4] % 0x11;      // 0x00..0x10
    if (inc == 9) inc = 6;                 // frequency exception
    hopping_frequency[rf_ch_num] = 12 * 16 + inc;
}

static void RLINK_TXID_init()
{
    set_rx_tx_addr(get_tx_id());

    #ifdef RLINK_FORCE_ID
        //surface RC6GS
        //memcpy(rx_tx_addr,"\x3A\x99\x22\x3A",RLINK_TX_ID_LEN);
        //air T8FB
        //memcpy(rx_tx_addr,"\xFC\x11\x0D\x20",RLINK_TX_ID_LEN);
        // hexfet's am-x12
        // memcpy(rx_tx_addr,"\x8a\xda\xf5\xcc\x02",RLINK_TX_ID_LEN+1);
    #endif

    // channels order depend on ID
    RLINK_hop();
 }

static const uint8_t RLINK_init_values[] = {
  /* 00 */ 0x5B, 0x06, 0x5C, 0x07, 0xAB, 0xCD, 0x40, 0x04,
  /* 08 */ 0x45, 0x00, 0x00, 0x06, 0x00, 0x5C, 0x62, 0x76,
  /* 10 */ 0x7A, 0x7F, 0x13, 0x23, 0xF8, 0x44, 0x07, 0x30,
  /* 18 */ 0x18, 0x16, 0x6C, 0x43, 0x40, 0x91, 0x87, 0x6B,
  /* 20 */ 0xF8, 0x56, 0x10, 0xA9, 0x0A, 0x00, 0x11
};

static void RLINK_rf_init()
{
    CC2500_Strobe(CC2500_SIDLE);

    for (uint8_t i = 0; i < 39; ++i)
        CC2500_WriteReg(i, RLINK_init_values[i]);

    if (Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_DUMBO)
    {
        CC2500_WriteReg(4, 0xBA);
        CC2500_WriteReg(5, 0xDC);
    }

    freq_offset = (s8)Model.proto_opts[PROTOOPTS_FREQ];
    CC2500_WriteReg(CC2500_0C_FSCTRL0, freq_offset);
    
    CC2500_SetTxRxMode(TX_EN);
}

// Channel value -125%<->125% is scaled to 16bit value with no limit
#define CHANNEL_MAX_100 1844    //  100%
#define CHANNEL_MIN_100 204     //  100%
static u16 convert_channel_16b_nolimit(uint8_t num, int16_t min, int16_t max)
{
    s32 val = Channels[num] * 1024 / CHAN_MAX_VALUE + 1024;    // 0<->2047
    val=(val-CHANNEL_MIN_100)*(max-min)/(CHANNEL_MAX_100-CHANNEL_MIN_100)+min;
    return (u16)val;
}

static void RLINK_send_packet()
{
    static uint32_t pseudo=0;
    uint32_t bits = 0;
    uint8_t bitsavailable = 0;
    uint8_t idx = 6;

    CC2500_Strobe(CC2500_SIDLE);

    // packet length
    packet[0] = RLINK_TX_PACKET_LEN;
    // header
    if (packet_count > 3)
        packet[1] = 0x02;                   // 0x02 telemetry request flag
    else
        packet[1] = 0x00;                   // no telemetry

    switch (Model.proto_opts[PROTOOPTS_FORMAT])
    {
        case FORMAT_SURFACE:
            packet[1] |= 0x01;
            //radiolink additionnal ID which is working only on a small set of RXs
            //if(RX_num) packet[1] |= ((RX_num+2)<<4)+4;    // RX number limited to 10 values, 0 is a wildcard
            break;
        case FORMAT_AIR:
            packet[1] |= 0x21;                  // air 0x21 on dump but it looks to support telemetry at least RSSI
            break;
        case FORMAT_DUMBO:
            packet[1] = 0x00;                   // always 0x00 on dump
            break;
    }
    
    // ID
    memcpy(&packet[2],rx_tx_addr,RLINK_TX_ID_LEN);

    // pack 16 channels on 11 bits values between 170 and 1876, 1023 middle. The last 8 channels are failsafe values associated to the first 8 values.
    for (uint8_t i = 0; i < 16; i++)
    {
        uint32_t val = convert_channel_16b_nolimit(i,170,1876);       // allow extended limits
        if (val & 0x8000)
            val = 0;
        else if (val > 2047)
            val=2047;

        bits |= val << bitsavailable;
        bitsavailable += 11;
        while (bitsavailable >= 8) {
            packet[idx++] = bits & 0xff;
            bits >>= 8;
            bitsavailable -= 8;
        }
    }
    
    // hop
    pseudo=((pseudo * 0xAA) + 0x03) % 0x7673;   // calc next pseudo random value
    CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[pseudo & 0x0F]);
    packet[28]= pseudo;
    packet[29]= pseudo >> 8;
    packet[30]= 0x00;                       // unknown
    packet[31]= 0x00;                       // unknown
    packet[32]= rf_ch_num;                  // index of value changed in the RF table
    
    // check
    uint8_t sum=0;
    for (uint8_t i=1; i < 33; i++)
        sum += packet[i];
    packet[33] = sum;

    // send packet
    CC2500_WriteData(packet, RLINK_TX_PACKET_LEN+1);
    
    // packets type
    packet_count++;
    if (packet_count > 5) packet_count = 0; // restart count till next telem check

#ifdef EMULATOR
    dbgprintf("chan 0x%02x, data %02x", hopping_frequency[pseudo & 0x0f], packet[0]);
    for(int i=1; i < RLINK_TX_PACKET_LEN-1; i++) dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif
}

static uint16_t radiolink_callback()
{
    uint8_t len;

    switch(phase)
    {
        case RLINK_DATA:
            CC2500_SetPower(Model.tx_power);
            if (freq_offset != (s8)Model.proto_opts[PROTOOPTS_FREQ]) {
                freq_offset = (s8)Model.proto_opts[PROTOOPTS_FREQ];
                CC2500_WriteReg(CC2500_0C_FSCTRL0, freq_offset);
            }

            RLINK_send_packet();

            if(!(packet[1] & 0x02))
                return RLINK_TIMING_PROTO;                  // Normal packet

            // telemetry requested - check for telemetry packet
            phase++;
            return RLINK_TIMING_RFSEND;
        case RLINK_RX1:
            CC2500_Strobe(CC2500_SIDLE);
            CC2500_Strobe(CC2500_SFRX);
            CC2500_SetTxRxMode(RX_EN);
            CC2500_Strobe(CC2500_SRX);
            phase++;                                        // RX2
            return RLINK_TIMING_PROTO-RLINK_TIMING_RFSEND-RLINK_TIMING_CHECK;
        case RLINK_RX2:
            len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F; 
            if (len == RLINK_RX_PACKET_LEN + 1 + 2)         //Telemetry frame is 15 bytes + 1 byte for length + 2 bytes for RSSI&LQI&CRC
            {
                CC2500_ReadData(packet_in, len);
                if(packet_in[0]==RLINK_RX_PACKET_LEN && (packet_in[len-1] & 0x80) && memcmp(&packet[2],rx_tx_addr,RLINK_TX_ID_LEN)==0 && packet_in[6]==packet[1])
                {   // Correct telemetry received: length, CRC, ID and type
                    Telemetry.value[TELEM_FRSKY_LRSSI] = packet_in[len-2] ^ 0x80;
                    TELEMETRY_SetUpdated(TELEM_FRSKY_LRSSI);
                    Telemetry.value[TELEM_FRSKY_RSSI] = packet_in[7] & 0x7F;
                    TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
                    Telemetry.value[TELEM_FRSKY_VOLT1] = packet_in[8] << 1;    //RX Batt  // In 1/100 of Volts
                    TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
                    Telemetry.value[TELEM_FRSKY_VOLT2] = packet_in[9];         //Batt
                    TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT2);

                    pps_counter++;
                    packet_count = 0;
                }
            }
            if (CLOCK_getms() - pps_timer >= 2000)
            {   // 1 telemetry packet every 100ms
                pps_timer = CLOCK_getms();
                if (pps_counter < 20)
                    pps_counter *= 5;
                else
                    pps_counter = 100;

                Telemetry.value[TELEM_FRSKY_LQI] = pps_counter;                       //0..100%
                TELEMETRY_SetUpdated(TELEM_FRSKY_LQI);
                pps_counter = 0;
            }
            CC2500_SetTxRxMode(TX_EN);
            phase = RLINK_DATA;                               // DATA
            return RLINK_TIMING_CHECK;
    }
    return 0;
}


static void initialize()
{
    CLOCK_StopTimer();
    RLINK_TXID_init();
    RLINK_rf_init();

    CC2500_SetPower(Model.tx_power);
    packet_count = 0;
    phase = RLINK_DATA;

//    PROTOCOL_SetBindState(0);
    CLOCK_StartTimer(RLINK_TIMING_PROTO*2, radiolink_callback);
}

uintptr_t RADIOLINK_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT: initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS:
            return (uintptr_t)radiolink_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_DUMBO ? PROTO_TELEM_UNSUPPORTED : PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_FRSKY;
        case PROTOCMD_RESET:
        case PROTOCMD_DEINIT:
            CLOCK_StopTimer();
            return (CC2500_Reset() ? 1 : -1);
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
