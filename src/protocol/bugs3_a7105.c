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
  #define BUGS3_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include <string.h>
#include <stdlib.h>

#ifdef EMULATOR
#define USE_FIXED_MFGID
#endif

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif
#ifdef PROTO_HAS_A7105

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Leds
    CHANNEL6,     // Still camera 
    CHANNEL7,     //
    CHANNEL8,     //
    CHANNEL9,     //
    CHANNEL10,    //
    CHANNEL11,    //
};

#define CHANNEL_ARM         CHANNEL5
#define CHANNEL_LED         CHANNEL6
#define CHANNEL_PICTURE     CHANNEL7

// flags
#define FLAG_ARM     0x40    // arm (turn on motors)
#define FLAG_LED     0x80    // enable LEDs
#define FLAG_PICTURE 0x01    // take picture

static const char * const bugs3_opts[] = {
    _tr_noop("Freq Tune"), "-300", "300", "655361", NULL, // big step 10, little step 1
    NULL 
};

enum {
    PROTOOPTS_FREQTUNE,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);


#define PACKET_SIZE   22
#define NUM_RFCHAN     7

typedef struct {
  u32 radio_id;
  u8 channels[NUM_RFCHAN];
} radio_data_t;

static radio_data_t radio_data;
static u8 packet[PACKET_SIZE];
static u8 channel;
static u8 state;
static u8 packet_count;
static s16 freq_offset;

enum {
    BIND_1,
    BIND_2,
    BIND_3,
    DATA_1,
    DATA_2,
    DATA_3,
};

static int bugs3_init() {
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;
    
    A7105_WriteReg(A7105_00_MODE, 0x00);
    A7105_WriteReg(A7105_01_MODE_CONTROL, 0x42);
    A7105_WriteReg(A7105_02_CALC, 0x00);
    A7105_WriteReg(A7105_03_FIFOI, 0x15);
    A7105_WriteReg(A7105_04_FIFOII, 0x00);
    A7105_WriteReg(A7105_07_RC_OSC_I, 0x00);
    A7105_WriteReg(A7105_08_RC_OSC_II, 0x00);
    A7105_WriteReg(A7105_09_RC_OSC_III, 0x00);
    A7105_WriteReg(A7105_0A_CK0_PIN, 0x00);
    A7105_WriteReg(A7105_0D_CLOCK, 0x05);
    A7105_WriteReg(A7105_0E_DATA_RATE, 0x01);
    A7105_WriteReg(A7105_0F_PLL_I, 0x50);
    A7105_WriteReg(A7105_10_PLL_II, 0x9e);
    A7105_WriteReg(A7105_11_PLL_III, 0x4b);
    A7105_WriteReg(A7105_12_PLL_IV, 0x00);
    A7105_WriteReg(A7105_13_PLL_V, 0x02);
    A7105_WriteReg(A7105_14_TX_I, 0x16);
    A7105_WriteReg(A7105_15_TX_II, 0x2b);
    A7105_WriteReg(A7105_16_DELAY_I, 0x12);
    A7105_WriteReg(A7105_17_DELAY_II, 0x40);
    A7105_WriteReg(A7105_18_RX, 0x62);
    A7105_WriteReg(A7105_19_RX_GAIN_I, 0x80);
    A7105_WriteReg(A7105_1A_RX_GAIN_II, 0x80);
    A7105_WriteReg(A7105_1B_RX_GAIN_III, 0x00);
    A7105_WriteReg(A7105_1C_RX_GAIN_IV, 0x0a);
    A7105_WriteReg(A7105_1D_RSSI_THOLD, 0x32);
    A7105_WriteReg(A7105_1E_ADC, 0xc3);
    A7105_WriteReg(A7105_1F_CODE_I, 0x0f);
    A7105_WriteReg(A7105_20_CODE_II, 0x16);
    A7105_WriteReg(A7105_21_CODE_III, 0x00);
    A7105_WriteReg(A7105_22_IF_CALIB_I, 0x00);
    A7105_WriteReg(A7105_24_VCO_CURCAL, 0x00);
    A7105_WriteReg(A7105_25_VCO_SBCAL_I, 0x00);
    A7105_WriteReg(A7105_26_VCO_SBCAL_II, 0x3b);
    A7105_WriteReg(A7105_27_BATTERY_DET, 0x00);
    A7105_WriteReg(A7105_29_RX_DEM_TEST_I, 0x47);
    A7105_WriteReg(A7105_2A_RX_DEM_TEST_II, 0x80);
    A7105_WriteReg(A7105_2B_CPC, 0x03);
    A7105_WriteReg(A7105_2C_XTAL_TEST, 0x01);
    A7105_WriteReg(A7105_2D_PLL_TEST, 0x45);
    A7105_WriteReg(A7105_2E_VCO_TEST_I, 0x18);
    A7105_WriteReg(A7105_2F_VCO_TEST_II, 0x00);
    A7105_WriteReg(A7105_30_IFAT, 0x01);
    A7105_WriteReg(A7105_31_RSCALE, 0x0f);

    A7105_Strobe(A7105_STANDBY);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    A7105_ReadReg(0x02);
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while (CLOCK_getms() - ms < 500) {
        if (!A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    if_calibration1 = A7105_ReadReg(A7105_22_IF_CALIB_I);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    A7105_WriteReg(A7105_24_VCO_CURCAL, 0x13);
    A7105_WriteReg(A7105_26_VCO_SBCAL_II, 0x3b);

    //VCO Calibration
    A7105_WriteReg(A7105_0F_CHANNEL, 0);
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration0 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Set Channel
    A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
    //VCO Calibration
    A7105_WriteReg(A7105_02_CALC, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while (CLOCK_getms()  - ms < 500) {
        if ( !A7105_ReadReg(A7105_02_CALC))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration1 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
    }

    //Reset VCO Band calibration
    A7105_WriteReg(A7105_25_VCO_SBCAL_I, 0x0e);

    A7105_SetTxRxMode(TX_EN);

    A7105_SetPower(0x0b);   // bind in low-power (from spi capture)
    
    freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
    A7105_AdjustLOBaseFreq(freq_offset);

    A7105_WriteID(radio_data.radio_id);
    A7105_Strobe(A7105_STANDBY);
    return 1;
}

static s16 get_channel(u8 ch, s32 scale, s32 center, s32 range) {
    s32 value = (s32)Channels[ch] * scale / CHAN_MAX_VALUE + center;
    if (value < center - range)
        value = center - range;
    if (value >= center + range)
        value = center + range -1;
    return value;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

static void build_packet(u8 bind) {
    memset(packet, 0, sizeof(packet));
    packet[0] = 0x34 + ((packet_count & 0x1) << 6) + (bind ? 0x80 : 0);
    packet[1] = 0x24;
    packet[2] = 0x29;
    packet[3] = 0x74;
    packet[4] = 0x04 + ((packet_count & 0x1) << 6) + (bind ? 0x80 : 0);
    if (bind)
      packet[5] = 0x40;
    else
      packet[5] = (FLAG_ARM & ~GET_FLAG(CHANNEL_ARM, FLAG_ARM))     // 0x00 armed, 0x40 is disarmed
              + GET_FLAG(CHANNEL_PICTURE, FLAG_PICTURE)
              + GET_FLAG(CHANNEL_LED,     FLAG_LED);
    packet[6] = get_channel(CHANNEL1, 100, 100, 100);
    packet[7] = get_channel(CHANNEL2, 100, 100, 100);
    packet[8] = get_channel(CHANNEL3, 100, 100, 100);
    packet[9] = get_channel(CHANNEL4, 100, 100, 100);
    packet[10] = 100;
    packet[11] = 100;
    packet[12] = 100;
    packet[13] = 100;

//    packet[14] = 0;
//    packet[15] = 0;

    // try driven trims
    packet[16] = packet[6] / 2 + 15;
    packet[17] = packet[7] / 2 + 15;
    packet[18] = 64;
    packet[19] = packet[9] / 2 + 15;

//    packet[20] = 0;
//    packet[21] = 0;

//TODO
#if 0
printf("packet ");
for (int i=0; i < PACKET_SIZE; i++) {
  printf("%02x ", packet[i]);
}
printf("\n");
#endif
//TODO

}

#ifdef REAL
// Generate internal id from TX id and manufacturer id (STM32 unique id)
static int get_tx_id()
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
#endif

static void set_radio_data(u8 index) {
    radio_data_t fixed_radio_data[] = {
            {0xac59a453, {0x1d, 0x3b, 0x4d, 0x29, 0x11, 0x2d, 0x63}},     // bind phase
            {0x56926d94, {0x27, 0x4b, 0x1c, 0x63, 0x25, 0x0a, 0x57}}, };  // data phase if rx responds 1d 5b 2c 7f 00 00 00 00 00 00 ff 87 00 00 00 00
    memcpy(&radio_data, &fixed_radio_data[index], sizeof(radio_data_t));
}


MODULE_CALLTYPE
static u16 bugs3_cb() {
    u16 packet_period = 0;
    u8 mode;

    // keep frequency tuning updated
    if (freq_offset != Model.proto_opts[PROTOOPTS_FREQTUNE]) {
        freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
        A7105_AdjustLOBaseFreq(freq_offset);
    }

    switch(state) {
    case BIND_1:
//TODO
#if 0
printf("state %d, channel %02x\n", state, radio_data.channels[channel]);
#endif
//TODO
        build_packet(1);
        A7105_Strobe(A7105_STANDBY);
        A7105_WriteData(packet, PACKET_SIZE, radio_data.channels[channel]);
        state = BIND_2;
        packet_period = 1168;
        break;

    case BIND_2:
//TODO
#if 0
printf("state %d, channel %02x\n", state, radio_data.channels[channel]-2);
#endif
//TODO
        A7105_Strobe(A7105_STANDBY);
        A7105_SetTxRxMode(RX_EN);
        A7105_WriteReg(A7105_0F_PLL_I, radio_data.channels[channel] - 2);
        A7105_Strobe(A7105_RX);
        packet_count += 1;
        channel += packet_count & 1;
        channel %= NUM_RFCHAN;
        state = BIND_3;
        packet_period = 3850;
        break;

    case BIND_3:
        A7105_Strobe(A7105_STANDBY);
        A7105_SetTxRxMode(TX_EN);
        mode = A7105_ReadReg(A7105_00_MODE);
//TODO
#if 0
printf("state %d, radio_id %04lx, mode %02x\n", state, radio_data.radio_id, mode);
#endif
//TODO
        if (mode & 0x01) {
            state = BIND_1;
            packet_period = 20;         // No received data so restart binding procedure.
            break;
        }
        A7105_ReadData(packet, 16);
//TODO
#if 0
printf("received ");
for (int i=0; i < 16; i++) {
  printf("%02x ", packet[i]);
}
printf("\n");
#endif
//TODO
if ((packet[0] + packet[1] + packet[2] + packet[3]) == 0) {
    state = BIND_1;
    packet_period = 20;         // No received data so restart binding procedure.
    break;
}
        set_radio_data(1);
        A7105_WriteID(radio_data.radio_id);
        PROTOCOL_SetBindState(0);
        state = DATA_1;
        channel = 0;
        packet_period = 500;
        break;

    case DATA_1:
//TODO
#if 0
printf("state %d, channel %02x\n", state, radio_data.channels[channel]);
#endif
//TODO
        A7105_SetPower(Model.tx_power);
        build_packet(0);
        A7105_Strobe(A7105_STANDBY);
        A7105_WriteData(packet, PACKET_SIZE, radio_data.channels[channel]);
        state = DATA_2;
        packet_period = 1168;
        break;

    case DATA_2:
//TODO
#if 0
printf("state %d, channel %02x\n", state, radio_data.channels[channel]-2);
#endif
//TODO
        A7105_Strobe(A7105_STANDBY);
        A7105_SetTxRxMode(RX_EN);
        A7105_WriteReg(A7105_0F_PLL_I, radio_data.channels[channel] - 2);
        A7105_Strobe(A7105_RX);
        packet_count += 1;
        channel += packet_count & 1;
        channel %= NUM_RFCHAN;
        state = DATA_3;
        packet_period = 3850;
        break;

    case DATA_3:
        A7105_Strobe(A7105_STANDBY);
        A7105_SetTxRxMode(TX_EN);
        mode = A7105_ReadReg(A7105_00_MODE);
//TODO
#if 0
printf("state %d, radio_id %04lx, mode %02x\n", state, radio_data.radio_id, mode);
#endif
//TODO
        if (!(mode & 0x01)) {
            A7105_ReadData(packet, 16);
//TODO
#if 0
printf("received ");
for (int i=0; i < 16; i++) {
  printf("%02x ", packet[i]);
}
printf("\n");
#endif
//TODO
        }
        state = DATA_1;
        packet_period = 20;
        break;
    }
#ifndef EMULATOR
    return packet_period;
#else
    return packet_period / 200;
#endif
}


static void initialize(u8 bind) {
    CLOCK_StopTimer();

    if (bind) {
        set_radio_data(0);
        state = BIND_1;
        PROTOCOL_SetBindState(0xFFFFFFFF);
    } else {
        set_radio_data(1);
        state = DATA_1;
    }

    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (bugs3_init())
            break;
    }
    
    channel = 0;
    packet_count = 0;
    CLOCK_StartTimer(100, bugs3_cb);
}

const void *BUGS3_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT: initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(A7105_Reset() ? 1L : -1L);
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)7L; // A, E, T, R, Arm, Pic, Led
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)7L;
        case PROTOCMD_CURRENT_ID: return 0;
        case PROTOCMD_GETOPTIONS:
            return bugs3_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
