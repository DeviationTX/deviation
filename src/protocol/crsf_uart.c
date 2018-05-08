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
  #define CRSF_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#define CRSF_DATARATE             400000
#define CRSF_FRAME_PERIOD         4000   // 4ms
#define CRSF_CHANNELS             16
#define CRSF_PACKET_SIZE          26
#define CRSF_SYNC_BYTE            0xc8
#define CRSF_TYPE_RCDATA          0x16

// Device addresses
#define BROADCAST_ADDRESS              0x00
#define RADIO_ADDRESS                  0xEA
#define MODULE_ADDRESS                 0xEE

// Received Frame id
#define GPS_ID                         0x02
#define BATTERY_ID                     0x08
#define LINK_ID                        0x14
#define CHANNELS_ID                    0x16
#define ATTITUDE_ID                    0x1E
#define FLIGHT_MODE_ID                 0x21
#define PING_DEVICES_ID                0x28
#define DEVICE_INFO_ID                 0x29
#define REQUEST_SETTINGS_ID            0x2A

#define TELEMETRY_RX_PACKET_SIZE       128

static u8 telemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];
static u8 telemetryRxBufferCount;

static void processCrossfireTelemetryData(u8 data, u8 status);  // ISR callback
static void processCrossfireTelemetryFrame();


// crc implementation from CRSF protocol document rev7
static u8 crc8tab[256] = {
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

static u8 crc8(const u8 *ptr, u8 len)
{
    u8 crc = 0;
    for (u8 i=0; i < len; i++) {
        crc = crc8tab[crc ^ *ptr++];
    }
    return crc;
}

static u8 checkCrossfireTelemetryFrameCRC()
{
  u8 len = telemetryRxBuffer[1];
  u8 crc = crc8(&telemetryRxBuffer[2], len-1);
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

static void set_telemetry(crossfire_telem_t offset, s32 value) {
    Telemetry.value[offset] = value;
    TELEMETRY_SetUpdated(offset);
}

static void processCrossfireTelemetryFrame()
{
  if (!checkCrossfireTelemetryFrameCRC())
    return;

  s32 value;
  u8 i;
  u8 id = telemetryRxBuffer[2];

  switch(id) {
    case GPS_ID:
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

    case LINK_ID:
      for (i=0; i <= TELEM_CRSF_TX_SNR; i++) {
        if (getCrossfireTelemetryValue(3+i, &value, 1)) {
          if (i == TELEM_CRSF_TX_POWER) {
            static const s32 power_values[] = { 0, 10, 25, 100, 500, 1000, 2000 };
            value = power_values[value];
          }
          set_telemetry(i, value);
        }
      }
      break;

    case BATTERY_ID:
      if (getCrossfireTelemetryValue(3, &value, 2))
        set_telemetry(TELEM_CRSF_BATT_VOLTAGE, value);
      if (getCrossfireTelemetryValue(5, &value, 2))
        set_telemetry(TELEM_CRSF_BATT_CURRENT, value);
      if (getCrossfireTelemetryValue(7, &value, 3))
        set_telemetry(TELEM_CRSF_BATT_CAPACITY, value);
      break;

    case ATTITUDE_ID:
      if (getCrossfireTelemetryValue(3, &value, 2))
        set_telemetry(TELEM_CRSF_ATTITUDE_PITCH, value/10);
      if (getCrossfireTelemetryValue(5, &value, 2))
        set_telemetry(TELEM_CRSF_ATTITUDE_ROLL, value/10);
      if (getCrossfireTelemetryValue(7, &value, 2))
        set_telemetry(TELEM_CRSF_ATTITUDE_YAW, value/10);
      break;

    case FLIGHT_MODE_ID: // string - save first four bytes for now
      getCrossfireTelemetryValue(3, &value, 4);
      set_telemetry(TELEM_CRSF_FLIGHT_MODE, value);
      break;
  }
}

// serial data receive ISR callback
static void processCrossfireTelemetryData(u8 data, u8 status) {
  if (status != UART_RX_RXNE || (telemetryRxBufferCount == 0 && data != RADIO_ADDRESS)) {
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
  }
  
  if (telemetryRxBufferCount > 4) {
    u8 length = telemetryRxBuffer[1];
    if (length + 2 == telemetryRxBufferCount) {
      processCrossfireTelemetryFrame();
      telemetryRxBufferCount = 0;
    }
  }
}
//TODO #endif  HAS_EXTENDED_TELEMETRY 

static u8 packet[CRSF_PACKET_SIZE];



/* from CRSF document
Center (1500us) = 992
TICKS_TO_US(x) ((x - 992) * 5 / 8 + 1500)
US_TO_TICKS(x) ((x - 1500) * 8 / 5 + 992)
*/
//#define STICK_SCALE    869  // full scale at +-125
#define STICK_SCALE    800  // +/-100 gives 2000/1000 us
static void build_rcdata_pkt()
{
    int i;
	u16 channels[CRSF_CHANNELS];

    for (i=0; i < CRSF_CHANNELS; i++) {
        if (i < Model.num_channels)
            channels[i] = (u16)(Channels[i] * STICK_SCALE / CHAN_MAX_VALUE + 992);
        else
            channels[i] = 992;  // midpoint
    }

    packet[0] = CRSF_SYNC_BYTE; 
    packet[1] = 24;   // length of type + payload + crc
    packet[2] = CRSF_TYPE_RCDATA; 

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

    packet[25] = crc8(&packet[2], CRSF_PACKET_SIZE-3);
}

// static u8 testrxframe[] = { 0x00, 0x0C, 0x14, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x03, 0x00, 0x00, 0x00, 0xF4 };

static u16 serial_cb()
{
    build_rcdata_pkt();
    UART_Send(packet, sizeof packet);

    return CRSF_FRAME_PERIOD;
}

static void initialize()
{
    CLOCK_StopTimer();
    if (PPMin_Mode())
    {
        return;
    }
#if HAS_EXTENDED_AUDIO
#if HAS_AUDIO_UART5
    if (!Transmitter.audio_uart5)
#endif
    Transmitter.audio_player = AUDIO_DISABLED; // disable voice commands on serial port
#endif
    UART_Initialize();
    UART_SetDataRate(CRSF_DATARATE);
    UART_SetDuplex(UART_DUPLEX_HALF);
#if HAS_EXTENDED_TELEMETRY
    UART_StartReceive(processCrossfireTelemetryData);
#endif

    CLOCK_StartTimer(1000, serial_cb);
}

const void * CRSF_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: UART_Initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)16L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return (void *)(long) TELEM_CRSF;
        default: break;
    }
    return 0;
}
