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
#include "config/model.h"
#include "telemetry.h"

struct Telemetry Telemetry;
s32 TELEMETRY_GetValue(int idx)
{
    switch (idx) {
    case TELEM_VOLT1:
    case TELEM_VOLT2:
    case TELEM_VOLT3:
        return Telemetry.volt[idx - TELEM_VOLT1];
    case TELEM_TEMP1:
    case TELEM_TEMP2:
    case TELEM_TEMP3:
    case TELEM_TEMP4:
        return Telemetry.temp[idx - TELEM_TEMP1];
    case TELEM_RPM1:
    case TELEM_RPM2:
        return Telemetry.rpm[idx - TELEM_RPM1];
    case TELEM_GPS_LONG:
        return Telemetry.gps.longitude;
    case TELEM_GPS_LAT:
        return Telemetry.gps.latitude;
    case TELEM_GPS_ALT:
        return Telemetry.gps.altitude;
    case TELEM_GPS_SPEED:
        return Telemetry.gps.velocity;
    case TELEM_GPS_TIME:
        return Telemetry.gps.time;
    }
    return 0;
}

const char * TELEMETRY_GetValueStrByValue(char *str, u8 telem, s32 value)
{
    int h, m, s, ss;
    switch(telem) {
        case TELEM_VOLT1:
        case TELEM_VOLT2:
        case TELEM_VOLT3:
            sprintf(str, "%d.%dV", (int)value /10, (int)value % 10);
            break;
        case TELEM_TEMP1:
        case TELEM_TEMP2:
        case TELEM_TEMP3:
        case TELEM_TEMP4:
            if (value == 0) {
                strcpy(str, "----");
            } else {
                if (Model.telem_flags & TELEMFLAG_FAREN) {
                    sprintf(str, "%dF", ((int)value * 9 + 288)/ 5);
                } else {
                    sprintf(str, "%dC", (int)value);
                }
            }
            break;
        case TELEM_RPM1:
        case TELEM_RPM2:
            sprintf(str, "%d", (int)value);
            break;
        case TELEM_GPS_LONG:
            h = value / 1000 / 60 / 60;
            m = (value - h * 1000 * 60 * 60) / 1000 / 60;
            s = (value - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = value % 1000;
            sprintf(str, "%03d %02d %02d.%03d", h, m, s, ss);
            break;
        case TELEM_GPS_LAT:
            h = value / 1000 / 60 / 60;
            m = (value - h * 1000 * 60 * 60) / 1000 / 60;
            s = (value - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = value % 1000;
            sprintf(str, "%03d %02d %02d.%03d", h, m, s, ss);
            break;
        case TELEM_GPS_ALT:
            sprintf(str, "%d.%03dm", (int)value / 1000, (int)value % 1000);
            break;
        case TELEM_GPS_SPEED:
            sprintf(str, "%d.%03dm/s", (int)value / 1000, (int)value % 1000);
            break;
        case TELEM_GPS_TIME:
        {
            int year = 2000 + (((u32)Telemetry.gps.time >> 26) & 0x3F);
            int month = ((u32)Telemetry.gps.time >> 22) & 0x0F;
            int day = ((u32)Telemetry.gps.time >> 17) & 0x1F;
            int hour = ((u32)Telemetry.gps.time >> 12) & 0x1F;
            int min = ((u32)Telemetry.gps.time >> 6) & 0x3F;
            int sec = ((u32)Telemetry.gps.time >> 0) & 0x3F;
            sprintf(str, "%02d:%02d:%02d %04d-%02d-%02d",
                    hour, min, sec, year, month, day);
            break;
        }
    }
    return str;
}

const char * TELEMETRY_GetValueStr(char *str, u8 telem)
{
    s32 value = TELEMETRY_GetValue(telem);
    return TELEMETRY_GetValueStrByValue(str, telem, value);
}

const char * TELEMETRY_Name(char *str, u8 telem)
{
    switch (telem) {
      case TELEM_VOLT1:
      case TELEM_VOLT2:
      case TELEM_VOLT3:
        sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_VOLT1 + 1);
        break;
      case TELEM_TEMP1:
      case TELEM_TEMP2:
      case TELEM_TEMP3:
      case TELEM_TEMP4:
        sprintf(str, "%s%d", _tr("TelemT"), telem - TELEM_TEMP1 + 1);
        break;
      case TELEM_RPM1:
      case TELEM_RPM2:
        sprintf(str, "%s%d", _tr("TelemRPM"), telem - TELEM_RPM1 + 1);
        break;
      default:
        return "";
    }
    return str;
}

const char * TELEMETRY_ShortName(char *str, u8 telem)
{
    switch(telem) {
        case TELEM_VOLT1: sprintf(str, "%s%d", _tr("Volt"), 1); break;
        case TELEM_VOLT2: sprintf(str, "%s%d", _tr("Volt"), 2); break;
        case TELEM_VOLT3: sprintf(str, "%s%d", _tr("Volt"), 3); break;
        case TELEM_RPM1:  sprintf(str, "%s%d",  _tr("RPM"), 1);  break;
        case TELEM_RPM2:  sprintf(str, "%s%d",  _tr("RPM"), 2);  break;
        case TELEM_TEMP1: sprintf(str, "%s%d", _tr("Temp"), 1); break;
        case TELEM_TEMP2: sprintf(str, "%s%d", _tr("Temp"), 2); break;
        case TELEM_TEMP3: sprintf(str, "%s%d", _tr("Temp"), 3); break;
        case TELEM_TEMP4: sprintf(str, "%s%d", _tr("Temp"), 4); break;
        case TELEM_GPS_LONG:   strcpy(str, _tr("Longitude")); break;
        case TELEM_GPS_LAT:    strcpy(str, _tr("Latitude")); break;
        case TELEM_GPS_ALT:    strcpy(str, _tr("Altitude")); break;
        case TELEM_GPS_SPEED:  strcpy(str, _tr("Speed")); break;
        case TELEM_GPS_TIME:   strcpy(str, _tr("Time")); break;
        default: str[0] = '\0'; break;
    }
    return str;
}
s32 TELEMETRY_GetMaxValue(u8 telem)
{
    switch(telem) {
        case TELEM_VOLT1:
        case TELEM_VOLT2:
        case TELEM_VOLT3:
            return 255;
        case TELEM_RPM1:
        case TELEM_RPM2:
            return 20000;
        case TELEM_TEMP1:
        case TELEM_TEMP2:
        case TELEM_TEMP3:
        case TELEM_TEMP4:
            return 255;
        default:
            return 0;
    }
}
