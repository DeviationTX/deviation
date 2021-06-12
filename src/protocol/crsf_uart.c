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
#include "crsf.h"
#include "pages.h"
#if HAS_EXTENDED_TELEMETRY
#include "telemetry.h"
#endif

#define CRSF_DATARATE             400000
#define CRSF_FRAME_PERIOD         4000   // 4ms
#define CRSF_CHANNELS             16
#define CRSF_PACKET_SIZE          26

static u8 currentPktRate = 0;
static u8 currentTlmRatio = 0;
static u8 currentPower = 0;
static u8 currentBind = 0;
static u8 currentWiFi = 0;
static u8 getParamsCounter = 0;

static const char * const expresslrs_opts_24[] = {
  _tr_noop("Bad Pkts"), "0", "1000", "1", NULL,
  _tr_noop("Good Pkts"), "0", "1000", "1", NULL,
    //                      0        1      3        5       6
  _tr_noop("Pkt. Rate"),  "500Hz", "250Hz", "150Hz", "50Hz", "25Hz", NULL,
  //                      0      1        2       3       4       5      6      7
  _tr_noop("TLM Ratio"),  "OFF", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2", NULL,
  //                  0       1       2       3        4        5        6         7
  _tr_noop("Power"),  "10mW", "25mW", "50mW", "100mW", "250mW", "500mW", "1000mW", "2000mW", NULL,
  //                   0    1         2          3         4         5         6
  _tr_noop("Rf Freq"), "?", "915 AU", "915 FCC", "868 EU", "433 AU", "433 EU", "2.4G ISM", NULL,
  _tr_noop("Bind"),  "OFF", "ON", NULL,
  _tr_noop("WiFi Update"),  "OFF", "ON", NULL,
  NULL
};

static const char * const expresslrs_opts_900[] = {
  _tr_noop("Bad Pkts"), "0", "1000", "1", NULL,
  _tr_noop("Good Pkts"), "0", "1000", "1", NULL,
    //                    2        4        5       6
  _tr_noop("Pkt. Rate"),  "200Hz", "100Hz", "50Hz", "25Hz", NULL,
  //                      0      1        2       3       4       5      6      7
  _tr_noop("TLM Ratio"),  "OFF", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2", NULL,
  //                  0       1       2       3        4        5        6         7
  _tr_noop("Power"),  "10mW", "25mW", "50mW", "100mW", "250mW", "500mW", "1000mW", "2000mW", NULL,
  //                   0    1         2          3         4         5         6
  _tr_noop("Rf Freq"), "?", "915 AU", "915 FCC", "868 EU", "433 AU", "433 EU", "2.4G ISM", NULL,
  _tr_noop("Bind"),  "OFF", "ON", NULL,
  _tr_noop("WiFi Update"),  "OFF", "ON", NULL,
  NULL
};

static u8 convertPktRateToElrs(u8 rfFreq, u8 rate)
{
  switch (rate) {
    case 0:
      if (rfFreq == 6) return 0;
      return 2;
    case 1:
      if (rfFreq == 6) return 1;
      return 4;
    case 2:
      if (rfFreq == 6) return 3;
      return 5;
    case 3:
      if (rfFreq == 6) return 5;
      return 6;
    case 4: return 6;
  }
  return 6;
}

static u8 convertElrsToPktRate(u8 rfFreq, u8 elrsRate)
{
  switch (elrsRate) {
    case 0: return 0;   // 2.4
    case 1: return 1;   // 2.4
    case 2: return 0;   // 900
    case 3: return 2;   // 2.4
    case 4: return 1;   // 900
    case 5:
      if (rfFreq == 6) return 3;
      return 2;
    case 6:
      if (rfFreq == 6) return 4;
      return 3;
  }
  return 0;
}

static u16 convertPktRateToPeriod(u8 rfFreq, u8 rate)
{
  if (rfFreq == 0) return CRSF_FRAME_PERIOD;
  switch (rate) {
    case 0:
      if (rfFreq == 6) return 2000;
      return 5000;
    case 1:
      if (rfFreq == 6) return 4000;
      return 10000;
    case 2:
      if (rfFreq == 6) return 6666;
      return 20000;
    case 3:
      if (rfFreq == 6) return 20000;
      return 40000;
    case 4: return 40000;
  }
  return CRSF_FRAME_PERIOD;
}

enum {
    PROTO_OPTS_BAD_PKTS,
    PROTO_OPTS_GOOD_PKTS,
    PROTO_OPTS_PKT_RATE,
    PROTO_OPTS_TLM_RATIO,
    PROTO_OPTS_POWER,
    PROTO_OPTS_RF_FREQ,
    PROTO_OPTS_BIND,
    PROTO_OPTS_WIFI_UPDATE,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

// crc implementation from CRSF protocol document rev7
static u8 crsf_crc8tab[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

u8 crsf_crc8(const u8 *ptr, u8 len) {
    u8 crc = 0;
    for (u8 i=0; i < len; i++) {
        crc = crsf_crc8tab[crc ^ *ptr++];
    }
    return crc;
}


#if HAS_EXTENDED_TELEMETRY
static u8 telemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];
static u8 telemetryRxBufferCount;

static void set_telemetry(crossfire_telem_t offset, s32 value) {
    Telemetry.value[offset] = value;
    TELEMETRY_SetUpdated(offset);
}

static u8 checkCrossfireTelemetryFrameCRC() {
  u8 len = telemetryRxBuffer[1];
  u8 crc = crsf_crc8(&telemetryRxBuffer[2], len-1);
  return (crc == telemetryRxBuffer[len+1]);
}

static u8 getCrossfireTelemetryValue(u8 index, s32 *value, u8 len) {
  u8 result = 0;
  u8 *byte = &telemetryRxBuffer[index];
  *value = (*byte & 0x80) ? -1 : 0;
  for (u8 i=0; i < len; i++) {
    *value <<= 8;
    if (*byte != 0xff) result = 1;
    *value += *byte++;
  }
  return result;
}

static void processCrossfireTelemetryFrame()
{

  s32 value;
  u8 i;
  u8 id = telemetryRxBuffer[2];

  switch(id) {
    case TYPE_GPS:
      if (getCrossfireTelemetryValue(3, &value, 4)) {
        Telemetry.gps.latitude = value / 10;
        if (value & (1 << 30))
            Telemetry.gps.latitude = -Telemetry.gps.latitude;   // south negative
        TELEMETRY_SetUpdated(TELEM_GPS_LAT);
      }
      if (getCrossfireTelemetryValue(7, &value, 4)) {
        Telemetry.gps.longitude = value / 10;
        if (value & (1 << 30))
            Telemetry.gps.longitude = -Telemetry.gps.longitude;   // west negative
        TELEMETRY_SetUpdated(TELEM_GPS_LONG);
      }
      if (getCrossfireTelemetryValue(11, &value, 2)) {
        Telemetry.gps.velocity = value;
        TELEMETRY_SetUpdated(TELEM_GPS_SPEED);
      }
      if (getCrossfireTelemetryValue(13, &value, 2)) {
        Telemetry.gps.heading = value;
        TELEMETRY_SetUpdated(TELEM_GPS_HEADING);
      }
      if (getCrossfireTelemetryValue(15, &value, 2)) {
        Telemetry.gps.altitude = value - 1000;
        TELEMETRY_SetUpdated(TELEM_GPS_ALT);
      }
      if (getCrossfireTelemetryValue(17, &value, 1)) {
        Telemetry.gps.satcount = value;
        TELEMETRY_SetUpdated(TELEM_GPS_SATCOUNT);
      }
      break;

    case TYPE_LINK:
      for (i=1; i <= TELEM_CRSF_TX_SNR; i++) {
        if (getCrossfireTelemetryValue(2+i, &value, 1)) {   // payload starts at third byte of rx packet
          if (i == TELEM_CRSF_TX_POWER) {
            static const s32 power_values[] = { 0, 10, 25, 100, 500, 1000, 2000, 250 };
            if ((u8)value >= (sizeof power_values / sizeof (s32)))
              continue;
            value = power_values[value];
          }
          set_telemetry(i, value);
        }
      }
      break;

    case TYPE_BATTERY:
      if (getCrossfireTelemetryValue(3, &value, 2))
        set_telemetry(TELEM_CRSF_BATT_VOLTAGE, value);
      if (getCrossfireTelemetryValue(5, &value, 2))
        set_telemetry(TELEM_CRSF_BATT_CURRENT, value);
      if (getCrossfireTelemetryValue(7, &value, 3))
        set_telemetry(TELEM_CRSF_BATT_CAPACITY, value);
      break;

    case TYPE_ATTITUDE:
      if (getCrossfireTelemetryValue(3, &value, 2))
        set_telemetry(TELEM_CRSF_ATTITUDE_PITCH, value/10);
      if (getCrossfireTelemetryValue(5, &value, 2))
        set_telemetry(TELEM_CRSF_ATTITUDE_ROLL, value/10);
      if (getCrossfireTelemetryValue(7, &value, 2))
        set_telemetry(TELEM_CRSF_ATTITUDE_YAW, value/10);
      break;

    case TYPE_FLIGHT_MODE:  // string - save first four bytes for now
      memcpy(&value, &telemetryRxBuffer[3], 4);
      set_telemetry(TELEM_CRSF_FLIGHT_MODE, value);
      break;
  }
}

static void processElrsParamsData()
{
  if (telemetryRxBuffer[2] == TYPE_SETTINGS_WRITE && telemetryRxBuffer[3] == ADDR_RADIO && telemetryRxBuffer[4] == ADDR_MODULE && telemetryRxBuffer[5] == 0xFF) {
    if (telemetryRxBuffer[6] == 0x01) {
      currentBind = 1;
    } else {
      currentBind = 0;
    }

    if (telemetryRxBuffer[6] == 0x02) {
      currentWiFi = 1;
    } else {
      currentWiFi = 0;
    }

    currentPktRate = convertElrsToPktRate(telemetryRxBuffer[10], telemetryRxBuffer[7]);
    currentTlmRatio = telemetryRxBuffer[8];
    currentPower = telemetryRxBuffer[9];
    Model.proto_opts[PROTO_OPTS_BIND] = currentBind;
    Model.proto_opts[PROTO_OPTS_WIFI_UPDATE] = currentWiFi;
    Model.proto_opts[PROTO_OPTS_PKT_RATE] = currentPktRate;
    Model.proto_opts[PROTO_OPTS_TLM_RATIO] = currentTlmRatio;
    Model.proto_opts[PROTO_OPTS_POWER] = currentPower;
    Model.proto_opts[PROTO_OPTS_RF_FREQ] = telemetryRxBuffer[10];
    Model.proto_opts[PROTO_OPTS_BAD_PKTS] = telemetryRxBuffer[11];
    Model.proto_opts[PROTO_OPTS_GOOD_PKTS] = telemetryRxBuffer[12] * 256 + telemetryRxBuffer[13];
  }
}

// serial data receive ISR callback
static void processCrossfireTelemetryData(u8 data, u8 status) {
  (void)status;

  if (telemetryRxBufferCount == 0 && data != ADDR_RADIO) {
    return;
  }

  if (telemetryRxBufferCount == 1 && (data < 2 || data > TELEMETRY_RX_PACKET_SIZE-2)) {
    telemetryRxBufferCount = 0;
    return;
  }
  
  if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
    telemetryRxBuffer[telemetryRxBufferCount++] = data;
  } else {
    telemetryRxBufferCount = 0;
    return;
  }
  
  if ((telemetryRxBuffer[1] + 2) == telemetryRxBufferCount) {
    if (checkCrossfireTelemetryFrameCRC()) {
      if (telemetryRxBuffer[2] < TYPE_PING_DEVICES) {
        processCrossfireTelemetryFrame();     // Broadcast frame
#if SUPPORT_CRSF_CONFIG
      } else if (telemetryRxBuffer[2] == TYPE_SETTINGS_WRITE && getParamsCounter >= 100) {
        getParamsCounter = 0;
        processElrsParamsData();
      } else {
        CRSF_serial_rcv(telemetryRxBuffer+2, telemetryRxBuffer[1]-1);  // Extended frame
#endif
      }
    }
    telemetryRxBufferCount = 0;
  }
}
#endif  // HAS_EXTENDED_TELEMETRY

static u8 packet[CRSF_PACKET_SIZE];

#define ELRS_ADDRESS 0xEE
#define ELRS_BIND_COMMAND 0xFF
#define ELRS_WIFI_COMMAND 0xFE
#define ELRS_PKT_RATE_COMMAND 1
#define ELRS_TLM_RATIO_COMMAND 2
#define ELRS_POWER_COMMAND 3

static u8 buildElrsPacket(u8 command, u8 value)
{
  packet[0] = ADDR_MODULE;
  packet[1] = 6;
  packet[2] = TYPE_SETTINGS_WRITE;
  packet[3] = ELRS_ADDRESS;
  packet[4] = ADDR_RADIO;
  packet[5] = command;
  packet[6] = value;
  packet[7] = crsf_crc8(&packet[2], packet[1]-1);

  return 8;
}

static u8 setElrsOptions()
{
  if (Model.proto_opts[PROTO_OPTS_BIND] == 1 && currentBind == 0) {
    currentBind = Model.proto_opts[PROTO_OPTS_BIND];
    getParamsCounter = 0;
    return buildElrsPacket(ELRS_BIND_COMMAND, 0x01);
  }
  if (Model.proto_opts[PROTO_OPTS_WIFI_UPDATE] == 1 && currentWiFi == 0) {
    currentWiFi = Model.proto_opts[PROTO_OPTS_WIFI_UPDATE];
    getParamsCounter = 0;
    return buildElrsPacket(ELRS_WIFI_COMMAND, 0x01);
  }
  if (Model.proto_opts[PROTO_OPTS_PKT_RATE] != currentPktRate) {
    currentPktRate = Model.proto_opts[PROTO_OPTS_PKT_RATE];
    getParamsCounter = 0;
    return buildElrsPacket(ELRS_PKT_RATE_COMMAND, convertPktRateToElrs(Model.proto_opts[PROTO_OPTS_RF_FREQ], Model.proto_opts[PROTO_OPTS_PKT_RATE]));
  }
  if (Model.proto_opts[PROTO_OPTS_TLM_RATIO] != currentTlmRatio) {
    currentTlmRatio = Model.proto_opts[PROTO_OPTS_TLM_RATIO];
    getParamsCounter = 0;
    return buildElrsPacket(ELRS_TLM_RATIO_COMMAND, Model.proto_opts[PROTO_OPTS_TLM_RATIO]);
  }
  if (Model.proto_opts[PROTO_OPTS_POWER] != currentPower) {
    currentPower = Model.proto_opts[PROTO_OPTS_POWER];
    getParamsCounter = 0;
    return buildElrsPacket(ELRS_POWER_COMMAND, Model.proto_opts[PROTO_OPTS_POWER]);
  }
  return 0;
}

/* from CRSF document
Center (1500us) = 992
TICKS_TO_US(x) ((x - 992) * 5 / 8 + 1500)
US_TO_TICKS(x) ((x - 1500) * 8 / 5 + 992)
*/
//#define STICK_SCALE    869  // full scale at +-125
#define STICK_SCALE    800  // +/-100 gives 2000/1000 us
static u8 build_rcdata_pkt()
{
    int i;
	u16 channels[CRSF_CHANNELS];

    for (i=0; i < CRSF_CHANNELS; i++) {
        if (i < Model.num_channels)
            channels[i] = (u16)(Channels[i] * STICK_SCALE / CHAN_MAX_VALUE + 992);
        else
            channels[i] = 992;  // midpoint
    }

    packet[0] = ADDR_MODULE;
    packet[1] = 24;   // length of type + payload + crc
    packet[2] = TYPE_CHANNELS;

    packet[3]  = (u8) ((channels[0] & 0x07FF));
    packet[4]  = (u8) ((channels[0] & 0x07FF)>>8   | (channels[1] & 0x07FF)<<3);
    packet[5]  = (u8) ((channels[1] & 0x07FF)>>5   | (channels[2] & 0x07FF)<<6);
    packet[6]  = (u8) ((channels[2] & 0x07FF)>>2);
    packet[7]  = (u8) ((channels[2] & 0x07FF)>>10  | (channels[3] & 0x07FF)<<1);
    packet[8]  = (u8) ((channels[3] & 0x07FF)>>7   | (channels[4] & 0x07FF)<<4);
    packet[9]  = (u8) ((channels[4] & 0x07FF)>>4   | (channels[5] & 0x07FF)<<7);
    packet[10] = (u8) ((channels[5] & 0x07FF)>>1);
    packet[11] = (u8) ((channels[5] & 0x07FF)>>9   | (channels[6] & 0x07FF)<<2);
    packet[12] = (u8) ((channels[6] & 0x07FF)>>6   | (channels[7] & 0x07FF)<<5);
    packet[13] = (u8) ((channels[7] & 0x07FF)>>3);
    packet[14] = (u8) ((channels[8] & 0x07FF));
    packet[15] = (u8) ((channels[8] & 0x07FF)>>8   | (channels[9] & 0x07FF)<<3);
    packet[16] = (u8) ((channels[9] & 0x07FF)>>5   | (channels[10] & 0x07FF)<<6);  
    packet[17] = (u8) ((channels[10] & 0x07FF)>>2);
    packet[18] = (u8) ((channels[10] & 0x07FF)>>10 | (channels[11] & 0x07FF)<<1);
    packet[19] = (u8) ((channels[11] & 0x07FF)>>7  | (channels[12] & 0x07FF)<<4);
    packet[20] = (u8) ((channels[12] & 0x07FF)>>4  | (channels[13] & 0x07FF)<<7);
    packet[21] = (u8) ((channels[13] & 0x07FF)>>1);
    packet[22] = (u8) ((channels[13] & 0x07FF)>>9  | (channels[14] & 0x07FF)<<2);
    packet[23] = (u8) ((channels[14] & 0x07FF)>>6  | (channels[15] & 0x07FF)<<5);
    packet[24] = (u8) ((channels[15] & 0x07FF)>>3);

    packet[25] = crsf_crc8(&packet[2], CRSF_PACKET_SIZE-3);

    return CRSF_PACKET_SIZE;
}

#ifdef EMULATOR
static const u8 rxframes[][64];
#endif //EMULATOR

static enum {
    ST_DATA1,
    ST_DATA2,
} state;

static u16 mixer_runtime;
static u16 serial_cb()
{
    u8 length;

    getParamsCounter++;

    switch (state) {
    case ST_DATA1:
        CLOCK_RunMixer();    // clears mixer_sync, which is then set when mixer update complete
        state = ST_DATA2;
        return mixer_runtime;

    case ST_DATA2:
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
#if SUPPORT_CRSF_CONFIG
        if (Model.proto_opts[PROTO_OPTS_RF_FREQ] == 0) {
            length = CRSF_serial_txd(packet, sizeof packet);
        } else {
            length = setElrsOptions();
        }
        if (length == 0) {
            length = build_rcdata_pkt();
        }
#else
        length = build_rcdata_pkt();
#endif
        UART_Send(packet, length);
        state = ST_DATA1;

        return convertPktRateToPeriod(Model.proto_opts[PROTO_OPTS_RF_FREQ], currentPktRate) - mixer_runtime;
    }

    return CRSF_FRAME_PERIOD;   // avoid compiler warning
}

static void initialize()
{
    CLOCK_StopTimer();
    if (PPMin_Mode())
    {
        return;
    }
#if HAS_EXTENDED_AUDIO
#if HAS_AUDIO_UART
    if (!Transmitter.audio_uart)
#endif
    Transmitter.audio_player = AUDIO_DISABLED; // disable voice commands on serial port
#endif
    UART_Initialize();
    UART_SetDataRate(CRSF_DATARATE);
    UART_SetDuplex(UART_DUPLEX_HALF);
#if HAS_EXTENDED_TELEMETRY
    UART_StartReceive(processCrossfireTelemetryData);
#endif
    state = ST_DATA1;
    mixer_runtime = 50;

    CLOCK_StartTimer(1000, serial_cb);
}

uintptr_t CRSF_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: UART_Stop(); UART_Initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CHANNELMAP: return UNCHG;
#if SUPPORT_CRSF_CONFIG
        case PROTOCMD_OPTIONSPAGE:
            if (Model.proto_opts[PROTO_OPTS_RF_FREQ] == 0) return PAGEID_CRSFCFG;
            break;
        case PROTOCMD_GETOPTIONS:
            if (Model.proto_opts[PROTO_OPTS_RF_FREQ] == 6)
              return (uintptr_t)expresslrs_opts_24;
            else if (Model.proto_opts[PROTO_OPTS_RF_FREQ] > 0)
              return (uintptr_t)expresslrs_opts_900;
            break;
#endif  // SUPPORT_CRSF_CONFIG
#if HAS_EXTENDED_TELEMETRY
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_CRSF;
#endif
        default: break;
    }
    return 0;
}

