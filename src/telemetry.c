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
#include "music.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

struct Telemetry Telemetry;
static u32 alarm_duration[TELEM_NUM_ALARMS] = {0, 0, 0, 0, 0, 0};
static u8 telem_idx = 0;
static u8 alarm = 0;
static u32 alarm_time = 0;
static u32 last_updated;
static u32 last_time;
#define CHECK_DURATION 500
#define MUSIC_INTERVAL 2000 // DON'T need to play music in every 100ms

u32 TELEMETRY_IsUpdated()
{
    return last_updated | Telemetry.updated;
}

s32 TELEMETRY_GetValue(int idx)
{
    return _TELEMETRY_GetValue(&Telemetry, idx);
}

s32 _TELEMETRY_GetValue(struct Telemetry *t, int idx)
{
    switch (idx) {
    case TELEM_VOLT1:
    case TELEM_VOLT2:
    case TELEM_VOLT3:
        return t->volt[idx - TELEM_VOLT1];
    case TELEM_TEMP1:
    case TELEM_TEMP2:
    case TELEM_TEMP3:
    case TELEM_TEMP4:
        return t->temp[idx - TELEM_TEMP1];
    case TELEM_RPM1:
    case TELEM_RPM2:
        return t->rpm[idx - TELEM_RPM1];
    case TELEM_GPS_LONG:
        return t->gps.longitude;
    case TELEM_GPS_LAT:
        return t->gps.latitude;
    case TELEM_GPS_ALT:
        return t->gps.altitude;
    case TELEM_GPS_SPEED:
        return t->gps.velocity;
    case TELEM_GPS_TIME:
        return t->gps.time;
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
                if (Transmitter.telem & TELEMUNIT_FAREN) {
                    sprintf(str, "%dF", ((int)value * 9 + 160)/ 5);
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
            if (Transmitter.telem & TELEMUNIT_FEET) {
                value = value * 328 / 100;
                sprintf(str, "%d.%03dft", (int)value / 1000, (int)value % 1000);
            } else {
                sprintf(str, "%d.%03dm", (int)value / 1000, (int)value % 1000);
            }
            break;
        case TELEM_GPS_SPEED:
            if (Transmitter.telem & TELEMUNIT_FEET) {
                value = value * 2237 / 1000;
                sprintf(str, "%d.%02dmph", (int)value / 1000, (int)(value / 10) % 100);
            } else {
                sprintf(str, "%d.%03dm/s", (int)value / 1000, (int)value % 1000);
            }
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
        case 0: strcpy(str, _tr("None")); break;
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

//#define DEBUG_TELEMALARM
void TELEMETRY_Alarm()
{
    //Update 'updated' state every time we get here
    u32 current_time = CLOCK_getms();
    if (current_time - last_time > TELEM_ERROR_TIME) {
        last_time = current_time;
        last_updated = Telemetry.updated;
        Telemetry.updated = 0;
    }
    // don't need to check all the 6 telem-configs at one time, this is not a critical and urgent task
    // instead, check 1 of them at a time
    telem_idx = (telem_idx + 1) % TELEM_NUM_ALARMS;
    if(! Model.telem_alarm[telem_idx]) {
        alarm &= ~(1 << telem_idx); // clear this set
        return;
    }
    u8 idx = Model.telem_alarm[telem_idx];
    s32 value = TELEMETRY_GetValue(idx);
    if (value == 0) {
        alarm &= ~(1 << telem_idx); // clear this set
        return;
    }

    if (! TELEMETRY_IsUpdated()) {
        // bug fix: do not alarm when no telem packet is received, it might caused by RX is powered off
        alarm &= ~(1 << telem_idx); // clear this set
        return;
    }

    if (Model.telem_flags & (1 << telem_idx)) {
        if (! (alarm & (1 << telem_idx)) && (value <= Model.telem_alarm_val[telem_idx])) {
            if (alarm_duration[telem_idx] == 0) {
                alarm_duration[telem_idx] = current_time;
            } else if (current_time - alarm_duration[telem_idx] > CHECK_DURATION) {
                alarm_duration[telem_idx] = 0;
                alarm |= 1 << telem_idx;
#ifdef DEBUG_TELEMALARM
                printf("set: 0x%x\n\n", alarm);
#endif
            }
        } else if ((alarm & (1 << telem_idx)) && (value > (s32)Model.telem_alarm_val[telem_idx])) {
            if (alarm_duration[telem_idx] == 0) {
                alarm_duration[telem_idx] = current_time;
            } else if (current_time - alarm_duration[telem_idx] > CHECK_DURATION) {
                alarm_duration[telem_idx] = 0;
                alarm &= ~(1 << telem_idx);
#ifdef DEBUG_TELEMALARM
                printf("clear: 0x%x\n\n", alarm);
#endif
            }
        } else
            alarm_duration[telem_idx] = 0;
    } else {
        if (! (alarm & (1 << telem_idx)) && (value >= Model.telem_alarm_val[telem_idx])) {
            if (alarm_duration[telem_idx] == 0) {
                alarm_duration[telem_idx] = current_time;
            } else if (current_time - alarm_duration[telem_idx] > CHECK_DURATION) {
                alarm_duration[telem_idx] = 0;
                alarm |= 1 << telem_idx;
#ifdef DEBUG_TELEMALARM
                printf("set: 0x%x\n\n", alarm);
#endif
            }
        } else if ((alarm & (1 << telem_idx)) && (value < (s32)Model.telem_alarm_val[telem_idx])) {
            if (alarm_duration[telem_idx] == 0) {
                alarm_duration[telem_idx] = current_time;
            } else if (current_time - alarm_duration[telem_idx] > CHECK_DURATION) {
                alarm_duration[telem_idx] = 0;
                alarm &= ~(1 << telem_idx);
#ifdef DEBUG_TELEMALARM
                printf("clear: 0x%x\n\n", alarm);
#endif
            }
        } else
            alarm_duration[telem_idx] = 0;
    }

    if ((alarm & (1 << telem_idx))) {
        if (current_time >= alarm_time + MUSIC_INTERVAL) {
            alarm_time = current_time;
#ifdef DEBUG_TELEMALARM
            printf("beep: %d\n\n", telem_idx);
#endif
            MUSIC_Play(MUSIC_TELEMALARM1 + telem_idx);
        }
    }
}

int TELEMETRY_HasAlarm(int src)
{
    for(int i = 0; i < TELEM_NUM_ALARMS; i++)
        if(Model.telem_alarm[i] == src && (alarm & (1 << i)))
            return 1;
    return 0;
}
