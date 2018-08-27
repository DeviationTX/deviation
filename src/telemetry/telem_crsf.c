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

#if HAS_EXTENDED_TELEMETRY
s32 _crsf_value(struct Telemetry *t, int idx)
{
    return t->value[idx];
}

#if 0
const CrossfireSensor crossfireSensors[] = {
  {LINK_ID,        0, ZSTR_RX_RSSI1,    UNIT_DB,            0},
  {LINK_ID,        1, ZSTR_RX_RSSI2,    UNIT_DB,            0},
  {LINK_ID,        2, ZSTR_RX_QUALITY,  UNIT_PERCENT,       0},
  {LINK_ID,        3, ZSTR_RX_SNR,      UNIT_DB,            0},
  {LINK_ID,        4, ZSTR_ANTENNA,     UNIT_RAW,           0},
  {LINK_ID,        5, ZSTR_RF_MODE,     UNIT_RAW,           0},
  {LINK_ID,        6, ZSTR_TX_POWER,    UNIT_MILLIWATTS,    0},
  {LINK_ID,        7, ZSTR_TX_RSSI,     UNIT_DB,            0},
  {LINK_ID,        8, ZSTR_TX_QUALITY,  UNIT_PERCENT,       0},
  {LINK_ID,        9, ZSTR_TX_SNR,      UNIT_DB,            0},
  {BATTERY_ID,     0, ZSTR_BATT,        UNIT_VOLTS,         1},
  {BATTERY_ID,     1, ZSTR_CURR,        UNIT_AMPS,          1},
  {BATTERY_ID,     2, ZSTR_CAPACITY,    UNIT_MAH,           0},
  {GPS_ID,         0, ZSTR_GPS,         UNIT_GPS_LATITUDE,  0},
  {GPS_ID,         0, ZSTR_GPS,         UNIT_GPS_LONGITUDE, 0},
  {GPS_ID,         2, ZSTR_GSPD,        UNIT_KMH,           1},
  {GPS_ID,         3, ZSTR_HDG,         UNIT_DEGREE,        3},
  {GPS_ID,         4, ZSTR_ALT,         UNIT_METERS,        0},
  {GPS_ID,         5, ZSTR_SATELLITES,  UNIT_RAW,           0},
  {ATTITUDE_ID,    0, ZSTR_PITCH,       UNIT_RADIANS,       3},
  {ATTITUDE_ID,    1, ZSTR_ROLL,        UNIT_RADIANS,       3},
  {ATTITUDE_ID,    2, ZSTR_YAW,         UNIT_RADIANS,       3},
  {FLIGHT_MODE_ID, 0, ZSTR_FLIGHT_MODE, UNIT_TEXT,          0},
  {0,              0, "UNKNOWN",        UNIT_RAW,           0},
};
#endif

const char * _crsf_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
    case TELEM_CRSF_TX_SNR:
    case TELEM_CRSF_TX_RSSI:
    case TELEM_CRSF_RX_SNR:
    case TELEM_CRSF_RX_RSSI1:
    case TELEM_CRSF_RX_RSSI2: _get_value_str(str, value, 0, 'd'); break;
    case TELEM_CRSF_FLIGHT_MODE: memcpy(str, &value, 4); str[4]=0; break;
    case TELEM_CRSF_RF_MODE:
    case TELEM_CRSF_RX_ANTENNA: _get_value_str(str, value, 0, '\0'); break;
    case TELEM_CRSF_TX_QUALITY:
    case TELEM_CRSF_RX_QUALITY: _get_value_str(str, value, 0, '%'); break;
    case TELEM_CRSF_TX_POWER: _get_value_str(str, value, 0, 'w'); break;
    case TELEM_CRSF_BATT_VOLTAGE: _get_value_str(str, value, 1, 'V'); break;
    case TELEM_CRSF_BATT_CURRENT: _get_value_str(str, value, 1, 'A'); break;
    case TELEM_CRSF_BATT_CAPACITY: _get_value_str(str, value, 1, 'a'); break;
    case TELEM_CRSF_ATTITUDE_PITCH:
    case TELEM_CRSF_ATTITUDE_ROLL:
    case TELEM_CRSF_ATTITUDE_YAW: _get_value_str(str, value, 3, 'R'); break;
    default:
        return "";
    }
    return str;
}

const char * _crsf_short_name(char *str, u8 telem)
{
    switch(telem) {
    case 0: strcpy(str, _tr("None")); break;
    case TELEM_CRSF_TX_SNR: strcpy(str, _tr("tx SNR")); break;
    case TELEM_CRSF_TX_RSSI: strcpy(str, _tr("txRSSI")); break;
    case TELEM_CRSF_RX_SNR: strcpy(str, _tr("rx SNR")); break;
    case TELEM_CRSF_RX_RSSI1:
    case TELEM_CRSF_RX_RSSI2: sprintf(str, "rxRSSI%d", telem-TELEM_CRSF_RX_RSSI1+1); break;
    case TELEM_CRSF_FLIGHT_MODE: strcpy(str, _tr("FMode")); break;
    case TELEM_CRSF_RF_MODE: strcpy(str, _tr("RFMode")); break;
    case TELEM_CRSF_RX_ANTENNA: strcpy(str, _tr("ANT")); break;
    case TELEM_CRSF_TX_POWER: strcpy(str, _tr("Power")); break;
    case TELEM_CRSF_RX_QUALITY: strcpy(str, _tr("rxQual")); break;
    case TELEM_CRSF_TX_QUALITY: strcpy(str, _tr("txQual")); break;
    case TELEM_CRSF_BATT_VOLTAGE: sprintf(str, _tr("Volt")); break;
    case TELEM_CRSF_BATT_CURRENT: strcpy(str, _tr("Curr")); break;
    case TELEM_CRSF_BATT_CAPACITY: strcpy(str, _tr("Cap")); break;
    case TELEM_CRSF_ATTITUDE_PITCH: strcpy(str, _tr("Pitch")); break;
    case TELEM_CRSF_ATTITUDE_ROLL: strcpy(str, _tr("Roll")); break;
    case TELEM_CRSF_ATTITUDE_YAW: strcpy(str, _tr("Yaw")); break;
    default: sprintf(str, "CRSF%d", telem); break;
    }
    return str;
}

const char * _crsf_name(char *str, u8 telem)
{
    switch (telem) {
    case TELEM_CRSF_FLIGHT_MODE: strcpy(str, _tr("Flight Mode")); break;
    case TELEM_CRSF_RF_MODE: strcpy(str, _tr("RF Mode")); break;
    case TELEM_CRSF_RX_ANTENNA: strcpy(str, _tr("Antenna")); break;
    case TELEM_CRSF_RX_QUALITY: strcpy(str, _tr("rx Quality")); break;
    case TELEM_CRSF_TX_QUALITY: strcpy(str, _tr("tx Quality")); break;
    case TELEM_CRSF_BATT_CAPACITY: strcpy(str, _tr("Capacity")); break;
    default: _crsf_short_name(str, telem); break;
    }
    return str;
}

s32 _crsf_get_max_value(u8 telem)
{
    switch(telem) {
    case TELEM_CRSF_TX_SNR: return 1000; break;
    case TELEM_CRSF_TX_RSSI: return 100; break;
    case TELEM_CRSF_RX_SNR: return 100; break;
    case TELEM_CRSF_RX_RSSI1:
    case TELEM_CRSF_RX_RSSI2: return 100; break;
    case TELEM_CRSF_RF_MODE: return 10; break;
    case TELEM_CRSF_RX_ANTENNA: return 10; break;
    case TELEM_CRSF_TX_POWER: return 1000; break;
    case TELEM_CRSF_RX_QUALITY: return 150; break;
    case TELEM_CRSF_TX_QUALITY: return 150; break;
    case TELEM_CRSF_BATT_VOLTAGE: return 100; break;
    case TELEM_CRSF_BATT_CURRENT: return 100; break;
    case TELEM_CRSF_BATT_CAPACITY: return 500000; break;
    case TELEM_CRSF_ATTITUDE_PITCH:
    case TELEM_CRSF_ATTITUDE_ROLL:
    case TELEM_CRSF_ATTITUDE_YAW: return 7; break;
    default: return 0;
    }
}

s32 _crsf_get_min_value(u8 telem)
{
    switch(telem) {
    case TELEM_CRSF_TX_SNR: return -1000; break;
    case TELEM_CRSF_TX_RSSI: return -100; break;
    case TELEM_CRSF_RX_SNR: return -100; break;
    case TELEM_CRSF_RX_RSSI1:
    case TELEM_CRSF_RX_RSSI2: return -100; break;
    case TELEM_CRSF_ATTITUDE_PITCH:
    case TELEM_CRSF_ATTITUDE_ROLL:
    case TELEM_CRSF_ATTITUDE_YAW: return -7; break;
    default: return 0;
    }
}
#else
    s32 _crsf_value(struct Telemetry *t, int idx) {(void)t, (void)idx; return 0;}
    const char * _crsf_str_by_value(char *str, u8 telem, s32 value) {(void)str; (void)telem; (void)value; return NULL;}
    const char * _crsf_short_name(char *str, u8 telem) {(void)str; (void) telem; return NULL;}
    const char * _crsf_name(char *str, u8 telem) {(void)str; (void) telem; return NULL;}
    s32 _crsf_get_max_value(u8 telem) {(void) telem; return 0;}
    s32 _crsf_get_min_value(u8 telem) {(void) telem; return 0;}
#endif //HAS_EXTENDED_TELEMETRY
