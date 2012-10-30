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
#include "telemetry.h"

struct Telemetry Telemetry;
s32 TELEMETRY_GetValue(int idx)
{
    if(idx < 3) {
        return Telemetry.volt[idx];
    } else if(idx < 7) {
        return Telemetry.temp[idx-3];
    } else if(idx < 9) {
        return Telemetry.rpm[idx-7];
    }
    return 0;
}

const char * TELEMETRY_GetValueStr(char *str, u8 telem)
{
    int h, m, s, ss;
    switch(telem) {
        case TELEM_VOLT1:
        case TELEM_VOLT2:
        case TELEM_VOLT3:
            sprintf(str, "%d.%dV",
                    (int)Telemetry.volt[telem-TELEM_VOLT1] /10,
                    (int)Telemetry.volt[telem-TELEM_VOLT1] % 10);
            break;
        case TELEM_TEMP1:
        case TELEM_TEMP2:
        case TELEM_TEMP3:
        case TELEM_TEMP4:
            if (Telemetry.temp[telem-TELEM_TEMP1] == 0) {
                strcpy(str, "----");
            } else {
                sprintf(str, "%dC", (int)Telemetry.temp[telem-TELEM_TEMP1]);
            }
            break;
        case TELEM_RPM1:
        case TELEM_RPM2:
            sprintf(str, "%d", (int)Telemetry.rpm[telem-TELEM_RPM1]);
            break;
        case TELEM_GPS_LONG:
            h = Telemetry.gps.longitude / 1000 / 60 / 60;
            m = (Telemetry.gps.longitude - h * 1000 * 60 * 60) / 1000 / 60;
            s = (Telemetry.gps.longitude - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = Telemetry.gps.longitude % 1000;
            sprintf(str, "%03d %02d %02d.%03d", h, m, s, ss);
            break;
        case TELEM_GPS_LAT:
            h = Telemetry.gps.latitude / 1000 / 60 / 60;
            m = (Telemetry.gps.latitude - h * 1000 * 60 * 60) / 1000 / 60;
            s = (Telemetry.gps.latitude - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = Telemetry.gps.latitude % 1000;
            sprintf(str, "%03d %02d %02d.%03d", h, m, s, ss);
            break;
        case TELEM_GPS_ALT:
            sprintf(str, "%d.%03dm", (int)Telemetry.gps.altitude / 1000, (int)Telemetry.gps.altitude % 1000);
            break;
        case TELEM_GPS_SPEED:
            sprintf(str, "%d.%03dm/s", (int)Telemetry.gps.velocity / 1000, (int)Telemetry.gps.velocity % 1000);
            break;
        case TELEM_GPS_TIME:
            sprintf(str, "%02d:%02d:%02d %04d-%02d-%02d",
                    (int)Telemetry.gps.hour, (int)Telemetry.gps.min, (int)Telemetry.gps.sec,
                    (int)Telemetry.gps.year, (int)Telemetry.gps.month, (int)Telemetry.gps.day);
            break;
    }
    return str;
}

const char * TELEMETRY_Name(char *str, u8 telem)
{
    if(telem < 3) {
        sprintf(str, "%s%d", _tr("TelemV"), telem+1);
    } else if(telem < 7) {
        sprintf(str, "%s%d", _tr("TelemT"), telem - 2);
    } else if(telem < 9) {
        sprintf(str, "%s%d", _tr("TelemRPM"), telem - 6);
    } else {
        return "";
    }
    return str;
}
