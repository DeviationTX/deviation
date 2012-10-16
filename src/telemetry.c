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
    (void)idx;
    if(idx == 0)
        return Telemetry.volt[1];
    else
        return Telemetry.volt[2];
}

const char * TELEMETRY_SetString(char *str, u8 telem)
{
    if(telem < 3) {
        sprintf(str, "%d.%dV", (int)Telemetry.volt[telem] /10, (int)Telemetry.volt[telem] % 10);
    } else if(telem < 7) {
        sprintf(str, "%dC", (int)Telemetry.temp[telem-3]);
    } else if(telem < 10) {
        sprintf(str, "%d", (int)Telemetry.rpm[telem-7]);
    }
    return str;
}

const char * TELEMETRY_GetGPS(char *str, u8 line)
{
    int h, m, s, ss;
    switch(line) {
    case 0: 
        h = Telemetry.gps.longitude / 1000 / 60 / 60;
        m = (Telemetry.gps.longitude - h * 1000 * 60 * 60) / 1000 / 60;
        s = (Telemetry.gps.longitude - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
        ss = Telemetry.gps.longitude % 1000;
        sprintf(str, "Long: %03d %02d %02d.%03d", h, m, s, ss);
        break;
    case 1: 
        h = Telemetry.gps.latitude / 1000 / 60 / 60;
        m = (Telemetry.gps.latitude - h * 1000 * 60 * 60) / 1000 / 60;
        s = (Telemetry.gps.latitude - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
        ss = Telemetry.gps.latitude % 1000;
        sprintf(str, "Lat: %03d %02d %02d.%03d", h, m, s, ss);
        break;
    case 2:
        sprintf(str, "Alt: %d.%03dm", (int)Telemetry.gps.altitude / 1000, (int)Telemetry.gps.altitude % 1000);
        break;
    case 3:
        sprintf(str, "Speed: %d.%03dm/s", (int)Telemetry.gps.velocity / 1000, (int)Telemetry.gps.velocity % 1000);
        break;
    case 4:
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
    } else if(telem < 10) {
        sprintf(str, "%s%d", _tr("TelemRPM"), telem - 6);
    } else {
        return "";
    }
    return str;
}
