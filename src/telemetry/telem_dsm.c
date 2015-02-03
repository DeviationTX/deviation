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

#if HAS_DSM_EXTENDED_TELEMETRY
const char * _dsm_jetcat_status(u8 idx)
{
    const char * text[] = {
    //Possible messages for status:
    /*0x00:*/"OFF",
    /*0x01:*/"WAIT FOR RPM",
    /*0x02:*/"IGNITE",
    /*0x03:*/"ACCELERATE",
    /*0x04:*/"STABILIZE",
    /*0x05:*/"LEARN HIGH",
    /*0x06:*/"LEARN LOW",
    /*0x07:undef:*/"?",
    /*0x08:*/"SLOW DOWN",
    /*0x09:*/"MANUAL",
    /*0x0a,0x10:*/"AUTO OFF",
    /*0x0b,0x11:*/"RUN",
    /*0x0c,0x12:*/"ACCELERATION DELAY",
    /*0x0d,0x13:*/"SPEED REG",
    /*0x0e,0x14:*/"TWO SHAFT REGULATE",
    /*0x0f,0x15:*/"PRE HEAT",
    /*0x16:*/"PRE HEAT 2",
    /*0x17:*/"MAIN F START",
    /*0x18:not used:*/"",
    /*0x19:*/"KERO FULL ON",
    /*0x1a:*/"MAX STATE"};
    if (idx > 15)
        idx -= 6;
    if (idx <= sizeof(text)+1)
        return text[idx];
    else
        return "";
}
const char * _dsm_jetcat_offcond(u8 idx)
{
    const char * text[] = {
    //Messages for Off_Condition:
    /*0x00:*/"NA",
    /*0x01:*/"OFF BY RC",
    /*0x02:*/"OVER TEMPERATURE",
    /*0x03:*/"IGNITION TIMEOUT",
    /*0x04:*/"ACCELERATION TIMEOUT",
    /*0x05:*/"ACCELERATION TOO SLOW",
    /*0x06:*/"OVER RPM",
    /*0x07:*/"LOW RPM OFF",
    /*0x08:*/"LOW BATTERY",
    /*0x09:*/"AUTO OFF",
    /*0x0a,0x10:*/"LOW TEMP OFF",
    /*0x0b,0x11:*/"HIGH TEMP OFF",
    /*0x0c,0x12:*/"GLOW PLUG DEFECTIVE",
    /*0x0d,0x13:*/"WATCH DOG TIMER",
    /*0x0e,0x14:*/"FAIL SAFE OFF",
    /*0x0f,0x15:*/"MANUAL OFF",
    /*0x16:*/"POWER BATT FAIL",
    /*0x17:*/"TEMP SENSOR FAIL",
    /*0x18:*/"FUEL FAIL",
    /*0x19:*/"PROP FAIL",
    /*0x1a:*/"2nd ENGINE FAIL",
    /*0x1b:*/"2nd ENGINE DIFFERENTIAL TOO HIGH",
    /*0x1c:*/"2nd ENGINE NO COMMUNICATION",
    /*0x1d:*/"MAX OFF CONDITION"};
    if (idx > 15)
        idx -= 6;
    if (idx <= sizeof(text)+1)
        return text[idx];
    else
        return "";
}
#endif //HAS_DSM_EXTENDED_TELEMETRY

s32 _dsm_value(struct Telemetry *_t, int idx)
{
    struct telem_dsm *t = &_t->p.dsm;
    switch (idx) {
        case TELEM_DSM_FLOG_FADESA:
        case TELEM_DSM_FLOG_FADESB:
        case TELEM_DSM_FLOG_FADESL:
        case TELEM_DSM_FLOG_FADESR:     return t->flog.fades[idx - TELEM_DSM_FLOG_FADESA];
        case TELEM_DSM_FLOG_FRAMELOSS:  return t->flog.frameloss;
        case TELEM_DSM_FLOG_HOLDS:      return t->flog.holds;
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:      return t->flog.volt[idx - TELEM_DSM_FLOG_VOLT1];
        case TELEM_DSM_FLOG_RPM1:       return t->flog.rpm;
        case TELEM_DSM_FLOG_TEMP1:      return t->flog.temp;
#if HAS_DSM_EXTENDED_TELEMETRY
        case TELEM_DSM_AMPS1:           return t->sensors.amps;
        case TELEM_DSM_PBOX_VOLT1:
        case TELEM_DSM_PBOX_VOLT2:      return t->pbox.volt[idx - TELEM_DSM_PBOX_VOLT1];
        case TELEM_DSM_PBOX_CAPACITY1:
        case TELEM_DSM_PBOX_CAPACITY2:  return t->pbox.capacity[idx - TELEM_DSM_PBOX_CAPACITY1];
        case TELEM_DSM_PBOX_ALARMV1:
        case TELEM_DSM_PBOX_ALARMV2:    return t->pbox.alarmv[idx - TELEM_DSM_PBOX_ALARMV1];
        case TELEM_DSM_PBOX_ALARMC1:
        case TELEM_DSM_PBOX_ALARMC2:    return t->pbox.alarmc[idx - TELEM_DSM_PBOX_ALARMC1];
        case TELEM_DSM_AIRSPEED:        return t->sensors.airspeed;
        case TELEM_DSM_ALTITUDE:        return t->sensors.altitude;
        case TELEM_DSM_GFORCE_X:        return t->gforce.x;
        case TELEM_DSM_GFORCE_Y:        return t->gforce.y;
        case TELEM_DSM_GFORCE_Z:        return t->gforce.z;
        case TELEM_DSM_GFORCE_XMAX:     return t->gforce.xmax;
        case TELEM_DSM_GFORCE_YMAX:     return t->gforce.ymax;
        case TELEM_DSM_GFORCE_ZMAX:     return t->gforce.zmax;
        case TELEM_DSM_GFORCE_ZMIN:     return t->gforce.zmin;
        case TELEM_DSM_JETCAT_STATUS:   return t->jetcat.status;
        case TELEM_DSM_JETCAT_THROTTLE: return t->jetcat.throttle;
        case TELEM_DSM_JETCAT_PACKVOLT: return t->jetcat.packvolt;
        case TELEM_DSM_JETCAT_PUMPVOLT: return t->jetcat.pumpvolt;
        case TELEM_DSM_JETCAT_RPM:      return t->jetcat.rpm;
        case TELEM_DSM_JETCAT_TEMPEGT:  return t->jetcat.temp_egt;
        case TELEM_DSM_JETCAT_OFFCOND:  return t->jetcat.offcond;
#endif
        default:  return 999;
    }
}

const char * _dsm_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
        case TELEM_DSM_FLOG_FADESA:
        case TELEM_DSM_FLOG_FADESB:
        case TELEM_DSM_FLOG_FADESL:
        case TELEM_DSM_FLOG_FADESR:
        case TELEM_DSM_FLOG_FRAMELOSS:
        case TELEM_DSM_FLOG_HOLDS:
        case TELEM_DSM_FLOG_RPM1:       _get_value_str(str, value, 0, '\0'); break;
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:      _get_value_str(str, value, 2, 'V'); break;
        case TELEM_DSM_FLOG_TEMP1:      _get_temp_str(str, value, 0); break;
#if HAS_DSM_EXTENDED_TELEMETRY
        case TELEM_DSM_AMPS1:           _get_value_str(str, value, 1, 'A'); break;
        case TELEM_DSM_AIRSPEED:        _get_value_str(str, value, 0, '\0'); break;
        case TELEM_DSM_ALTITUDE:        _get_value_str(str, value, 1, 'm'); break;
        case TELEM_DSM_PBOX_CAPACITY1:
        case TELEM_DSM_PBOX_CAPACITY2:  _get_value_str(str, value, 0, '\0'); break;
        case TELEM_DSM_GFORCE_X:
        case TELEM_DSM_GFORCE_Y:
        case TELEM_DSM_GFORCE_Z:
        case TELEM_DSM_GFORCE_XMAX:
        case TELEM_DSM_GFORCE_YMAX:
        case TELEM_DSM_GFORCE_ZMAX:
        case TELEM_DSM_GFORCE_ZMIN:     _get_value_str(str, value, 2, 'g'); break;
        case TELEM_DSM_PBOX_VOLT1:
        case TELEM_DSM_PBOX_VOLT2:
        case TELEM_DSM_JETCAT_PACKVOLT:
        case TELEM_DSM_JETCAT_PUMPVOLT: _get_value_str(str, value, 2, 'V'); break;
        case TELEM_DSM_JETCAT_TEMPEGT:  _get_temp_str(str, value, 0); break;
        case TELEM_DSM_JETCAT_STATUS:   _dsm_jetcat_status(value); break;
        case TELEM_DSM_JETCAT_OFFCOND:  _dsm_jetcat_offcond(value); break;
#endif
        default:  strcpy(str, "Unknown");
    }
    return str;
}
const char * _dsm_name(char *str, u8 telem)
{
    switch (telem) {
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:      sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_DSM_FLOG_VOLT1 + 1); break;
        case TELEM_DSM_FLOG_TEMP1:      strcpy(str, _tr("TelemT")); break;
        case TELEM_DSM_FLOG_RPM1:       strcpy(str, _tr("TelemRPM")); break;
        case TELEM_DSM_FLOG_FADESA:     sprintf(str, "%s%c", _tr("Fades"), 'A'); break;
        case TELEM_DSM_FLOG_FADESB:     sprintf(str, "%s%c", _tr("Fades"), 'B'); break;
        case TELEM_DSM_FLOG_FADESL:     sprintf(str, "%s%c", _tr("Fades"), 'L'); break;
        case TELEM_DSM_FLOG_FADESR:     sprintf(str, "%s%c", _tr("Fades"), 'R'); break;
        case TELEM_DSM_FLOG_FRAMELOSS:  strcpy(str, _tr("Loss")); break;
        case TELEM_DSM_FLOG_HOLDS:      strcpy(str, _tr("Holds")); break;
#if HAS_DSM_EXTENDED_TELEMETRY
        case TELEM_DSM_AMPS1:           strcpy(str, _tr("Amps")); break;
        case TELEM_DSM_PBOX_VOLT1:
        case TELEM_DSM_PBOX_VOLT2:      sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_DSM_PBOX_VOLT1 + 1); break;
        case TELEM_DSM_PBOX_CAPACITY1:
        case TELEM_DSM_PBOX_CAPACITY2:  sprintf(str, "%s%d mAh", _tr("Bat"), telem - TELEM_DSM_PBOX_CAPACITY1 + 1); break;
        case TELEM_DSM_PBOX_ALARMV1:
        case TELEM_DSM_PBOX_ALARMV2:    sprintf(str, "%s%d", _tr("AlarmV"), telem - TELEM_DSM_PBOX_ALARMV1 + 1); break;
        case TELEM_DSM_PBOX_ALARMC1:
        case TELEM_DSM_PBOX_ALARMC2:    sprintf(str, "%s%d", _tr("AlarmC"), telem - TELEM_DSM_PBOX_ALARMC1 + 1); break;
        case TELEM_DSM_AIRSPEED:        sprintf(str, "%s km/h", _tr("Air")); break;
        case TELEM_DSM_ALTITUDE:        strcpy(str, _tr("Altitude")); break;
        case TELEM_DSM_GFORCE_X:        strcpy(str, "g -> X"); break;
        case TELEM_DSM_GFORCE_Y:        strcpy(str, "g -> Y"); break;
        case TELEM_DSM_GFORCE_Z:        strcpy(str, "g -> Z"); break;
        case TELEM_DSM_GFORCE_XMAX:     strcpy(str, "g max X"); break;
        case TELEM_DSM_GFORCE_YMAX:     strcpy(str, "g max Y"); break;
        case TELEM_DSM_GFORCE_ZMAX:     strcpy(str, "g max Z"); break;
        case TELEM_DSM_GFORCE_ZMIN:     strcpy(str, "g min Z"); break;
        case TELEM_DSM_JETCAT_STATUS:   strcpy(str, _tr("Status")); break;
        case TELEM_DSM_JETCAT_THROTTLE: strcpy(str, _tr("Throttle")); break;
        case TELEM_DSM_JETCAT_PACKVOLT: strcpy(str, _tr("PackVolt")); break;
        case TELEM_DSM_JETCAT_PUMPVOLT: strcpy(str, _tr("PumpVolt")); break;
        case TELEM_DSM_JETCAT_RPM:      strcpy(str, _tr("RPM")); break;
        case TELEM_DSM_JETCAT_TEMPEGT:  strcpy(str, _tr("TempEGT")); break;
        case TELEM_DSM_JETCAT_OFFCOND:  strcpy(str, _tr("Offcond")); break;
#endif
        default:
            return "";
    }
    return str;
}

static const char * _dsm_short_name(char *str, u8 telem)
{
    switch(telem) {
        case 0: strcpy(str, _tr("None")); break;
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:
            sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_DSM_FLOG_VOLT1 + 1);
            break;
        case TELEM_DSM_FLOG_RPM1:  strcpy(str, _tr("RPM")); break;
        case TELEM_DSM_FLOG_TEMP1: strcpy(str, _tr("Temp")); break;
        default: return _dsm_name(str, telem);
    }
    return str;
}

s32 _dsm_get_max_value(u8 telem)
{           
    switch(telem) {
        case TELEM_DSM_FLOG_FADESA:
        case TELEM_DSM_FLOG_FADESB:
        case TELEM_DSM_FLOG_FADESL:
        case TELEM_DSM_FLOG_FADESR:
        case TELEM_DSM_FLOG_FRAMELOSS:
        case TELEM_DSM_FLOG_TEMP1:  return 9999;
        case TELEM_DSM_FLOG_HOLDS:  return 999;
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:  return 65000;   // could not find maximum spec
        case TELEM_DSM_FLOG_RPM1:   return 999999;
        default: return 0;
    }
}

s32 _dsm_get_min_value(u8 telem)
{           
    switch(telem) {
        case TELEM_DSM_FLOG_TEMP1:  return -150;
        case TELEM_DSM_FLOG_RPM1:   return 200;
        default: return 0;
    }
}
