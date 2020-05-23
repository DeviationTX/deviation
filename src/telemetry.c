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

static void _get_value_str(char *str, s32 value, u8 decimals, char units);
static void _get_temp_str(char *str, s32 value, u8 decimals, char units);
static void _get_altitude_str(char *str, s32 value, u8 decimals, char units);
#include "telemetry/telem_devo.c"
#include "telemetry/telem_dsm.c"
#include "telemetry/telem_frsky.c"
#if HAS_EXTENDED_TELEMETRY
#include "telemetry/telem_crsf.c"
#endif  // HAS_EXTENDED_TELEMETRY

#define CAP_DSM   1
#define CAP_FRSKY 2
#define CAP_CRSF  4
#define CAP_TYPEMASK 0x07

struct Telemetry Telemetry;
static u8 telem_idx = 0;
static u32 last_updated[TELEM_UPDATE_SIZE] = {0};
static u32 music_time = 0;
static u32 error_time = 0;
#define CHECK_DURATION 500

void _get_value_str(char *str, s32 value, u8 decimals, char units)
{
    char format[] = "%0*d";
    format[2] = '1' + decimals;
    if (value < 0) {
        *str++ = '-';    // work-around tfp_format negative number bug
        value = -value;
    }
    sprintf(str, format, value);

    int i, len = strlen(str);
    if (decimals && len <= 2 + decimals && value) {
        for (i = len; i > len - decimals; i--) {
            str[i] = str[i-1];
        }
        str[i] = '.';
        len += (len < 4) ? 1 : 0;
    } else {
        len -= decimals;
    }
    str[len++] = units;
    str[len] = '\0';
}

void _get_temp_str(char *str, s32 value, u8 decimals, char units)
{
    if (Transmitter.telem & TELEMUNIT_FAREN) {
        if (units != 'F') {
            value = value ? (value * 9 + 160)/ 5 : 0;
            units = 'F';
        }
    } else if (units == 'F') {
        value = value ? (value - 32) * 5 / 9: 0; //Convert to degrees-C
        units = 'C';
    }
    _get_value_str(str, value, decimals, units);
}

void _get_altitude_str(char *str, s32 value, u8 decimals, char units)
{
    if (Transmitter.telem & TELEMUNIT_FEET) {
        if (units != '\'') {
            value = value ? value * 328 / 100 : 0;
            units = '\'';
        }
    } else if (units != 'm') {
        value = value ? value * 100 / 328 : 0;
        units = 'm';
    }
    _get_value_str(str, value, decimals, units);
}

u32 TELEMETRY_IsUpdated(int val)
{
    if (val == 0xff) {
        for(int i = 0; i < TELEM_UPDATE_SIZE; i++) {
            if (last_updated[i] | Telemetry.updated[i])
                return 1;
        }
        return 0;
    }
    return ((last_updated[val/32] | Telemetry.updated[val/32]) >> (val % 32)) & 1;
}

s32 TELEMETRY_GetValue(int idx)
{
    s32 value = _TELEMETRY_GetValue(&Telemetry, idx);
    return ((value & 0x7fff)==0x7fff) ? 0 : value;
}

s32 _TELEMETRY_GetValue(struct Telemetry *t, int idx)
{
    switch (idx) {
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
    case TELEM_GPS_HEADING:
        return t->gps.heading;
    case TELEM_GPS_SATCOUNT:
        return t->gps.satcount;
    }
    switch (TELEMETRY_Type()) {
    case TELEM_DEVO: return _devo_value(t, idx);
    case TELEM_DSM: return _dsm_value(t, idx);
    case TELEM_FRSKY: return _frsky_value(t, idx);
#if HAS_EXTENDED_TELEMETRY
    case TELEM_CRSF: return _crsf_value(t, idx);
#endif  // HAS_EXTENDED_TELEMETRY
    }
    return 0;
}

const char * TELEMETRY_GetValueStrByValue(char *str, int idx, s32 value)
{
    unsigned h, m, s, ss;
    char letter = ' ';
    int unit = 0;    // rBE-OPT: Optimizing string usage, saves some bytes
    switch(idx) {
        case TELEM_GPS_LONG:
            // allowed values: +/-180° = +/- 180*60*60*1000; W if value<0, E if value>=0; -180° = 180°
            if (value < 0) {
                letter = 'W';
                value = -value;
            } else
                letter = 'E';
            h = value / 1000 / 60 / 60;
            m = (value - h * 1000 * 60 * 60) / 1000 / 60;
            s = (value - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = value % 1000;
            sprintf(str, "%c %3u° %02u' %02u.%03u\"", letter, h, m, s, ss);
            break;
        case TELEM_GPS_LAT:
            // allowed values: +/-90° = +/- 90*60*60*1000; S if value<0, N if value>=0
            if (value < 0) {
                letter = 'S';
                value = -value;
            } else
                letter = 'N';
            h = value / 1000 / 60 / 60;
            m = (value - h * 1000 * 60 * 60) / 1000 / 60;
            s = (value - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = value % 1000;
            sprintf(str, "%c %3u° %02u' %02u.%03u\"", letter, h, m, s, ss);
            break;
        case TELEM_GPS_ALT:
            if (value < 0) {
                letter = '-';
                value = -value;
            }
            if (Transmitter.telem & TELEMUNIT_FEET) {
                value = value * 328 / 100;
                unit = 1;
            }
            sprintf(str, "%c%u.%03u%s", letter, (unsigned)value / 1000, (unsigned)value % 1000, unit ? "ft" : "m");
            break;
        case TELEM_GPS_SPEED:
            if (Transmitter.telem & TELEMUNIT_FEET) {
                value = value * 2237 / 1000;
                unit = 1;
            } else {
                value = value * 3600 / 1000;
            }
            sprintf(str, "%u.%03u%s", (unsigned)value / 1000, (unsigned)value % 1000, unit ? "mph" : "km/h");
            break;
        case TELEM_GPS_TIME:
        {
            unsigned year = 2000 + (((u32)Telemetry.gps.time >> 26) & 0x3F);
            unsigned month = ((u32)Telemetry.gps.time >> 22) & 0x0F;
            unsigned day = ((u32)Telemetry.gps.time >> 17) & 0x1F;
            unsigned hour = ((u32)Telemetry.gps.time >> 12) & 0x1F;
            unsigned min = ((u32)Telemetry.gps.time >> 6) & 0x3F;
            unsigned sec = ((u32)Telemetry.gps.time >> 0) & 0x3F;
            sprintf(str, "%2u:%02u:%02u %4u-%02u-%02u",
                    hour, min, sec, year, month, day);
            break;
        }
        case TELEM_GPS_SATCOUNT:    _get_value_str(str, value, 0, '\0'); break;
        case TELEM_GPS_HEADING:     _get_value_str(str, value, 1, '\0'); break;
        default:
            switch (TELEMETRY_Type()) {
            case TELEM_DEVO:  return _devo_str_by_value(str, idx, value);
            case TELEM_DSM:   return _dsm_str_by_value(str, idx, value);
            case TELEM_FRSKY: return _frsky_str_by_value(str, idx, value);
#if HAS_EXTENDED_TELEMETRY
            case TELEM_CRSF:  return _crsf_str_by_value(str, idx, value);
#endif  // HAS_EXTENDED_TELEMETRY
            }
    }
    return str;
}

const char * TELEMETRY_GetValueStr(char *str, int idx)
{
    s32 value = TELEMETRY_GetValue(idx);
    return TELEMETRY_GetValueStrByValue(str, idx, value);
}

const char * TELEMETRY_Name(char *str, int idx)
{
    switch (TELEMETRY_Type()) {
    case TELEM_DEVO:  return _devo_name(str, idx);
    case TELEM_DSM:   return _dsm_name(str, idx);
    case TELEM_FRSKY: return _frsky_name(str, idx);
#if HAS_EXTENDED_TELEMETRY
    case TELEM_CRSF:  return _crsf_name(str, idx);
#endif  // HAS_EXTENDED_TELEMETRY
    }
    return NULL;
}

const char * TELEMETRY_ShortName(char *str, int idx)
{
    switch(idx) {
        case TELEM_GPS_LONG:    strcpy(str, _tr("Longitude")); break;
        case TELEM_GPS_LAT:     strcpy(str, _tr("Latitude")); break;
        case TELEM_GPS_ALT:     strcpy(str, _tr("Altitude")); break;
        case TELEM_GPS_SPEED:   strcpy(str, _tr("Speed")); break;
        case TELEM_GPS_TIME:    strcpy(str, _tr("Time")); break;
        case TELEM_GPS_SATCOUNT:strcpy(str, _tr("SatCount")); break;
        case TELEM_GPS_HEADING: strcpy(str, _tr("Heading")); break;
        default:
            switch (TELEMETRY_Type()) {
            case TELEM_DEVO:  return _devo_short_name(str, idx);
            case TELEM_DSM:   return _dsm_short_name(str, idx);
            case TELEM_FRSKY: return _frsky_short_name(str, idx);
#if HAS_EXTENDED_TELEMETRY
            case TELEM_CRSF:  return _crsf_short_name(str, idx);
#endif  // HAS_EXTENDED_TELEMETRY
            }
    }
    return str;
}
s32 TELEMETRY_GetMaxValue(int idx)
{
    switch (TELEMETRY_Type()) {
    case TELEM_DEVO:  return _devo_get_max_value(idx);
    case TELEM_DSM:   return _dsm_get_max_value(idx);
    case TELEM_FRSKY: return _frsky_get_max_value(idx);
#if HAS_EXTENDED_TELEMETRY
    case TELEM_CRSF:  return _crsf_get_max_value(idx);
#endif  // HAS_EXTENDED_TELEMETRY
    }
    return 0;
}
s32 TELEMETRY_GetMinValue(int idx)
{
    switch (TELEMETRY_Type()) {
    case TELEM_DEVO:  return _devo_get_min_value(idx);
    case TELEM_DSM:   return _dsm_get_min_value(idx);
    case TELEM_FRSKY: return _frsky_get_min_value(idx);
#if HAS_EXTENDED_TELEMETRY
    case TELEM_CRSF:  return _crsf_get_min_value(idx);
#endif  // HAS_EXTENDED_TELEMETRY
    }
    return 0;
}

void TELEMETRY_SetUpdated(int idx)
{
    Telemetry.updated[idx/32] |= (1 << idx % 32);
}

int TELEMETRY_Type()
{
    if (Telemetry.capabilities & CAP_DSM)
        return TELEM_DSM;
    if (Telemetry.capabilities & CAP_FRSKY)
        return TELEM_FRSKY;
    if (Telemetry.capabilities & CAP_CRSF)
        return TELEM_CRSF;
    return TELEM_DEVO;
}

int TELEMETRY_GetNumTelemSrc()
{
    if (TELEMETRY_Type() == TELEM_DEVO)
        return TELEM_DEVO_LAST-1;
    if (TELEMETRY_Type() == TELEM_DSM)
        return TELEM_DSM_LAST-1;
    if (TELEMETRY_Type() == TELEM_CRSF)
        return TELEM_CRSF_LAST-1;
    return TELEM_FRSKY_LAST-1;
}

void TELEMETRY_SetType(int type)
{
    unsigned  cap = Telemetry.capabilities & ~CAP_TYPEMASK;
    if (type == TELEM_DSM)
        cap |= CAP_DSM;
    if (type == TELEM_FRSKY)
        cap |= CAP_FRSKY;
    if (type == TELEM_CRSF)
        cap |= CAP_CRSF;
    Telemetry.capabilities = cap;
}

//#define DEBUG_TELEMALARM
void TELEMETRY_Alarm()
{
    if (PROTOCOL_GetTelemetryState() != PROTO_TELEM_ON)
        return;

    //Update 'updated' state every time we get here
    u32 current_time = CLOCK_getms();
    if (current_time >= error_time) {
        error_time = current_time + TELEM_ERROR_TIME;
        for(int i = 0; i < TELEM_UPDATE_SIZE; i++) {
            last_updated[i] = Telemetry.updated[i];
            Telemetry.updated[i] = 0;
        }
    }
    // don't need to check all the 6 telem-configs at one time, this is not a critical and urgent task
    // instead, check 1 of them at a time
    telem_idx = (telem_idx + 1) % TELEM_NUM_ALARMS;
    struct TelemetryAlarm *alarm = &Model.alarms[telem_idx];

    if (current_time >= alarm->alarm_time) {
        alarm->alarm_time = current_time + CHECK_DURATION;
        if (!TELEMETRY_IsUpdated(alarm->src)) {
            TELEMETRY_ResetAlarm(telem_idx);
        } else if ((TELEMETRY_GetValue( alarm->src ) - alarm->mute_value <=
                                        alarm->value) == alarm->above) {
            if (!alarm->state) {
                alarm->state++;
                alarm->limit_threshold_time = current_time + (alarm->threshold * 1000);
#ifdef DEBUG_TELEMALARM
                printf("set: 0x%x\n\n", telem_idx);
#endif
            }
        } else if (alarm->state) {
            alarm->state = 0;
            alarm->limit_threshold_time = 0;
#ifdef DEBUG_TELEMALARM
            printf("clear: 0x%x\n\n", telem_idx);
#endif
        }
    }

    if (alarm->state == 1 &&
        current_time >= music_time &&
        current_time >= alarm->limit_threshold_time) {
        music_time = current_time + Transmitter.telem_alert_interval*1000;
        // telem_idx > 2 is exclude first 3 alarms from jump action (interim solution)
        // <= (9 + type) is limit jump action to only visible telemetry monitor values
        if (telem_idx > 2 && alarm->src <= (9 + TELEMETRY_Type()))
            PAGE_ShowTelemetryAlarm();
#ifdef DEBUG_TELEMALARM
        printf("beep: %d\n\n", telem_idx);
#endif

#if HAS_EXTENDED_AUDIO
        u16 telem_music = MUSIC_GetTelemetryAlarm(MUSIC_TELEMALARM1 + telem_idx);
        s32 telem_value = TELEMETRY_GetValue(alarm->src);
        if (TELEMETRY_Type() == TELEM_DEVO) {
            switch (alarm->src) {
                case TELEM_DEVO_VOLT1:
                case TELEM_DEVO_VOLT2:
                case TELEM_DEVO_VOLT3: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_VOLT,1); break;
                case TELEM_DEVO_RPM1:
                case TELEM_DEVO_RPM2: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_RPM,0); break;
                default: MUSIC_PlayValue(telem_music, telem_value-20,VOICE_UNIT_TEMP,0); break;
            }
        }
        if (TELEMETRY_Type() == TELEM_DSM) {
            switch (alarm->src) {
#if HAS_EXTENDED_TELEMETRY
                case TELEM_DSM_JETCAT_RPM:
                case TELEM_DSM_ESC_RPM:
#endif
                case TELEM_DSM_FLOG_RPM1: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_RPM,0); break;
#if HAS_EXTENDED_TELEMETRY
                case TELEM_DSM_PBOX_VOLT1:
                case TELEM_DSM_PBOX_VOLT2:
                case TELEM_DSM_JETCAT_PACKVOLT:
                case TELEM_DSM_JETCAT_PUMPVOLT:
                case TELEM_DSM_RXPCAP_VOLT:
                case TELEM_DSM_ESC_VOLT1:
                case TELEM_DSM_ESC_VOLT2:
#endif
                case TELEM_DSM_FLOG_VOLT1:
                case TELEM_DSM_FLOG_VOLT2: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_VOLT,2); break;
#if HAS_EXTENDED_TELEMETRY
                case TELEM_DSM_JETCAT_TEMPEGT: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_TEMP,0); break;
                case TELEM_DSM_ESC_TEMP1:
                case TELEM_DSM_ESC_TEMP2:
#endif
                case TELEM_DSM_FLOG_TEMP1: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_TEMP,1); break;
#if HAS_EXTENDED_TELEMETRY
                case TELEM_DSM_RXPCAP_AMPS:
                case TELEM_DSM_ESC_AMPS1: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_AMPS,2); break;
                case TELEM_DSM_FPCAP_AMPS:
                case TELEM_DSM_ESC_AMPS2:
#endif
                case TELEM_DSM_AMPS1: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_AMPS,1); break;
                case TELEM_DSM_ALTITUDE:
                case TELEM_DSM_ALTITUDE_MAX:
                case TELEM_DSM_VARIO_CLIMBRATE1:
                case TELEM_DSM_VARIO_CLIMBRATE2:
                case TELEM_DSM_VARIO_CLIMBRATE3:
                case TELEM_DSM_VARIO_CLIMBRATE4:
                case TELEM_DSM_VARIO_CLIMBRATE5:
                case TELEM_DSM_VARIO_CLIMBRATE6:
                case TELEM_DSM_VARIO_ALTITUDE: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_ALTITUDE,1); break;

                case TELEM_DSM_GFORCE_X:
                case TELEM_DSM_GFORCE_Y:
                case TELEM_DSM_GFORCE_Z:
                case TELEM_DSM_GFORCE_XMAX:
                case TELEM_DSM_GFORCE_YMAX:
                case TELEM_DSM_GFORCE_ZMAX:
                case TELEM_DSM_GFORCE_ZMIN: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_GFORCE,2); break;
#if HAS_EXTENDED_TELEMETRY
                case TELEM_DSM_FLOG_RSSI_DBM: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_DB,0); break;
#endif
                default: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_NONE,0);
          }
        }

        if (TELEMETRY_Type() == TELEM_FRSKY) {
            switch (alarm->src) {
#if HAS_EXTENDED_TELEMETRY
                case TELEM_FRSKY_VOLT3:
                case TELEM_FRSKY_VOLTA:
                case TELEM_FRSKY_MIN_CELL:
                case TELEM_FRSKY_ALL_CELL:
                case TELEM_FRSKY_CELL1:
                case TELEM_FRSKY_CELL2:
                case TELEM_FRSKY_CELL3:
                case TELEM_FRSKY_CELL4:
                case TELEM_FRSKY_CELL5:
                case TELEM_FRSKY_CELL6:
#endif
                case TELEM_FRSKY_VOLT1:
                case TELEM_FRSKY_VOLT2: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_VOLT,2); break;
#if HAS_EXTENDED_TELEMETRY
                case TELEM_FRSKY_TEMP1:
                case TELEM_FRSKY_TEMP2: MUSIC_PlayValue(telem_music, telem_value-20,VOICE_UNIT_TEMP,0); break;
                case TELEM_FRSKY_RPM: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_RPM,0); break;
                case TELEM_FRSKY_CURRENT: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_AMPS,2); break;
                case TELEM_FRSKY_ALTITUDE: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_ALTITUDE,2); break;
#endif
                case TELEM_FRSKY_LRSSI:
                case TELEM_FRSKY_RSSI: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_DB,0); break;
                default: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_NONE,0);
            }
        }

#if HAS_EXTENDED_TELEMETRY
        if (TELEMETRY_Type() == TELEM_CRSF) {
            switch (alarm->src) {
                case TELEM_CRSF_BATT_VOLTAGE:
                    MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_VOLT,2); break;
                case TELEM_CRSF_BATT_CURRENT: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_AMPS,2); break;
                case TELEM_CRSF_GPS_ALTITUDE: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_ALTITUDE,2); break;
                case TELEM_CRSF_TX_SNR:
                case TELEM_CRSF_TX_RSSI:
                case TELEM_CRSF_RX_SNR:
                case TELEM_CRSF_RX_RSSI1:
                case TELEM_CRSF_RX_RSSI2: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_DB,0); break;
                default: MUSIC_PlayValue(telem_music, telem_value,VOICE_UNIT_NONE,0);
            }
        }
#endif //HAS_EXTENDED_TELEMETRY

#else
        MUSIC_Play(MUSIC_TELEMALARM1 + telem_idx);
#endif //HAS_EXTENDED_AUDIO

    }
}

void TELEMETRY_ResetAlarm(int i)
{
    struct TelemetryAlarm *alarm = &Model.alarms[i];
    alarm->state = 0;
    alarm->mute_value = 0;
}

void TELEMETRY_ResetValues(void)
{
    // Reset cumulative values, altitude ground level, etc.
    // Exact values are protocol depenedent
    PROTOCOL_ResetTelemetry();
    SOUND_SetFrequency(3951, Transmitter.volume * 10);
    SOUND_StartWithoutVibrating(100, NULL);
}

void TELEMETRY_MuteAlarm()
{
    for (int i = 0; i < TELEM_NUM_ALARMS; i++) {
        struct TelemetryAlarm *alarm = &Model.alarms[i];
        if (alarm->state == 1) {
            alarm->mute_value = TELEMETRY_GetValue(alarm->src) - (alarm->above << 8);
            alarm->state++;
        }
    }
}

int TELEMETRY_HasAlarm(int src)
{
    for (int i = 0; i < TELEM_NUM_ALARMS; i++) {
        struct TelemetryAlarm *alarm = &Model.alarms[i];
        if (alarm->src == src)
            return (alarm->state == 1);
    }

    return 0;
}
