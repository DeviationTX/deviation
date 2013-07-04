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

s32 _devo_value(struct Telemetry *_t, int idx)
{
    struct telem_devo *t = &_t->p.devo;
    switch (idx) {
    case TELEM_DEVO_VOLT1:
    case TELEM_DEVO_VOLT2:
    case TELEM_DEVO_VOLT3:
        return t->volt[idx - TELEM_DEVO_VOLT1];
    case TELEM_DEVO_TEMP1:
    case TELEM_DEVO_TEMP2:
    case TELEM_DEVO_TEMP3:
    case TELEM_DEVO_TEMP4:
        return t->temp[idx - TELEM_DEVO_TEMP1];
    case TELEM_DEVO_RPM1:
    case TELEM_DEVO_RPM2:
        return t->rpm[idx - TELEM_DEVO_RPM1];
    }
    return 0;
}

const char * _devo_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
        case TELEM_DEVO_VOLT1:
        case TELEM_DEVO_VOLT2:
        case TELEM_DEVO_VOLT3: _get_volt_str(str, value); break;
        case TELEM_DEVO_TEMP1:
        case TELEM_DEVO_TEMP2:
        case TELEM_DEVO_TEMP3:
        case TELEM_DEVO_TEMP4: _get_temp_str(str, value); break;
        case TELEM_DEVO_RPM1:
        case TELEM_DEVO_RPM2:  sprintf(str, "%d", (int)value);
            break;
    }
    return str;
}

const char * _devo_name(char *str, u8 telem)
{
    switch (telem) {
      case TELEM_DEVO_VOLT1:
      case TELEM_DEVO_VOLT2:
      case TELEM_DEVO_VOLT3:
        sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_DEVO_VOLT1 + 1);
        break;
      case TELEM_DEVO_TEMP1:
      case TELEM_DEVO_TEMP2:
      case TELEM_DEVO_TEMP3:
      case TELEM_DEVO_TEMP4:
        sprintf(str, "%s%d", _tr("TelemT"), telem - TELEM_DEVO_TEMP1 + 1);
        break;
      case TELEM_DEVO_RPM1:
      case TELEM_DEVO_RPM2:
        sprintf(str, "%s%d", _tr("TelemRPM"), telem - TELEM_DEVO_RPM1 + 1);
        break;
      default:
        return "";
    }
    return str;
}

const char * _devo_short_name(char *str, u8 telem)
{
    switch(telem) {
        case 0: strcpy(str, _tr("None")); break;
        case TELEM_DEVO_VOLT1:
        case TELEM_DEVO_VOLT2:
        case TELEM_DEVO_VOLT3: sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_DEVO_VOLT1 + 1); break;
        case TELEM_DEVO_RPM1:
        case TELEM_DEVO_RPM2:  sprintf(str, "%s%d",  _tr("RPM"), telem - TELEM_DEVO_RPM1 + 1);  break;
        case TELEM_DEVO_TEMP1:
        case TELEM_DEVO_TEMP2:
        case TELEM_DEVO_TEMP3:
        case TELEM_DEVO_TEMP4: sprintf(str, "%s%d", _tr("Temp"), telem - TELEM_DEVO_TEMP1 + 1); break;
        default: str[0] = '\0'; break;
    }
    return str;
}

s32 _devo_get_max_value(u8 telem)
{
    switch(telem) {
        case TELEM_DEVO_VOLT1:
        case TELEM_DEVO_VOLT2:
        case TELEM_DEVO_VOLT3:
            return 255;
        case TELEM_DEVO_RPM1:
        case TELEM_DEVO_RPM2:
            return 20000;
        case TELEM_DEVO_TEMP1:
        case TELEM_DEVO_TEMP2:
        case TELEM_DEVO_TEMP3:
        case TELEM_DEVO_TEMP4:
            return 255;
        default:
            return 0;
    }
}
