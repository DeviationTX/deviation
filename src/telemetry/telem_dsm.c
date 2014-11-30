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

s32 _dsm_value(struct Telemetry *_t, int idx)
{
    struct telem_dsm *t = &_t->p.dsm;
    switch (idx) {
    case TELEM_DSM_FLOG_FADESA:
    case TELEM_DSM_FLOG_FADESB:
    case TELEM_DSM_FLOG_FADESL:
    case TELEM_DSM_FLOG_FADESR:    return t->flog.fades[idx - TELEM_DSM_FLOG_FADESA];
    case TELEM_DSM_FLOG_FRAMELOSS: return t->flog.frameloss;
    case TELEM_DSM_FLOG_HOLDS:     return t->flog.holds;
    case TELEM_DSM_FLOG_VOLT1:
    case TELEM_DSM_FLOG_VOLT2:     return t->flog.volt[idx - TELEM_DSM_FLOG_VOLT1];
    case TELEM_DSM_FLOG_RPM1:      return t->flog.rpm;
    case TELEM_DSM_FLOG_TEMP1:     return t->flog.temp;
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
    case TELEM_DSM_FLOG_RPM1:      sprintf(str, "%d", (int)value); break;
    case TELEM_DSM_FLOG_VOLT1:
    case TELEM_DSM_FLOG_VOLT2:     _get_volt_str(str, value); break;
    case TELEM_DSM_FLOG_TEMP1:     _get_temp_str(str, value); break;
    default:  sprintf(str, "Unknown");
    }
    return str;
}
const char * _dsm_name(char *str, u8 telem)
{
    switch (telem) {
      case TELEM_DSM_FLOG_VOLT1: 
      case TELEM_DSM_FLOG_VOLT2:  
        sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_DSM_FLOG_VOLT1 + 1);
        break;
      case TELEM_DSM_FLOG_TEMP1:
        strcpy(str, _tr("TelemT"));
        break;
      case TELEM_DSM_FLOG_RPM1:
        strcpy(str, _tr("TelemRPM"));
        break;
      case TELEM_DSM_FLOG_FADESA: sprintf(str, "%s%c", _tr("Fades"), 'A'); break;
      case TELEM_DSM_FLOG_FADESB: sprintf(str, "%s%c", _tr("Fades"), 'B'); break;
      case TELEM_DSM_FLOG_FADESL: sprintf(str, "%s%c", _tr("Fades"), 'L'); break;
      case TELEM_DSM_FLOG_FADESR: sprintf(str, "%s%c", _tr("Fades"), 'R'); break;
      case TELEM_DSM_FLOG_FRAMELOSS: strcpy(str, _tr("Loss")); break;
      case TELEM_DSM_FLOG_HOLDS: strcpy(str, _tr("Holds")); break;
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
        case TELEM_DSM_FLOG_HOLDS:
        case TELEM_DSM_FLOG_TEMP1:
            return 9999;
        case TELEM_DSM_FLOG_VOLT1:
        case TELEM_DSM_FLOG_VOLT2:
            return 999;
        case TELEM_DSM_FLOG_RPM1:
            return 20000;
        default: return 0;
    }
}
