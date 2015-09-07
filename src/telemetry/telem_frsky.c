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

s32 _frsky_value(struct Telemetry *t, int idx)
{
    switch (idx) {
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:
            return (s16)t->value[idx];
        case TELEM_FRSKY_ALTITUDE:
            // Multiply by 100 because of decimal 2 in _frsky_str_by_value
            return 100 * ((s32) t->value[idx])
                   + t->value[TELEM_FRSKY_ALTITUDE_DECIMETERS];
        default:
            return t->value[idx];
    }
    return 0;
}

const char * _frsky_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
        case TELEM_FRSKY_MIN_CELL:
        case TELEM_FRSKY_CELL1:
        case TELEM_FRSKY_CELL2:
        case TELEM_FRSKY_CELL3:
        case TELEM_FRSKY_CELL4:
        case TELEM_FRSKY_CELL5:
        case TELEM_FRSKY_CELL6:
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2:
        case TELEM_FRSKY_VOLT3: _get_value_str(str, value, 2, 'V'); break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2: _get_temp_str(str, value, 0, 'C'); break;
        case TELEM_FRSKY_RSSI:  _get_value_str(str, value, 0, 'D'); break;
        case TELEM_FRSKY_RPM:   _get_value_str(str, value, 0, 'R'); break;
        case TELEM_FRSKY_ALTITUDE:
            // The decimal value of 2 here means we multiplyl
            // t->value[TELEM_FRSKY_ALTITUDE] by 100 in _frsky_value
            _get_altitude_str(str, value, 2, 'm');
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
        case TELEM_FRSKY_VOLT3: sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_FRSKY_VOLT1 + 1); break;
        case TELEM_FRSKY_MIN_CELL: strcpy(str, _tr("MinCell")); break;
        case TELEM_FRSKY_CELL1:
        case TELEM_FRSKY_CELL2:
        case TELEM_FRSKY_CELL3:
        case TELEM_FRSKY_CELL4:
        case TELEM_FRSKY_CELL5:
        case TELEM_FRSKY_CELL6: sprintf(str, "%s%d", _tr("Cell"), telem - TELEM_FRSKY_CELL1 + 1); break;
        case TELEM_FRSKY_RSSI:  strcpy(str, _tr("RSSI")); break;
        case TELEM_FRSKY_RPM:   strcpy(str, _tr("RPM")); break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2: sprintf(str, "%s%d", _tr("Temp"), telem - TELEM_FRSKY_TEMP1 + 1); break;
        case TELEM_FRSKY_ALTITUDE: strcpy(str, _tr("Altitude")); break;
        default: sprintf(str, "FrST%d", telem); break;
    }
    return str;
}

const char * _frsky_name(char *str, u8 telem)
{
    switch (telem) {
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2:
        case TELEM_FRSKY_VOLT3:
            sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_FRSKY_VOLT1 + 1);
            break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:
            sprintf(str, "%s%d", _tr("TelemT"), telem - TELEM_FRSKY_TEMP1 + 1);
            break;
        case TELEM_FRSKY_RSSI:
            strcpy(str, _tr("TelemRSSI"));
            break;
        case TELEM_FRSKY_RPM:
            strcpy(str, _tr("TelemRPM"));
            break;
        default:
            _frsky_short_name(str, telem);
            break;
    }
    return str;
}

s32 _frsky_get_max_value(u8 telem)
{
    switch(telem) {
        case TELEM_FRSKY_VOLT1:     return 1326; // All voltages are x100
        case TELEM_FRSKY_VOLT2:     return 8538; //should be 33 * AD2gain, but ugh
        case TELEM_FRSKY_VOLT3:     return 819 * 6;
        case TELEM_FRSKY_MIN_CELL:  return 819 ;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:     return 250;
        case TELEM_FRSKY_RSSI:
        case TELEM_FRSKY_RPM:       return 60000;
        case TELEM_FRSKY_ALTITUDE:  return 900000; //x100
        default:
            return 0;
    }
}

s32 _frsky_get_min_value(u8 telem)
{
    switch(telem) {
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:     return -30;
        case TELEM_FRSKY_RPM:       return 60;
        case TELEM_FRSKY_ALTITUDE:  return -50000; //x100
        default:
            return 0;
    }
}
