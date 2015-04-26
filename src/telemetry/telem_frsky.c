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

s32 _frsky_value(struct Telemetry *_t, int idx)
{
    struct telem_frsky *t = &_t->p.frsky;
    switch (idx) {
    case TELEM_FRSKY_VOLT1:
    case TELEM_FRSKY_VOLT2:
    case TELEM_FRSKY_VOLT3:
        return t->volt[idx - TELEM_FRSKY_VOLT1];
    case TELEM_FRSKY_TEMP1:
    case TELEM_FRSKY_TEMP2:
        return t->temp[idx - TELEM_FRSKY_TEMP1];
    case TELEM_FRSKY_RPM:
        return t->rpm;
    }
    return 0;
}

const char * _frsky_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2:
        case TELEM_FRSKY_VOLT3: _get_volt_str(str, value); break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2: _get_temp_str(str, value); break;
        case TELEM_FRSKY_RPM:   sprintf(str, "%d", (int)value);
            break;
    }
    return str;
}

const char * _frsky_name(char *str, u8 telem)
{
    switch (telem) {
      case TELEM_FRSKY_VOLT1:
      case TELEM_FRSKY_VOLT2:
        sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_FRSKY_VOLT1 + 1);
        break;
      case TELEM_FRSKY_TEMP1:
      case TELEM_FRSKY_TEMP2:
        sprintf(str, "%s%d", _tr("TelemT"), telem - TELEM_FRSKY_TEMP1 + 1);
        break;
      case TELEM_FRSKY_RPM:
        sprintf(str, "%s", _tr("TelemRPM"));
        break;
      default:
        return "";
    }
    return str;
}

const char * _frsky_short_name(char *str, u8 telem)
{
    switch(telem) {
        case 0: strcpy(str, _tr("None")); break;
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2:
        case TELEM_FRSKY_VOLT3: sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_DEVO_VOLT1 + 1); break;
        case TELEM_FRSKY_RPM:  sprintf(str, "%s%d",  _tr("RPM"), telem - TELEM_DEVO_RPM1 + 1);  break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2: sprintf(str, "%s%d", _tr("Temp"), telem - TELEM_DEVO_TEMP1 + 1); break;
        default: str[0] = '\0'; break;
    }
    return str;
}

s32 _frsky_get_max_value(u8 telem)
{
    switch(telem) {
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2:
        case TELEM_FRSKY_VOLT3:
            return 255;
        case TELEM_FRSKY_RPM:
            return 20000;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:
            return 255;
        default:
            return 0;
    }
}
