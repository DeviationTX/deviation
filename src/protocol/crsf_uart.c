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
#include "target/drivers/serial/usb_cdc/CBUF.h"
#endif

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#define CRSF_FRAME_PERIOD         4000   // 250Hz 4ms
#define CRSF_FRAME_PERIOD_MIN     850    // 1000Hz 1ms, but allow shorter for offset cancellation
#define CRSF_FRAME_PERIOD_MAX     50000  // 25Hz  40ms, but allow longer for offset cancellation
#define CRSF_CHANNELS             16
#define CRSF_PACKET_SIZE          26

volatile uint16_t crcErrorCount;
static const u32 bitrates[] = { 400000, 1870000, 2000000 };
static s8 new_bitrate_index = -1;
static const char * const crsf_opts[] = {
  _tr_noop("Bit Rate"), "400K", "1.87M", "2.00M", NULL,
  _tr_noop("Show Hidden"), "No", "Yes", NULL,
  _tr_noop("ELRS Arm"), "CH5", "Virt1", "Virt2", "Virt3", "Virt4", "Virt5", "Virt6", "Virt7", "Virt8", "Virt9", "Virt10", NULL,
  NULL
};

// this function called from UART TX DMA send complete ISR
void set_bitrate(u8 data, u8 status) {
    (void)data;
    (void)status;
    if (new_bitrate_index >= 0) {
        UART_SetDataRate(bitrates[new_bitrate_index]);
        Model.proto_opts[PROTO_OPTS_BITRATE] = new_bitrate_index;
    }
    new_bitrate_index = -1;
    UART_TxCallback(NULL);
}

s8 check_bitrate(u32 rate) {
    if (rate == bitrates[0]) return 0;
    if (rate == bitrates[1]) return 1;
    if (rate == bitrates[2]) return 2;
    return -1;
}


// crc implementation from CRSF protocol document rev7
static const u8 crc8tab[256] = {
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

// CRC8 implementation with polynom = 0xBA
static const u8 crc8tab_BA[256] = {
    0x00, 0xBA, 0xCE, 0x74, 0x26, 0x9C, 0xE8, 0x52, 0x4C, 0xF6, 0x82, 0x38, 0x6A, 0xD0, 0xA4, 0x1E,
    0x98, 0x22, 0x56, 0xEC, 0xBE, 0x04, 0x70, 0xCA, 0xD4, 0x6E, 0x1A, 0xA0, 0xF2, 0x48, 0x3C, 0x86,
    0x8A, 0x30, 0x44, 0xFE, 0xAC, 0x16, 0x62, 0xD8, 0xC6, 0x7C, 0x08, 0xB2, 0xE0, 0x5A, 0x2E, 0x94,
    0x12, 0xA8, 0xDC, 0x66, 0x34, 0x8E, 0xFA, 0x40, 0x5E, 0xE4, 0x90, 0x2A, 0x78, 0xC2, 0xB6, 0x0C,
    0xAE, 0x14, 0x60, 0xDA, 0x88, 0x32, 0x46, 0xFC, 0xE2, 0x58, 0x2C, 0x96, 0xC4, 0x7E, 0x0A, 0xB0,
    0x36, 0x8C, 0xF8, 0x42, 0x10, 0xAA, 0xDE, 0x64, 0x7A, 0xC0, 0xB4, 0x0E, 0x5C, 0xE6, 0x92, 0x28,
    0x24, 0x9E, 0xEA, 0x50, 0x02, 0xB8, 0xCC, 0x76, 0x68, 0xD2, 0xA6, 0x1C, 0x4E, 0xF4, 0x80, 0x3A,
    0xBC, 0x06, 0x72, 0xC8, 0x9A, 0x20, 0x54, 0xEE, 0xF0, 0x4A, 0x3E, 0x84, 0xD6, 0x6C, 0x18, 0xA2,
    0xE6, 0x5C, 0x28, 0x92, 0xC0, 0x7A, 0x0E, 0xB4, 0xAA, 0x10, 0x64, 0xDE, 0x8C, 0x36, 0x42, 0xF8,
    0x7E, 0xC4, 0xB0, 0x0A, 0x58, 0xE2, 0x96, 0x2C, 0x32, 0x88, 0xFC, 0x46, 0x14, 0xAE, 0xDA, 0x60,
    0x6C, 0xD6, 0xA2, 0x18, 0x4A, 0xF0, 0x84, 0x3E, 0x20, 0x9A, 0xEE, 0x54, 0x06, 0xBC, 0xC8, 0x72,
    0xF4, 0x4E, 0x3A, 0x80, 0xD2, 0x68, 0x1C, 0xA6, 0xB8, 0x02, 0x76, 0xCC, 0x9E, 0x24, 0x50, 0xEA,
    0x48, 0xF2, 0x86, 0x3C, 0x6E, 0xD4, 0xA0, 0x1A, 0x04, 0xBE, 0xCA, 0x70, 0x22, 0x98, 0xEC, 0x56,
    0xD0, 0x6A, 0x1E, 0xA4, 0xF6, 0x4C, 0x38, 0x82, 0x9C, 0x26, 0x52, 0xE8, 0xBA, 0x00, 0x74, 0xCE,
    0xC2, 0x78, 0x0C, 0xB6, 0xE4, 0x5E, 0x2A, 0x90, 0x8E, 0x34, 0x40, 0xFA, 0xA8, 0x12, 0x66, 0xDC,
    0x5A, 0xE0, 0x94, 0x2E, 0x7C, 0xC6, 0xB2, 0x08, 0x16, 0xAC, 0xD8, 0x62, 0x30, 0x8A, 0xFE, 0x44};

static u8 crsf_crc(const u8 crctab[], const u8 *ptr, u8 len) {
    u8 crc = 0;
    for (u8 i=0; i < len; i++) {
        crc = crctab[crc ^ *ptr++];
    }
    return crc;
}
u8 crsf_crc8(const u8 *ptr, u8 len) {
    return crsf_crc(crc8tab, ptr, len);
}
u8 crsf_crc8_BA(const u8 *ptr, u8 len) {
    return crsf_crc(crc8tab_BA, ptr, len);
}
// crc accumulator format - start with crc=0
void crsf_crc8_acc(u8 *crc, const u8 val) {
    *crc = crc8tab[*crc ^ val];
}
void crsf_crc8_BA_acc(u8 *crc, const u8 val) {
    *crc = crc8tab_BA[*crc ^ val];
}

#if SUPPORT_CRSF_CONFIG
static u8 model_id_send;
static u32 elrs_info_time;
static module_type_t module_type;

#define MODULE_IS_ELRS     (module_type == MODULE_ELRS)
#define MODULE_IS_UNKNOWN  (module_type == MODULE_UNKNOWN)
void protocol_module_type(module_type_t type) {
    module_type = type;
};
u8 protocol_module_is_elrs() { return MODULE_IS_ELRS; }
inline u8 protocol_elrs_is_armed() { return elrs_info.flags & FLAG_ARMD; }
void protocol_read_params(u8 device_idx, crsf_param_t params[]) {
    // protocol parameters are bitrate and show hidden UI options
    params[0].device = device_idx;            // device index of device parameter belongs to
    params[0].id = 1;                // Parameter number (starting from 1)
    params[0].parent = 0;            // Parent folder parameter number of the parent folder, 0 means root
    params[0].type = TEXT_SELECTION;  // (Parameter type definitions and hidden bit)
    params[0].hidden = 0;            // set if hidden
    params[0].loaded = 1;
    params[0].name = (char*)crsf_opts[0];           // Null-terminated string
    params[0].value = "400K\0001.87M\0002.00M";    // must match crsf_opts
    params[0].default_value = 0;  // size depending on data type. Not present for COMMAND.
    params[0].min_value = 0;        // not sent for string type
    params[0].max_value = 2;        // not sent for string type
    params[0].changed = 0;           // flag if set needed when edit element is de-selected
    params[0].max_str = &((char*)params[0].value)[11];        // Longest choice length for text select
    params[0].lines_per_row = 1;
    params[0].u.text_sel = Model.proto_opts[PROTO_OPTS_BITRATE];

    params[1].device = device_idx;            // device index of device parameter belongs to
    params[1].id = 2;                // Parameter number (starting from 1)
    params[1].parent = 0;            // Parent folder parameter number of the parent folder, 0 means root
    params[1].type = TEXT_SELECTION;  // (Parameter type definitions and hidden bit)
    params[1].hidden = 0;            // set if hidden
    params[1].loaded = 1;
    params[1].name = (char*)crsf_opts[5];           // Null-terminated string
    params[1].value = "No\000Yes";    // must match crsf_opts
    params[1].default_value = 0;  // size depending on data type. Not present for COMMAND.
    params[1].min_value = 0;        // not sent for string type
    params[1].max_value = 1;        // not sent for string type
    params[1].changed = 0;           // flag if set needed when edit element is de-selected
    params[1].max_str = &((char*)params[1].value)[3];        // Longest choice length for text select
    params[1].lines_per_row = 1;
    params[1].u.text_sel = Model.proto_opts[PROTO_OPTS_HIDDEN];

    params[2].device = device_idx;            // device index of device parameter belongs to
    params[2].id = 3;                // Parameter number (starting from 1)
    params[2].parent = 0;            // Parent folder parameter number of the parent folder, 0 means root
    params[2].type = TEXT_SELECTION;  // (Parameter type definitions and hidden bit)
    params[2].hidden = 0;            // set if hidden
    params[2].loaded = 1;
    params[2].name = (char*)crsf_opts[9];           // Null-terminated string
    params[2].value = "CH5\000Virt1\000Virt2\000Virt3\000Virt4\000Virt5\000Virt6\000Virt7\000Virt8\000Virt9\000Virt10";
    params[2].default_value = 0;  // size depending on data type. Not present for COMMAND.
    params[2].min_value = 0;        // not sent for string type
    params[2].max_value = 10;       // not sent for string type
    params[2].changed = 0;           // flag if set needed when edit element is de-selected
    params[2].max_str = &((char*)params[2].value)[58];        // Longest choice length for text select
    params[2].lines_per_row = 1;
    params[2].u.text_sel = Model.proto_opts[PROTO_OPTS_ELRSARM];
}

void protocol_set_param(crsf_param_t *param) {
    u8 value = param->u.text_sel;
    switch(param->id) {
    case 1:
        Model.proto_opts[PROTO_OPTS_BITRATE] = value;
        UART_SetDataRate(bitrates[value]);
        break;
    case 2:
        Model.proto_opts[PROTO_OPTS_HIDDEN] = value;
        break;
    case 3:
        Model.proto_opts[PROTO_OPTS_ELRSARM] = value;
        break;
    }
}
#endif


#if HAS_EXTENDED_TELEMETRY

static struct {
    volatile uint8_t    m_get_idx;
    volatile uint8_t    m_put_idx;
    uint8_t             m_entry[1024];  // must be power of 2
} receive_buf;

static u8 telemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];
static u8 telemetryRxBufferCount;
static u32 updateInterval = CRSF_FRAME_PERIOD;
static s32 correction;

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
        // convert degrees * 1M to seconds * 1000
        // (degree * 10,000,000) / 100 * 36
        Telemetry.gps.latitude = value / 25 * 9;
        TELEMETRY_SetUpdated(TELEM_GPS_LAT);
      }
      if (getCrossfireTelemetryValue(7, &value, 4)) {
        Telemetry.gps.longitude = value / 25 * 9;
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
            static const s32 power_values[] = { 0, 10, 25, 100, 500, 1000, 2000, 250, 50 };
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
      if (getCrossfireTelemetryValue(10, &value, 1))
        set_telemetry(TELEM_CRSF_BATT_REMAINING, value);
      break;

#if SUPPORT_CRSF_CONFIG
    case TYPE_VARIO:
      if (getCrossfireTelemetryValue(3, &value, 2))
        set_telemetry(TELEM_CRSF_VERTSPD, value);
      break;

    case TYPE_RX_ID:
      if (getCrossfireTelemetryValue(4, &value, 1))
        set_telemetry(TELEM_CRSF_RX_RSSI_PERC, value);
      if (getCrossfireTelemetryValue(7, &value, 1))
        set_telemetry(TELEM_CRSF_TX_RF_POWER, value);
      break;

    case TYPE_TX_ID:
      if (getCrossfireTelemetryValue(4, &value, 1))
        set_telemetry(TELEM_CRSF_TX_RSSI_PERC, value);
      if (getCrossfireTelemetryValue(7, &value, 1))
        set_telemetry(TELEM_CRSF_RX_RF_POWER, value);
      if (getCrossfireTelemetryValue(8, &value, 1))
        set_telemetry(TELEM_CRSF_TX_FPS, value * 10);
      break;

    case TYPE_BARO_ALT:
      if (getCrossfireTelemetryValue(3, &value, 2)) {
        if (value & 0x8000) {
          // Altitude in meters
          value &= ~(0x8000);
          value *= 100;        // cm
        } else {
          // Altitude in decimeters + 10000dm
          value -= 10000;
          value *= 10;
        }
        set_telemetry(TELEM_CRSF_ALTITUDE, value);
      }
      // if length greater than 4 then vario info is included
      if (telemetryRxBuffer[1] > 5 && getCrossfireTelemetryValue(5, &value, 2))
        set_telemetry(TELEM_CRSF_VERTSPD, value);
      break;

// Leave this mostly disabled for backwards compatibility with TBS XF Transmitter firmware
// version 6.19.  This 2022 version is the last Public release.  The full code below works
// with latest Beta release 6.42.  The 6.19 firmware switches to inverted serial when
// changing bitrates by this command, so will stay disabled until compatible Public release.
// The code is left in to respond positively if the requested speed is the same as the current
// speed.  This keeps the XF from repeating the request forever.
    case TYPE_COMMAND_ID:
      if (telemetryRxBuffer[3] == ADDR_RADIO && telemetryRxBuffer[5] == GENERAL_SUBCMD) {
          if (telemetryRxBuffer[6] == SUBCMD_SPD_PROPOSAL) {
              if (telemetryRxBuffer[7] != 0) break;
              getCrossfireTelemetryValue(8, &value, 4);
              new_bitrate_index = check_bitrate((u32)value);
              if (new_bitrate_index >= 0)
                  if (new_bitrate_index == Model.proto_opts[PROTO_OPTS_BITRATE])
                      CRSF_speed_response(1, NULL);
                  else
                      CRSF_speed_response(0, NULL); // enable speed change if necessary   CRSF_speed_response(1, set_bitrate);
              else
                  CRSF_speed_response(0, NULL);

          }
      }
      break;
#endif

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

    case TYPE_RADIO_ID:
      if (telemetryRxBuffer[3] == ADDR_RADIO && telemetryRxBuffer[5] == CRSF_SUBCOMMAND) {
        if (getCrossfireTelemetryValue(6, (s32 *)&updateInterval, 4))
          updateInterval /= 10;  // values are in 10th of micro-seconds
        if (getCrossfireTelemetryValue(10, (s32 *)&correction, 4)) {
          correction /= 10;  // values are in 10th of micro-seconds
          // truncate - can be greater than interval
          if (correction >= 0)
              correction %= updateInterval;
          else
              correction = -((-correction) % updateInterval);
        }
      }
#if SUPPORT_CRSF_CONFIG
      if (MODULE_IS_UNKNOWN) CRSF_ping_devices(ADDR_MODULE);
#endif
      break;

    case TYPE_VTX_TELEM:
      if (getCrossfireTelemetryValue(3, &value, 2))
        set_telemetry(TELEM_CRSF_VTX_FREQ, value);
      if (getCrossfireTelemetryValue(5, &value, 1))
        set_telemetry(TELEM_CRSF_VTX_PITMODE, value);
      if (getCrossfireTelemetryValue(6, &value, 1))
        set_telemetry(TELEM_CRSF_VTX_POWER, value);
      break;
  }
}


static void processCrossfireTelemetryData() {
    static u8 length;
    u8 data;

    while (CBUF_Len(receive_buf)) {
        data = CBUF_Pop(receive_buf);

        if (telemetryRxBufferCount == 0) {
            if (data == ADDR_RADIO || data == ADDR_BROADCAST || data == UART_SYNC)
                telemetryRxBuffer[telemetryRxBufferCount++] = data;
            continue;
        }

        if (telemetryRxBufferCount == 1) {
            if (data < 2 || data > TELEMETRY_RX_PACKET_SIZE-2) {
                telemetryRxBufferCount = 0;
                if (data == ADDR_RADIO)
                    telemetryRxBuffer[telemetryRxBufferCount++] = data;
            } else {
                length = data;
                telemetryRxBuffer[telemetryRxBufferCount++] = data;
            }
            continue;
        }
        
        if (telemetryRxBufferCount <= length+1) {
            telemetryRxBuffer[telemetryRxBufferCount++] = data;
        }
        
        if (telemetryRxBufferCount >= length+2) {
            if (checkCrossfireTelemetryFrameCRC()) {
                if (telemetryRxBuffer[2] < TYPE_PING_DEVICES
                 || telemetryRxBuffer[2] == TYPE_RADIO_ID
                 || telemetryRxBuffer[2] == TYPE_COMMAND_ID) {
                    processCrossfireTelemetryFrame();
#if SUPPORT_CRSF_CONFIG
                    // wait for telemetry running before sending model id
                    if (model_id_send) {
                        CRSF_send_model_id(Model.fixed_id);
                        model_id_send = 0;
                        return;
                    }
                } else {
                    CRSF_serial_rcv(telemetryRxBuffer+2, telemetryRxBuffer[1]-1);  // Extended frame
                }
                if (MODULE_IS_ELRS
                    && !protocol_elrs_is_armed()       // disarmed
                    && (CLOCK_getms() - elrs_info_time) > 200)
                {
                    CRSF_get_elrs();
                    elrs_info_time = CLOCK_getms();
#endif
                }
            }
            telemetryRxBufferCount = 0;
        }
    }
}

// serial data receive ISR callback
static void serial_rcv(u8 data, u8 status) {
    (void)status;

    CBUF_Push(receive_buf, data);
    CLOCK_RunOnce(processCrossfireTelemetryData);
}

static u32 get_update_interval() {
    if (correction == 0) return updateInterval;

    u32 update = updateInterval + correction;
    update = constrain(update, CRSF_FRAME_PERIOD_MIN, CRSF_FRAME_PERIOD_MAX);
    correction -= update - updateInterval;
    return update;
}
#else
static u32 get_update_interval() {
    return CRSF_FRAME_PERIOD;
}
#endif  // HAS_EXTENDED_TELEMETRY


static u8 packet[CRSF_PACKET_SIZE+1];   // plus 1 for ELRS Arming extension



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

    u8 *p = &packet[25];
#if SUPPORT_CRSF_CONFIG
    if (MODULE_IS_ELRS && Model.proto_opts[PROTO_OPTS_ELRSARM] > 0) {
        packet[1] = 25;
        *p++ = MIXER_GetChannel(NUM_OUT_CHANNELS + Model.proto_opts[PROTO_OPTS_ELRSARM] - 1, 0) > 0 ? 1 : 0;
    }
#endif
    *p = crsf_crc8(&packet[2], packet[1]-1);

    return packet[1] + 2;
}

static enum {
    ST_DATA0,
    ST_DATA1,
} state;

static u16 mixer_runtime;
static u16 serial_cb()
{
    u8 length;

    switch (state) {
    case ST_DATA0:
        CLOCK_RunMixer();    // clears mixer_sync, which is then set when mixer update complete
        state = ST_DATA1;
        return mixer_runtime;

    case ST_DATA1:
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
#if SUPPORT_CRSF_CONFIG
        length = CRSF_serial_txd(packet);
        if (length == 0) {
            length = build_rcdata_pkt();
        }
#else
        length = build_rcdata_pkt();
#endif
        UART_Send(packet, length);

#if SUPPORT_CRSF_CONFIG
        if (CBUF_Len(receive_buf)) CLOCK_RunOnce(processCrossfireTelemetryData);
#endif

        state = ST_DATA0;
        return get_update_interval() - mixer_runtime;
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
    UART_SetDataRate(bitrates[Model.proto_opts[PROTO_OPTS_BITRATE]]);
    UART_SetDuplex(UART_DUPLEX_HALF);
#if HAS_EXTENDED_TELEMETRY
    CBUF_Init(receive_buf);
    UART_StartReceive(serial_rcv);
#endif
    state = ST_DATA0;
    mixer_runtime = 50;
#if SUPPORT_CRSF_CONFIG
    model_id_send = 1;
#endif
    if (Model.fixed_id > CRSF_MAX_FIXEDID)
        Model.fixed_id = 0;

    CLOCK_StartTimer(1000, serial_cb);
}

uintptr_t CRSF_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: UART_Stop(); UART_Initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_CHANGED_ID:
#if SUPPORT_CRSF_CONFIG
            model_id_send = 1;
#endif
            return 0;
        case PROTOCMD_MAX_ID:
#if SUPPORT_CRSF_CONFIG
            return CRSF_MAX_FIXEDID;
#else
            return 0;
#endif
        case PROTOCMD_BIND: return 0;
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CHANNELMAP: return UNCHG;
#if SUPPORT_CRSF_CONFIG
        case PROTOCMD_OPTIONSPAGE: return PAGEID_CRSFCFG;
#endif
        // this case is only used to load/save options when PROTOCMD_OPTIONSPAGE is defined
        case PROTOCMD_GETOPTIONS:
            return (uintptr_t)crsf_opts;
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

