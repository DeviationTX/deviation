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
#endif //HAS_EXTENDED_TELEMETRY

s32 _dsm_value(struct Telemetry *t, int idx)
{
    switch (idx) {
        case TELEM_DSM_FLOG_TEMP1:
        case TELEM_DSM_AMPS1:
        case TELEM_DSM_ALTITUDE:
        case TELEM_DSM_ALTITUDE_MAX:
        case TELEM_DSM_GFORCE_X:
        case TELEM_DSM_GFORCE_Y:
        case TELEM_DSM_GFORCE_Z:
        case TELEM_DSM_GFORCE_XMAX:
        case TELEM_DSM_GFORCE_YMAX:
        case TELEM_DSM_GFORCE_ZMAX:
        case TELEM_DSM_GFORCE_ZMIN:
#if HAS_EXTENDED_TELEMETRY
        case TELEM_DSM_RXPCAP_AMPS:
        case TELEM_DSM_RXPCAP_CAPACITY:
        case TELEM_DSM_FPCAP_AMPS:
        case TELEM_DSM_FPCAP_CAPACITY:
#endif
        case TELEM_DSM_VARIO_ALTITUDE:
        case TELEM_DSM_VARIO_CLIMBRATE1:
        case TELEM_DSM_VARIO_CLIMBRATE2:
        case TELEM_DSM_VARIO_CLIMBRATE3:
        case TELEM_DSM_VARIO_CLIMBRATE4:
        case TELEM_DSM_VARIO_CLIMBRATE5:
        case TELEM_DSM_VARIO_CLIMBRATE6:
            return (s16)t->value[idx];
        default:
            return t->value[idx];
    }
}

const char * _dsm_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
        case TELEM_DSM_AIRSPEED:
        case TELEM_DSM_FLOG_FADESA:
        case TELEM_DSM_FLOG_FADESB:
        case TELEM_DSM_FLOG_FADESL:
        case TELEM_DSM_FLOG_FADESR:
        case TELEM_DSM_FLOG_FRAMELOSS:
        case TELEM_DSM_FLOG_HOLDS:
        case TELEM_DSM_FLOG_RPM1:       _get_value_str(str, value, 0, '\0'); break;
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:      _get_value_str(str, value, 2, 'V'); break;
        case TELEM_DSM_FLOG_TEMP1:      _get_temp_str(str, value, 0, 'C'); break;
        case TELEM_DSM_AMPS1:           _get_value_str(str, value * 196791 / 100000, 1, 'A'); break;
        case TELEM_DSM_ALTITUDE:        
        case TELEM_DSM_ALTITUDE_MAX:    _get_altitude_str(str, value, 1, 'm'); break;
        case TELEM_DSM_GFORCE_X:
        case TELEM_DSM_GFORCE_Y:
        case TELEM_DSM_GFORCE_Z:
        case TELEM_DSM_GFORCE_XMAX:
        case TELEM_DSM_GFORCE_YMAX:
        case TELEM_DSM_GFORCE_ZMAX:
        case TELEM_DSM_GFORCE_ZMIN:     _get_value_str(str, value, 2, 'g'); break;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_DSM_PBOX_ALARMV1:
        case TELEM_DSM_PBOX_ALARMV2:
        case TELEM_DSM_PBOX_ALARMC1:
        case TELEM_DSM_PBOX_ALARMC2:    strcpy(str, _tr(value?"On":"Off")); break;
        case TELEM_DSM_PBOX_CAPACITY1:
        case TELEM_DSM_PBOX_CAPACITY2:
        case TELEM_DSM_FPCAP_CAPACITY:
        case TELEM_DSM_JETCAT_RPM:
        case TELEM_DSM_ESC_RPM:         _get_value_str(str, value, 0, '\0'); break;
        case TELEM_DSM_PBOX_VOLT1:
        case TELEM_DSM_PBOX_VOLT2:
        case TELEM_DSM_JETCAT_PACKVOLT:
        case TELEM_DSM_JETCAT_PUMPVOLT:
        case TELEM_DSM_ESC_VOLT1:
        case TELEM_DSM_ESC_VOLT2:
        case TELEM_DSM_RXPCAP_VOLT:     _get_value_str(str, value, 2, 'V'); break;
        case TELEM_DSM_JETCAT_THROTTLE: _get_value_str(str, value, 0, '%'); break;
        case TELEM_DSM_JETCAT_TEMPEGT:  _get_temp_str(str, value, 0, 'C'); break;
        case TELEM_DSM_JETCAT_STATUS:   strcpy(str, _dsm_jetcat_status(value)); break;
        case TELEM_DSM_JETCAT_OFFCOND:  strcpy(str, _dsm_jetcat_offcond(value)); break;
        case TELEM_DSM_ESC_AMPS1:
        case TELEM_DSM_RXPCAP_AMPS:     _get_value_str(str, value, 2, 'A'); break;
        case TELEM_DSM_ESC_AMPS2:
        case TELEM_DSM_FPCAP_AMPS:      _get_value_str(str, value, 1, 'A'); break;
        case TELEM_DSM_ESC_TEMP1:
        case TELEM_DSM_ESC_TEMP2:
        case TELEM_DSM_FPCAP_TEMP:      _get_temp_str(str, value, 1, 'C'); break;
        case TELEM_DSM_ESC_THROTTLE:
        case TELEM_DSM_ESC_OUTPUT:      _get_value_str(str, value, 1, '%'); break;
        case TELEM_DSM_RXPCAP_CAPACITY: _get_value_str(str, value, 1, '\0'); break;
#endif
        case TELEM_DSM_VARIO_ALTITUDE:
        case TELEM_DSM_VARIO_CLIMBRATE1:
        case TELEM_DSM_VARIO_CLIMBRATE2:
        case TELEM_DSM_VARIO_CLIMBRATE3:
        case TELEM_DSM_VARIO_CLIMBRATE4:
        case TELEM_DSM_VARIO_CLIMBRATE5:
        case TELEM_DSM_VARIO_CLIMBRATE6:_get_altitude_str(str, value, 1, 'm'); break;
        default:
            return "";
    }
    return str;
}
const char * _dsm_name(char *str, u8 telem)
{
    switch (telem) {
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:      sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_DSM_FLOG_VOLT1 + 1); break;
        case TELEM_DSM_FLOG_TEMP1:      strcpy(str, _tr("Temp")); break;
        case TELEM_DSM_FLOG_RPM1:       strcpy(str, _tr("RPM")); break;
        case TELEM_DSM_FLOG_FADESA:     sprintf(str, "%s%c", _tr("Fades"), 'A'); break;
        case TELEM_DSM_FLOG_FADESB:     sprintf(str, "%s%c", _tr("Fades"), 'B'); break;
        case TELEM_DSM_FLOG_FADESL:     sprintf(str, "%s%c", _tr("Fades"), 'L'); break;
        case TELEM_DSM_FLOG_FADESR:     sprintf(str, "%s%c", _tr("Fades"), 'R'); break;
        case TELEM_DSM_FLOG_FRAMELOSS:  strcpy(str, _tr("Loss")); break;
        case TELEM_DSM_FLOG_HOLDS:      strcpy(str, _tr("Holds")); break;
        case TELEM_DSM_AMPS1:           strcpy(str, _tr("Amps")); break;
        case TELEM_DSM_AIRSPEED:        strcpy(str, _tr("AirSpd")); break;
        case TELEM_DSM_ALTITUDE:        strcpy(str, _tr("Alt.")); break;
        case TELEM_DSM_ALTITUDE_MAX:    strcpy(str, _tr("Alt.max")); break;
        case TELEM_DSM_GFORCE_X:        strcpy(str, "g -> X"); break;
        case TELEM_DSM_GFORCE_Y:        strcpy(str, "g -> Y"); break;
        case TELEM_DSM_GFORCE_Z:        strcpy(str, "g -> Z"); break;
        case TELEM_DSM_GFORCE_XMAX:     strcpy(str, "g max X"); break;
        case TELEM_DSM_GFORCE_YMAX:     strcpy(str, "g max Y"); break;
        case TELEM_DSM_GFORCE_ZMAX:     strcpy(str, "g max Z"); break;
        case TELEM_DSM_GFORCE_ZMIN:     strcpy(str, "g min Z"); break;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_DSM_PBOX_VOLT1:
        case TELEM_DSM_PBOX_VOLT2:      sprintf(str, "%s%d", "Pbox.V", telem - TELEM_DSM_PBOX_VOLT1 + 1); break;
        case TELEM_DSM_PBOX_CAPACITY1:
        case TELEM_DSM_PBOX_CAPACITY2:  sprintf(str, "%s%d", "Pbox.C", telem - TELEM_DSM_PBOX_CAPACITY1 + 1); break;
        case TELEM_DSM_PBOX_ALARMV1:
        case TELEM_DSM_PBOX_ALARMV2:    sprintf(str, "%s%d", "PboxAV", telem - TELEM_DSM_PBOX_ALARMV1 + 1); break;
        case TELEM_DSM_PBOX_ALARMC1:
        case TELEM_DSM_PBOX_ALARMC2:    sprintf(str, "%s%d", "PboxAC", telem - TELEM_DSM_PBOX_ALARMC1 + 1); break;
        case TELEM_DSM_JETCAT_STATUS:   strcpy(str, "Jc.Stat"); break;
        case TELEM_DSM_JETCAT_THROTTLE: strcpy(str, "Jc.THR"); break;
        case TELEM_DSM_JETCAT_PACKVOLT:
        case TELEM_DSM_JETCAT_PUMPVOLT: sprintf(str, "%s%d", "Jc.V", telem - TELEM_DSM_JETCAT_PACKVOLT + 1); break;
        case TELEM_DSM_JETCAT_RPM:      strcpy(str, "Jc.RPM"); break;
        case TELEM_DSM_JETCAT_TEMPEGT:  strcpy(str, "Jc.Temp"); break;
        case TELEM_DSM_JETCAT_OFFCOND:  strcpy(str, "Jc.Off"); break;
        case TELEM_DSM_ESC_AMPS1:
        case TELEM_DSM_ESC_AMPS2:       sprintf(str, "%s%d", "ESC.A", telem - TELEM_DSM_ESC_AMPS1 + 1); break;
        case TELEM_DSM_ESC_VOLT1:
        case TELEM_DSM_ESC_VOLT2:       sprintf(str, "%s%d", "ESC.V", telem - TELEM_DSM_ESC_VOLT1 + 1); break;
        case TELEM_DSM_ESC_TEMP1:
        case TELEM_DSM_ESC_TEMP2:       sprintf(str, "%s%d", "ESC.T", telem - TELEM_DSM_ESC_TEMP1 + 1); break;
        case TELEM_DSM_ESC_RPM:         strcpy(str, "ESC.RPM"); break;
        case TELEM_DSM_ESC_THROTTLE:    strcpy(str, "ESC.THR"); break;
        case TELEM_DSM_ESC_OUTPUT:      strcpy(str, "ESC.PWR"); break;
        case TELEM_DSM_RXPCAP_AMPS:     strcpy(str, "RxCap.A"); break;
        case TELEM_DSM_RXPCAP_CAPACITY: strcpy(str, "RxCap.C"); break;
        case TELEM_DSM_RXPCAP_VOLT:     strcpy(str, "RxCap.V"); break;
        case TELEM_DSM_FPCAP_AMPS:      strcpy(str, "BtCap.A"); break;
        case TELEM_DSM_FPCAP_CAPACITY:  strcpy(str, "BtCap.C"); break;
        case TELEM_DSM_FPCAP_TEMP:      strcpy(str, "BtCap.T"); break;
#endif
        case TELEM_DSM_VARIO_ALTITUDE:  strcpy(str, "Var.Alt"); break;
        case TELEM_DSM_VARIO_CLIMBRATE1:
        case TELEM_DSM_VARIO_CLIMBRATE2:
        case TELEM_DSM_VARIO_CLIMBRATE3:
        case TELEM_DSM_VARIO_CLIMBRATE4:
        case TELEM_DSM_VARIO_CLIMBRATE5:
        case TELEM_DSM_VARIO_CLIMBRATE6:sprintf(str, "%s%d", "Var.CR", telem - TELEM_DSM_VARIO_CLIMBRATE1 + 1); break;
        default:
            return "";
    }
    return str;
}

static const char * _dsm_short_name(char *str, u8 telem)
{
    switch(telem) {
        case 0: strcpy(str, _tr("None")); break;
        case TELEM_DSM_FLOG_VOLT1: strcpy(str, _tr("RxV")); break;
        case TELEM_DSM_FLOG_VOLT2: strcpy(str, _tr("Bat")); break;
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
        case TELEM_DSM_FLOG_HOLDS:      return 999;
        case TELEM_DSM_FLOG_TEMP1:      return 538;
        case TELEM_DSM_FLOG_VOLT1:      return 800;
        case TELEM_DSM_FLOG_VOLT2:      return 6000;
        case TELEM_DSM_FLOG_RPM1:       return 65500;
        case TELEM_DSM_AMPS1:           return 762;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_DSM_PBOX_CAPACITY1:
        case TELEM_DSM_PBOX_CAPACITY2:
        case TELEM_DSM_FPCAP_CAPACITY:
        case TELEM_DSM_RXPCAP_CAPACITY: return 32766;
        case TELEM_DSM_PBOX_ALARMV1:
        case TELEM_DSM_PBOX_ALARMV2:
        case TELEM_DSM_PBOX_ALARMC1:
        case TELEM_DSM_PBOX_ALARMC2:    return 1;
        case TELEM_DSM_JETCAT_THROTTLE: return 159;
        case TELEM_DSM_PBOX_VOLT1:
        case TELEM_DSM_PBOX_VOLT2:      return 1000;
        case TELEM_DSM_RXPCAP_VOLT:     return 1260;
        case TELEM_DSM_JETCAT_PACKVOLT:
        case TELEM_DSM_JETCAT_PUMPVOLT:
        case TELEM_DSM_ESC_VOLT1:       return 6000;
        case TELEM_DSM_ESC_VOLT2:       return 1270;
        case TELEM_DSM_JETCAT_RPM:      return 999999;
        case TELEM_DSM_ESC_RPM:         return 655340;
        case TELEM_DSM_ESC_THROTTLE:
        case TELEM_DSM_ESC_OUTPUT:      return 1270;
        case TELEM_DSM_ESC_AMPS1:       return 15000;
        case TELEM_DSM_ESC_AMPS2:       return 254;
        case TELEM_DSM_ESC_TEMP1:
        case TELEM_DSM_ESC_TEMP2:       return 1500;
        case TELEM_DSM_JETCAT_TEMPEGT:  return 999;
        case TELEM_DSM_RXPCAP_AMPS:     return 1800;
        case TELEM_DSM_FPCAP_AMPS:      return 1400;
        case TELEM_DSM_FPCAP_TEMP:      return 1500;
#endif
        case TELEM_DSM_AIRSPEED:        return 999;
        case TELEM_DSM_ALTITUDE:
        case TELEM_DSM_ALTITUDE_MAX:
        case TELEM_DSM_VARIO_ALTITUDE:  return 65500;
        case TELEM_DSM_VARIO_CLIMBRATE1:return 25;
        case TELEM_DSM_VARIO_CLIMBRATE2:
        case TELEM_DSM_VARIO_CLIMBRATE3:
        case TELEM_DSM_VARIO_CLIMBRATE4:
        case TELEM_DSM_VARIO_CLIMBRATE5:
        case TELEM_DSM_VARIO_CLIMBRATE6:return (telem - TELEM_DSM_VARIO_CLIMBRATE1) * 50;
        case TELEM_DSM_GFORCE_X:
        case TELEM_DSM_GFORCE_Y:
        case TELEM_DSM_GFORCE_Z:
        case TELEM_DSM_GFORCE_XMAX:
        case TELEM_DSM_GFORCE_YMAX:
        case TELEM_DSM_GFORCE_ZMAX:
        case TELEM_DSM_GFORCE_ZMIN:     return 4000;
        default: return 0;  //JETCAT status, offcond -- don't display raw values
    }
}

s32 _dsm_get_min_value(u8 telem)
{           
    switch(telem) {
        case TELEM_DSM_FLOG_TEMP1:      return -40;
        case TELEM_DSM_FLOG_RPM1:       return 200;
        case TELEM_DSM_AMPS1:           return -762;
        case TELEM_DSM_GFORCE_X:
        case TELEM_DSM_GFORCE_Y:
        case TELEM_DSM_GFORCE_Z:
        case TELEM_DSM_GFORCE_XMAX:
        case TELEM_DSM_GFORCE_YMAX:
        case TELEM_DSM_GFORCE_ZMAX:
        case TELEM_DSM_GFORCE_ZMIN:     return -4000;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_DSM_FPCAP_AMPS:      return -1400;
        case TELEM_DSM_RXPCAP_AMPS:     return -1800;
        case TELEM_DSM_FPCAP_CAPACITY:
        case TELEM_DSM_RXPCAP_CAPACITY: return -32766;
#endif
        default: return 0;
    }
}
