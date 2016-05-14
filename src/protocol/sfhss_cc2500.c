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
  #define SFHSS_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "config/model.h"
#include "telemetry.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CC2500

#include "iface_cc2500.h"

//#define USE_TUNE_FREQ

#ifdef USE_TUNE_FREQ
static const char * const SFHSS_opts[] = {
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  _tr_noop("Freq-Coarse"),  "-127", "127", NULL,
  NULL
};
#else
#define SFHSS_opts 0
#endif
enum {
    PROTO_OPTS_FREQFINE = 0,
    PROTO_OPTS_FREQCOARSE = 1,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define PACKET_LEN 13
#define TX_ID_LEN   2

static u8 packet[PACKET_LEN];
static u32 state;
static u8 rf_chan;
static u8 fhss_code; // 0-27
static u8 tx_id[TX_ID_LEN];
static u8 tx_power;


#ifdef USE_TUNE_FREQ
// Frequency tuning options
static s8 coarse;
static s8 fine;
#endif


enum {
    SFHSS_START = 0x101,
    SFHSS_CAL   = 0x102,
    SFHSS_TUNE  = 0x103,
    SFHSS_DATA1 = 0x02,
    SFHSS_DATA2 = 0x0b
};

#define FREQ0_VAL 0xC4

// Some important initialization parameters, all others are either default,
// or not important in the context of transmitter
// IOCFG2   2F - GDO2_INV=0 GDO2_CFG=2F - HW0
// IOCFG1   2E - GDO1_INV=0 GDO1_CFG=2E - High Impedance
// IOCFG0   2F - GDO0 same as GDO2, TEMP_SENSOR_ENABLE=off
// FIFOTHR  07 - 33 decimal TX threshold
// SYNC1    D3
// SYNC0    91
// PKTLEN   0D - Packet length, 0D bytes
// PKTCTRL1 04 - APPEND_STATUS on, all other are receive parameters - irrelevant
// PKTCTRL0 0C - No whitening, use FIFO, CC2400 compatibility on, use CRC, fixed packet length
// ADDR     29
// CHANNR   10
// FSCTRL1  06 - IF 152343.75Hz, see page 65
// FSCTRL0  00 - zero freq offset
// FREQ2    5C - synthesizer frequency 2399999633Hz for 26MHz crystal, ibid
// FREQ1    4E
// FREQ0    C4
// MDMCFG4  7C - CHANBW_E - 01, CHANBW_M - 03, DRATE_E - 0C. Filter bandwidth = 232142Hz
// MDMCFG3  43 - DRATE_M - 43. Data rate = 128143bps
// MDMCFG2  83 - disable DC blocking, 2-FSK, no Manchester code, 15/16 sync bits detected (irrelevant for TX)
// MDMCFG1  23 - no FEC, 4 preamble bytes, CHANSPC_E - 03
// MDMCFG0  3B - CHANSPC_M - 3B. Channel spacing = 249938Hz (each 6th channel used, resulting in spacing of 1499628Hz)
// DEVIATN  44 - DEVIATION_E - 04, DEVIATION_M - 04. Deviation = 38085.9Hz
// MCSM2    07 - receive parameters, default, irrelevant
// MCSM1    0C - no CCA (transmit always), when packet received stay in RX, when sent go to IDLE
// MCSM0    08 - no autocalibration, PO_TIMEOUT - 64, no pin radio control, no forcing XTAL to stay in SLEEP
// FOCCFG   1D - not interesting, Frequency Offset Compensation
// FREND0   10 - PA_POWER = 0
static const u8 init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x07, 0xD3, 0x91, 0x0D, 0x04,
  /* 08 */ 0x0C, 0x29, 0x10, 0x06, 0x00, 0x5C, 0x4E, FREQ0_VAL,
  /* 10 */ 0x7C, 0x43, 0x83, 0x23, 0x3B, 0x44, 0x07, 0x0C,
  /* 18 */ 0x08, 0x1D, 0x1C, 0x43, 0x40, 0x91, 0x57, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x11, 0x11
};

// Fast calibration table, see page 55 of swrs040c.pdf
// 31.2 Frequency Hopping and Multi-Channel Systems
static u8 rf_cal[30][3];

static void tune_chan()
{
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_0A_CHANNR, rf_chan*6+16);
    CC2500_Strobe(CC2500_SCAL);
}

static void tune_chan_fast()
{
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_0A_CHANNR, rf_chan*6+16);
    CC2500_WriteRegisterMulti(CC2500_23_FSCAL3, rf_cal[rf_chan], 3);
    usleep(6);
}

#ifdef USE_TUNE_FREQ
static void tune_freq() {
// May be we'll need this tuning routine - some receivers are more sensitive to
// frequency impreciseness, and though CC2500 has a procedure to handle it it
// may not be applied in receivers, so we need to compensate for it on TX 
    if (fine != (s8)Model.proto_opts[PROTO_OPTS_FREQFINE] || coarse != (s8)Model.proto_opts[PROTO_OPTS_FREQCOARSE]) {
        coarse = Model.proto_opts[PROTO_OPTS_FREQCOARSE];
        fine   = Model.proto_opts[PROTO_OPTS_FREQFINE];
        CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
        CC2500_WriteReg(CC2500_0F_FREQ0, FREQ0_VAL + coarse);
    }
}
#endif

static void tune_power()
{
    if (tx_power != Model.tx_power) {
        CC2500_SetPower(Model.tx_power);
        tx_power = Model.tx_power;
    }
}


static u16 rf_init()
{
    CC2500_Reset();
    CC2500_Strobe(CC2500_SIDLE);

//    for (size_t i = 0, reg = CC2500_00_IOCFG2; i < sizeof(init_values); ++i, ++reg) {
//        CC2500_WriteReg(reg, init_values[i]);
//    }
    CC2500_WriteRegisterMulti(CC2500_00_IOCFG2, init_values, sizeof(init_values));

    CC2500_SetTxRxMode(TX_EN);
    tx_power = 0xFF;
    tune_power();

    return 10000;
}

static void calc_next_chan()
{
    rf_chan += fhss_code + 2;
    if (rf_chan > 29) {
        if (rf_chan < 31) rf_chan += fhss_code + 2;
        rf_chan -= 31;
    }
}


// Channel values are 10-bit values between 86 and 906, 496 is the middle.
// Values grow down and to the right, exact opposite to Deviation, so
// we just revert every channel.
static u16 convert_channel(u8 num)
{
    s32 ch = Channels[num];
    if (ch < CHAN_MIN_VALUE) {
        ch = CHAN_MIN_VALUE;
    } else if (ch > CHAN_MAX_VALUE) {
        ch = CHAN_MAX_VALUE;
    }
    return (u16) (496 - (ch * 410 / CHAN_MAX_VALUE));
}


static void build_data_packet()
{
#define spacer1 0x02 //0b10
#define spacer2 (spacer1 << 4)
    unsigned ch_offset = state == SFHSS_DATA1 ? 0 : 4;

    u16 ch1 = convert_channel(ch_offset+0);
    u16 ch2 = convert_channel(ch_offset+1);
    u16 ch3 = convert_channel(ch_offset+2);
    u16 ch4 = convert_channel(ch_offset+3);
    
    packet[0]  = 0x81; // can be 80, 81, 81 for Orange, only 81 for XK
    packet[1]  = tx_id[0];
    packet[2]  = tx_id[1];
    packet[3]  = 0;
    packet[4]  = 0;
    packet[5]  = (rf_chan << 3) | spacer1 | ((ch1 >> 9) & 0x01);
    packet[6]  = (ch1 >> 1);
    packet[7]  = (ch1 << 7) | spacer2 | ((ch2 >> 5) & 0x1F /*0b11111*/);
    packet[8]  = (ch2 << 3) | spacer1  | ((ch3 >> 9) & 0x01);
    packet[9]  = (ch3 >> 1);
    packet[10] = (ch3 << 7) | spacer2  | ((ch4 >> 5) & 0x1F /*0b11111*/);
    packet[11] = (ch4 << 3) | ((fhss_code >> 2) & 0x07 /*0b111 */);
    packet[12] = (fhss_code << 6) | state;
}


static void send_packet()
{
    tune_chan_fast();
    CC2500_WriteData(packet, sizeof(packet));
}


static u16 SFHSS_cb()
{
    switch(state) {
    case SFHSS_START:
        rf_chan = 0;
        tune_chan();
        state = SFHSS_CAL;
        return 2000;
    case SFHSS_CAL:
        CC2500_ReadRegisterMulti(CC2500_23_FSCAL3, rf_cal[rf_chan], 3);
        if (++rf_chan < 30) {
            tune_chan();
        } else {
            rf_chan = 0;
            state = SFHSS_DATA1;
        }
        return 2000;

    /* Work cycle, 6.8ms, second packet 1.65ms after first */
    case SFHSS_DATA1:
        build_data_packet();
        send_packet();
        state = SFHSS_DATA2;
        return 1650;
    case SFHSS_DATA2:
        build_data_packet();
        send_packet();
        calc_next_chan();
        state = SFHSS_TUNE;
        return 2000;
    case SFHSS_TUNE:
        tune_power();
        state = SFHSS_DATA1;
        return 3150;
/*
    case SFHSS_DATA1:
        build_data_packet();
        send_packet();
        state = SFHSS_DATA2;
        return 1650;
    case SFHSS_DATA2:
        build_data_packet();
        send_packet();
        state = SFHSS_CAL2;
        return 500;
    case SFHSS_CAL2:
        tune_freq();
//        tune_power();
        calc_next_chan();
        tune_chan();
        state = SFHSS_DATA1;
        return 4650;
*/
    }
    return 0;
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void get_tx_id()
{
    u32 fixed_id;
    u32 lfsr = 0x7649eca9ul;

    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
    for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
        rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    u32 rand_id = rand32_r(&lfsr, 0) & 0xffff;
    // Make a guaranteed high complexity id. Some receivers (Orange)
    // behaves better if they tuned to id that has no more than 6
    //  consequtive zeos and ones
    int run_count = 0;
    // add guard for bit count
    fixed_id = 1 ^ (rand_id & 1);
    for (size_t i = 0; i < 16; ++i) {
        fixed_id = (fixed_id << 1) | (rand_id & 1);
        rand_id >>= 1;
        // If two LS bits are the same
        if ((fixed_id & 3) == 0 || (fixed_id & 3) == 3) {
            if (++run_count > 6) {
                fixed_id ^= 1;
                run_count = 0;
            }
        } else {
            run_count = 0;
        }
    }
//    fixed_id = 0xBC11;
//    fixed_id = Model.fixed_id;
    for (size_t i = 0, j = (sizeof(tx_id)-1)*8; i < sizeof(tx_id); ++i, j -= 8) {
        tx_id[i] = fixed_id >> j;
    }
}

static void initialize()
{
    CLOCK_StopTimer();
#ifdef USE_TUNE_FREQ
    coarse = (int)Model.proto_opts[PROTO_OPTS_FREQCOARSE];
    fine = Model.proto_opts[PROTO_OPTS_FREQFINE];
#endif
    get_tx_id();
    u32 r = rand32_r(0, 0);
    fhss_code = r % 28; // Initialize it to random 0-27 inclusive
//    printf("%04x - %02x\n", fixed_id, crc);
    u16 init_timeout = rf_init();
    state = SFHSS_START;
    CLOCK_StartTimer(init_timeout, SFHSS_cb);
}

const void *SFHSS_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(CC2500_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // Always autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)8L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS:
            return SFHSS_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
