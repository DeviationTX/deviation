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
#include "config/model.h"
#include "telemetry.h"

#ifdef PROTO_HAS_CC2500

#define USE_TUNE_FREQ

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
static u8 rf_chan;
static u8 fhss_code; // 0-27
static u8 tx_id[TX_ID_LEN];
static u8 tx_power;
static u16 counter;


#ifdef USE_TUNE_FREQ
// Frequency tuning options
static s8 coarse;
static s8 fine;
#endif


static enum {
    SFHSS_START = 0x101,
    SFHSS_CAL   = 0x102,
    SFHSS_TUNE  = 0x103,
    SFHSS_DATA1 = 0x02,
    SFHSS_DATA2 = 0x0b,
    SFHSS_MIX,
} state;

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


// Channel values are 12-bit values between 1020 and 2020, 1520 is the middle
// Futaba @140% is 2070...1520...970
// Values grow down and to the right, exact opposite to Deviation, so
// we just revert every channel
static u16 convert_channel(u8 num)
{
    u16 value = 1520 - (s32)Channels[num] * 410 / CHAN_MAX_VALUE;
    if (value > 2135)     //-150%
        value = 2135;
    else if (value < 905) //+150%
        value = 905;
    return value;
}


static void build_data_packet()
{
    u16 channel[4];
    u8 ch;
    // command.bit0 is the packet number indicator: =0 -> SFHSS_DATA1, =1 -> SFHSS_DATA2
    // command.bit1 is unknown but seems to be linked to the payload[0].bit0 but more dumps are needed: payload[0]=0x82 -> =0, payload[0]=0x81 -> =1
    // command.bit2 is the failsafe transmission indicator: =0 -> normal data, =1->failsafe data
    // command.bit3 is the channels indicator: =0 -> CH1-4, =1 -> CH5-8
    u8 command = (state == SFHSS_DATA1) ? 0 : 1; // Building packet for Data1 or Data2
    counter+=command;
    if( (counter&0x3FC) == 0x3FC )
    {	// Transmit failsafe data twice every 7s
        if( ((counter&1)^(command&1)) == 0 )
            command|=0x04; // Failsafe
    }
    else
        command|=0x02; // Assuming packet[0] == 0x81
    counter&=0x3FF; // Reset failsafe counter
    if(counter&1) command|=0x08; // Transmit lower and upper channels twice in a row
    
    u8 ch_offset = ((command&0x08) >> 1);
    if(!(command & 0x04)) { // regular channels
        channel[0] = convert_channel(ch_offset + 0);
        channel[1] = convert_channel(ch_offset + 1);
        channel[2] = convert_channel(ch_offset + 2);
        channel[3] = convert_channel(ch_offset + 3);
    }
    else { // failsafe channels
        for(ch=0; ch<4; ch++) {
            if(ch+ch_offset<Model.num_channels && (Model.limits[ch+ch_offset].flags & CH_FAILSAFE_EN)) {
                channel[ch] = 0xc00 - ((s32)Model.limits[ch+ch_offset].failsafe * 4096/1000);
                if(channel[ch] > 0xdff)
                    channel[ch] = 0xdff;
            }
            else
                channel[ch] = 0x400; // hold ?
        }
        if((command&0x08)==0) 
            channel[2]|=0x800; // special flag for throttle 
    }
    
    packet[0]  = 0x81; // can be 80, 81, 81 for Orange, only 81 for XK
    packet[1]  = tx_id[0];
    packet[2]  = tx_id[1];
    packet[3]  = 0;
    packet[4]  = 0;
    packet[5]  = (rf_chan << 3) | ((channel[0] >> 9) & 0x07);
    packet[6]  = (channel[0] >> 1);
    packet[7]  = (channel[0] << 7) | ((channel[1] >> 5) & 0x7F);
    packet[8]  = (channel[1] << 3) | ((channel[2] >> 9) & 0x07);
    packet[9]  = (channel[2] >> 1);
    packet[10] = (channel[2] << 7) | ((channel[3] >> 5) & 0x7F);
    packet[11] = (channel[3] << 3) | ((fhss_code >> 2) & 0x07);
    packet[12] = (fhss_code << 6) | command;
}


static void send_packet()
{
    tune_chan_fast();
    CC2500_WriteData(packet, sizeof(packet));
}


static u16 mixer_runtime;
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
            counter = 0;
            state = SFHSS_DATA1;
        }
        return 2000;

    /* Work cycle, 6.8ms, second packet 1.65ms after first */
    case SFHSS_DATA1:
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
        build_data_packet();
        send_packet();
        state = SFHSS_DATA2;
        return 1630;
    case SFHSS_DATA2:
        build_data_packet();
        send_packet();
        calc_next_chan();
        state = SFHSS_TUNE;
        return 2020;
    case SFHSS_TUNE:
#ifdef USE_TUNE_FREQ
        tune_freq();
#endif
        tune_power();
        state = SFHSS_MIX;
        return 3150 - mixer_runtime;

    case SFHSS_MIX:
        state = SFHSS_DATA1;
        CLOCK_RunMixer();
        return mixer_runtime;
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
    /* Initialize to neutral values so tune_freq will work */
    coarse = 0;
    fine   = 0;
#endif
    get_tx_id();
    u32 r = rand32_r(0, 0);
    fhss_code = r % 28; // Initialize it to random 0-27 inclusive
//    printf("%04x - %02x\n", fixed_id, crc);
    u16 init_timeout = rf_init();
    state = SFHSS_START;
    CLOCK_StartTimer(init_timeout, SFHSS_cb);
}

uintptr_t SFHSS_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (CC2500_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;  // Always autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 8;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS:
            return (uintptr_t)SFHSS_opts;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
