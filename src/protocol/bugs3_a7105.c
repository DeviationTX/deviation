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
#include "music.h"
#include "config/model.h"
#include "telemetry.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

//////////// rxid -> radioid algorithm //////////////////////////////
// Hex digit 1 is periodic with length 2, and hex digit 2 is periodic
// with length 16. However, storing the byte of those 2 digits
// instead of manipulating bits results simpler code and smaller binary.
static u8 most_popular_01_cycle[16];
static void load_most_popular_01_cycle() {
  u8 data[] = { 0x52, 0xac, 0x59, 0xa4, 0x53, 0xab, 0x57, 0xa9,
                0x56, 0xa5, 0x5b, 0xa7, 0x5d, 0xa6, 0x58, 0xad};
  memcpy(most_popular_01_cycle, data, sizeof data);
};

static u8 most_popular_01(u8 i) {
  return most_popular_01_cycle[i % 16];
}

static u8 most_popular_67_cycle[37];
static void load_most_popular_67_cycle() {
    u8 data[] = { 0x34, 0xc5,
                  0x6a, 0xb4,
                  0x29, 0xd5,
                  0x2c, 0xd3,
                  0x91, 0xb3,
                  0x6c, 0x49,
                  0x52, 0x9c,
                  0x4d, 0x65,
                  0xc3, 0x4a,
                  0x5b, 0xd6,
                  0x92, 0x6d,
                  0x94, 0xa6,
                  0x55, 0xcd,
                  0x2b, 0x9a,
                  0x36, 0x95,
                  0x4b, 0xd4,
                  0x35, 0x8d,
                  0x96, 0xb2,
                  0xa3
    };
    memcpy(most_popular_67_cycle, data, sizeof data);
};

static u8 most_popular_67(u8 i) {
  if (i == 0) {
    return 0xd2;
  } else if (i == 1) {
    return 0xda;
  } else if (i % 16 < 2) {
    u8 ii = 2 * (i / 16) + i % 16 - 2;
    if (ii % 2 == 0) {
      return most_popular_67_cycle[ii + 7];
    } else {
      return most_popular_67_cycle[ii];
    }
  } else {
    return most_popular_67_cycle[2 * (i / 16) + (i % 16 - 2) % 7];
  }
}

static u8 most_popular_45(u8 i) {
  if (i == 0) {
    return 0xa3;
  } else if (i == 1) {
    return 0x86;
  } else {
    u8 ii = i;
    if (i % 8 == 1) {
      ii -= 8;
    } else {
      ii -= 1;
    }
    return most_popular_67(ii);
  }
}

static u8 most_popular_23(u8 i) {
  if (i == 0) {
    return 0xb2;
  } else if (i == 1) {
    return 0xcb;
  } else {
    u8 ii = i;
    if (i % 8 == 1) {
      ii -= 8;
    } else {
      ii -= 1;
    }
    return most_popular_45(ii);
  }
}

static u32 most_popular(u8 i) {
  u32 ii = i + !(i <= 127);
  return (u32) most_popular_01(ii) << 24 |
    (u32) most_popular_23(ii) << 16 |
    (u32) most_popular_45(ii) << 8 |
    most_popular_67(ii);
}

static u32 second_most_popular(u8 i) {
  if (i < 127) {
    return most_popular(i + 1);
  } else if (i > 128) {
    return most_popular(i - 1);
  } else {
    return 0x52d6926d;
  }
}

// The 22 irregular values do not match the above periodicities. They might be
// errors from the readout, but let us try them here as long as it is not
// proven.
static u16 irregular_keys[22];
static void load_irregular_keys() {
    u16 data[] = { 1131, 1287, 2842, 4668, 5311, 11594, 13122, 13813,
                  20655, 22975, 25007, 25068, 28252, 33309, 35364, 35765,
                  37731, 40296, 43668, 46540, 49868, 65535
    };
    memcpy(irregular_keys, data, sizeof data);
};

static u32 irregular_values[22];
static void load_irregular_values() {
    u32 data[] = { 0x52d6926d, 0xa586da34, 0x5329d52c, 0xa66c4952,
                   0x536c4952, 0x524a5bd6, 0x534d65c3, 0xa9d391b3,
                   0x5249529c, 0xa555cd2b, 0xac9a3695, 0x58d391b3,
                   0xa791b36c, 0x53926d94, 0xa7926d94, 0xa72cd391,
                   0xa9b429d5, 0x5629d52c, 0xad2b9a36, 0xa74d65c3,
                   0x526d94a6, 0xad96b2a3
    };
    memcpy(irregular_values, data, sizeof data);
};

static u32 is_irregular(u32 i) {
  for (u8 j = 0; j < sizeof(irregular_keys) / sizeof(irregular_keys[0]); ++j) {
    if (irregular_keys[j] == i) {
      return irregular_values[j];
    }
  }
  return 0;
}

static u32 rxid_to_radioid(u16 rxid) {
  u8 block = rxid / 256;
  u8 second_seq_size;
  bool use_most_popular;
  if (rxid < 32768) {
    second_seq_size = 128 - block;
    use_most_popular = rxid % 256 >= second_seq_size;
  } else {
    second_seq_size = block - 127;
    use_most_popular = 255 - rxid % 256 >= second_seq_size;
  }
  u32 v = is_irregular(rxid);
  if (!v) {
    if (use_most_popular) {
      v = most_popular(rxid % 255);
    } else {
      v = second_most_popular(rxid % 255);
    }
  }
  return v;
}
//////////// rxid -> radioid algorithm //////////////////////////////

#ifdef EMULATOR
#define USE_FIXED_MFGID
#else
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
    CHANNEL5,     // Arm
    CHANNEL6,     // LEDs
    CHANNEL7,     // Flip
    CHANNEL8,     //
    CHANNEL9,     //
    CHANNEL10,    //
    CHANNEL11,    //
};

#define CHANNEL_ARM         CHANNEL5
#define CHANNEL_LED         CHANNEL6
#define CHANNEL_FLIP        CHANNEL7
#define CHANNEL_PICTURE     CHANNEL8
#define CHANNEL_VIDEO       CHANNEL9

// flags packet byte 4
#define FLAG_FLIP    0x08    // automatic flip
#define FLAG_MODE    0x04    // low/high speed select (set is high speed)
#define FLAG_VIDEO   0x02    // toggle video
#define FLAG_PICTURE 0x01    // toggle picture

// flags packet byte 5
#define FLAG_LED     0x80    // enable LEDs
#define FLAG_ARM     0x40    // arm (toggle to turn on motors)
#define FLAG_DISARM  0x20    // disarm (toggle to turn off motors)

#define RXID_MAX  65535
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
#define NUM_RFCHAN    16

typedef struct {
  u32 radio_id;
  u8 channels[NUM_RFCHAN];
} radio_data_t;

static radio_data_t radio_data;
static u8 packet[PACKET_SIZE];
static u8 channel_idx;
static u8 state;
static u8 packet_count;
static u8 armed, arm_flags;
static u8 arm_channel_previous;
static s16 freq_offset;

enum {
    BIND_1,
    BIND_2,
    BIND_3,
    DATA_1,
    DATA_2,
    DATA_3,
};

static u8 a7105_wait_cal() {
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while (CLOCK_getms() - ms < 500)
        if (!A7105_ReadReg(0x02)) return 1;
    return 0;
}

static void a7105_SetVCOBand(u8 vb1, u8 vb2) {
    u8 diff1, diff2;

    if (vb1 >= 4)
        diff1 = vb1 - 4;
    else
        diff1 = 4 - vb1;

    if (vb2 >= 4)
        diff2 = vb2 - 4;
    else
        diff2 = 4 - vb2;

    if (diff1 == diff2 || diff1 > diff2)
        A7105_WriteReg(A7105_25_VCO_SBCAL_I, vb1 | 0x08);
    else
        A7105_WriteReg(A7105_25_VCO_SBCAL_I, vb2 | 0x08);
}

// A7105 calibration code ported from reference code application note
static u8 a7105_calibrate() {
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;

    // IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    if (!a7105_wait_cal()) return 0;
    if_calibration1 = A7105_ReadReg(A7105_22_IF_CALIB_I);
    if(if_calibration1 & A7105_MASK_FBCF) {
        return 0;
    }

    // Manual setting for VCO current
    A7105_WriteReg(A7105_24_VCO_CURCAL, 0x13);

    // VCO Band Calibration
    A7105_WriteReg(A7105_26_VCO_SBCAL_II, 0x3b); // same as default value, but write is in reference code
    // First get calibration value at 2400 MHz
    A7105_WriteReg(A7105_0F_CHANNEL, 0);
    A7105_WriteReg(0x02, 2);
    if (!a7105_wait_cal()) return 0;
    vco_calibration0 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        return 0;
    }

    // Then 2480 MHz
    A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
    //VCO Calibration
    A7105_WriteReg(A7105_02_CALC, 2);
    if (!a7105_wait_cal()) return 0;
    vco_calibration1 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        return 0;
    }

    // Then set calibration band value to best match
    a7105_SetVCOBand(vco_calibration0 & 0x07, vco_calibration1 & 0x07);

    return 1;
}


static int bugs3_init(u8 bind) {
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
    A7105_WriteReg(A7105_28_TX_TEST, 0x0b);
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

    if (!a7105_calibrate()) return 0;

    A7105_SetTxRxMode(TX_EN);

    freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
    A7105_AdjustLOBaseFreq(freq_offset);

    if (bind)
        A7105_WriteID(radio_data.radio_id);
    else
        A7105_WriteID(Model.fixed_id);

    A7105_Strobe(A7105_STANDBY);
    return 1;
}

static void update_telemetry(u8 *data) {
  Telemetry.value[TELEM_FRSKY_RSSI] = data[3];
  TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
  Telemetry.value[TELEM_FRSKY_VOLT1] = data[10] == 0xff ? 840 : 600;
  TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
}

static u16 get_channel(u8 ch, s32 scale, s32 center, s32 range) {
    s32 value = (s32)Channels[ch] * scale / CHAN_MAX_VALUE + center;
    if (value < center - range) value = center - range;
    if (value >= center + range) value = center + range;
    return value;
}

static u8 get_checkbyte() {
    u8 check = 0x6d;
    u8 i;

    for (i=1; i < PACKET_SIZE; i++)
        check ^= packet[i];

    return check;
}

static void check_arming(s32 channel_value) {
    u8 arm_channel = channel_value > 0;

    if (arm_channel != arm_channel_previous) {
        arm_channel_previous = arm_channel;
        if (arm_channel) {
            armed = 1;
            arm_flags ^= FLAG_ARM;
        } else {
            armed = 0;
            arm_flags ^= FLAG_DISARM;
        }
    }
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

static void build_packet(u8 bind) {
    u8 force_values = bind | !armed;
    u8 change_channel = ((packet_count & 0x1) << 6);
    u16 aileron  = get_channel(CHANNEL1, -400, 400, 400);
    u16 elevator = get_channel(CHANNEL2, -400, 400, 400);
    u16 throttle = get_channel(CHANNEL3,  400, 400, 400);
    u16 rudder   = get_channel(CHANNEL4, -400, 400, 400);

    memset(packet, 0, sizeof(packet));
    packet[1] = 0x76;   // txid (rx uses to know hopping frequencies)
    packet[2] = 0x71;
    packet[3] = 0x94;

    check_arming(Channels[CHANNEL_ARM]);  // sets globals arm_flags and armed
    if (bind) {
      packet[4] = change_channel | 0x80;
      packet[5] = 0x06 | arm_flags;
    } else {
      packet[4] = change_channel | FLAG_MODE
                | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP)
                | GET_FLAG(CHANNEL_PICTURE, FLAG_PICTURE)
                | GET_FLAG(CHANNEL_VIDEO, FLAG_VIDEO);
      packet[5] = 0x06 | arm_flags
                | GET_FLAG(CHANNEL_LED, FLAG_LED);
    }

    packet[6] = force_values ? 100 : (aileron  >> 2);
    packet[7] = force_values ? 100 : (elevator >> 2);
    packet[8] = force_values ?   0 : (throttle >> 2);
    packet[9] = force_values ? 100 : (rudder   >> 2);
    packet[10] = 100;
    packet[11] = 100;
    packet[12] = 100;
    packet[13] = 100;

    packet[14] = ((aileron  << 6) & 0xc0)
               | ((elevator << 4) & 0x30)
               | ((throttle << 2) & 0x0c)
               | ((rudder       ) & 0x03);

//    packet[15] = 0;

    // driven trims
    packet[16] = aileron / 8 + 14;
    packet[17] = elevator / 8 + 14;
    packet[18] = 64;
    packet[19] = rudder / 8 + 14;

//    packet[20] = 0;
//    packet[21] = 0;

    packet[0] = get_checkbyte();
}

static void set_radio_data(u8 index) {
    // captured radio data for bugs rx/tx version A2
    // it appears that the hopping frequencies are determined by the txid
    // and the data phase radio id is determined by the first 2 bytes of the
    // rx bind packet
    radio_data_t fixed_radio_data[] = {
            // bind phase
            {0xac59a453, {0x1d, 0x3b, 0x4d, 0x29, 0x11, 0x2d, 0x0b, 0x3d,
                          0x59, 0x48, 0x17, 0x41, 0x23, 0x4e, 0x2a, 0x63}},
            // data phase for txid 767194 if rx responds C6 BB 57 7F 00 00 00 00 00 00 FF 87 40 00 00 00
            {0xA4C56AB4, {0x4b, 0x19, 0x35, 0x1e, 0x63, 0x0f, 0x45, 0x21,
                          0x51, 0x3a, 0x5d, 0x25, 0x0a, 0x44, 0x61, 0x27}},
            // data phase for txid 767194 if rx responds A3 1E D7 7f 00 00 00 00 00 00 ff 87 00 00 00 00
            {0x57358d96, {0x4b, 0x19, 0x35, 0x1e, 0x63, 0x0f, 0x45, 0x21,
                          0x51, 0x3a, 0x5d, 0x25, 0x0a, 0x44, 0x61, 0x27}},
            // data phase if rx responds 1d 5b 2c 7f 00 00 00 00 00 00 ff 87 00 00 00 00
// A1 version capture            {0x56926d94, {0x27, 0x4b, 0x1c, 0x63, 0x25, 0x0a, 0x57}},
    };
    memcpy(&radio_data, &fixed_radio_data[index], sizeof(radio_data_t));
}

static void increment_counts() {
    // this logic works with the use of packet_count in build_packet
    // to properly indicate channel changes to rx
    packet_count += 1;
    if ((packet_count & 1) == 0) {
        channel_idx += 1;
        channel_idx %= NUM_RFCHAN;
    }
}

#define DELAY_POST_TX   1100
#define DELAY_WAIT_TX    500
#define DELAY_WAIT_RX   2000
#define DELAY_POST_RX   2000
#define DELAY_BIND_RST   200
// FIFO config is one less than desired value
#define FIFO_SIZE_RX      15
#define FIFO_SIZE_TX      21
MODULE_CALLTYPE
static u16 bugs3_cb() {
    u16 packet_period = 0;
    u8 mode;
    u8 count;

    // keep frequency tuning updated
    if (freq_offset != Model.proto_opts[PROTOOPTS_FREQTUNE]) {
        freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
        A7105_AdjustLOBaseFreq(freq_offset);
    }

    switch(state) {
    case BIND_1:
        build_packet(1);
        A7105_Strobe(A7105_STANDBY);
        A7105_WriteReg(A7105_03_FIFOI, FIFO_SIZE_TX);
        A7105_WriteData(packet, PACKET_SIZE, radio_data.channels[channel_idx]);
        state = BIND_2;
        packet_period = DELAY_POST_TX;
        break;

    case BIND_2:
        // wait here a bit for tx complete because
        // need to start rx immediately to catch return packet
        count = 20;
#ifndef EMULATOR
        while (A7105_ReadReg(A7105_00_MODE) & 0x01) {
            if (count-- == 0) {
                packet_period = DELAY_WAIT_TX;  // don't proceed until transmission complete
                break;
            }
        }
#endif
        A7105_SetTxRxMode(RX_EN);
        A7105_WriteReg(A7105_0F_PLL_I, radio_data.channels[channel_idx] - 2);
        A7105_WriteReg(A7105_03_FIFOI, FIFO_SIZE_RX);
        A7105_Strobe(A7105_RX);

        increment_counts();
        state = BIND_3;
        packet_period = DELAY_WAIT_RX;
        break;

    case BIND_3:
        mode = A7105_ReadReg(A7105_00_MODE);
        A7105_Strobe(A7105_STANDBY);
        A7105_SetTxRxMode(TX_EN);

#ifndef EMULATOR
        if (mode & 0x01) {
            state = BIND_1;
            packet_period = DELAY_BIND_RST;         // No received data so restart binding procedure.
            break;
        }
        A7105_ReadData(packet, 16);
        if ((packet[0] + packet[1] + packet[2] + packet[3]) == 0) {
            state = BIND_1;
            packet_period = DELAY_BIND_RST;         // No received data so restart binding procedure.
            break;
        }
#endif

        A7105_Strobe(A7105_STANDBY);

        // set fixed_id to radio_id
        u16 rxid = (packet[1] << 8) + packet[2];
        u32 radioid = rxid_to_radioid(rxid);
        Model.fixed_id = radioid;
        set_radio_data(1);
        A7105_WriteID(radioid);

        PROTOCOL_SetBindState(0);
        state = DATA_1;
        packet_count = 0;
        channel_idx = 0;
        packet_period = DELAY_POST_RX;
        break;

    case DATA_1:
        A7105_SetPower(Model.tx_power);
        build_packet(0);
        A7105_WriteReg(A7105_03_FIFOI, FIFO_SIZE_TX);
        A7105_WriteData(packet, PACKET_SIZE, radio_data.channels[channel_idx]);
        state = DATA_2;
        packet_period = DELAY_POST_TX;
        break;

    case DATA_2:
        // wait here a bit for tx complete because
        // need to start rx immediately to catch return packet
        count = 20;
        while (A7105_ReadReg(A7105_00_MODE) & 0x01) {
            if (count-- == 0) {
                packet_period = DELAY_WAIT_TX;  // don't proceed until transmission complete
                break;
            }
        }
        A7105_SetTxRxMode(RX_EN);
        A7105_WriteReg(A7105_0F_PLL_I, radio_data.channels[channel_idx] - 2);
        A7105_WriteReg(A7105_03_FIFOI, FIFO_SIZE_RX);
        A7105_Strobe(A7105_RX);

        increment_counts();
        state = DATA_3;
        packet_period = DELAY_WAIT_RX;
        break;

    case DATA_3:
        mode = A7105_ReadReg(A7105_00_MODE);
        A7105_Strobe(A7105_STANDBY);
        A7105_SetTxRxMode(TX_EN);

        if (!(mode & 0x01)) {
            A7105_ReadData(packet, 16);
            update_telemetry(packet);
        }
        state = DATA_1;
        packet_period = DELAY_POST_RX;
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

    // support for auto rxid -> radioid
    load_most_popular_01_cycle();
    load_most_popular_67_cycle();
    load_irregular_keys();
    load_irregular_values();

    if (bind) {
        set_radio_data(0);
        state = BIND_1;
        PROTOCOL_SetBindState(0xFFFFFFFF);
    } else {
        set_radio_data(1);
        state = DATA_1;
    }

    A7105_Reset();
    CLOCK_ResetWatchdog();
    while(!bugs3_init(bind)) {
        MUSIC_Play(MUSIC_ALARM1);
        A7105_Reset();
        CLOCK_ResetWatchdog();
    }

    channel_idx = 0;
    packet_count = 0;
    armed = 0;
    arm_flags = FLAG_DISARM;    // initial value from captures
    arm_channel_previous = Channels[CHANNEL_ARM] > 0;
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
        case PROTOCMD_NUMCHAN: return (void *)9L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)9L;
        case PROTOCMD_CURRENT_ID: return 0;
        case PROTOCMD_GETOPTIONS:
            return bugs3_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(PROTO_TELEM_ON);
        case PROTOCMD_TELEMETRYTYPE:
            return (void *)(long) TELEM_FRSKY;
        default: break;
    }
    return 0;
}
#endif
