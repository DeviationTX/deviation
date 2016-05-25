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
    return t->value[idx];
}

const char * _frsky_str_by_value(char *str, u8 telem, s32 value)
{
    switch(telem) {
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2: _get_value_str(str, value, 2, 'V'); break;
        case TELEM_FRSKY_RSSI:  _get_value_str(str, value, 0, 'd'); break;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_FRSKY_VOLT3:
        case TELEM_FRSKY_CELL1:
        case TELEM_FRSKY_CELL2:
        case TELEM_FRSKY_CELL3:
        case TELEM_FRSKY_CELL4:
        case TELEM_FRSKY_CELL5:
        case TELEM_FRSKY_CELL6:
        case TELEM_FRSKY_MIN_CELL:
        case TELEM_FRSKY_ALL_CELL:
        case TELEM_FRSKY_VOLTA: _get_value_str(str, value, 2, 'V'); break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2: _get_temp_str(str, value, 0, 'C'); break;
        case TELEM_FRSKY_RPM:   _get_value_str(str, value, 0, 'R'); break;
        case TELEM_FRSKY_FUEL: _get_value_str(str, value, 0, '%'); break;
        case TELEM_FRSKY_CURRENT: _get_value_str(str, value, 1, 'A'); break;
        case TELEM_FRSKY_ALTITUDE:
        case TELEM_FRSKY_VARIO: _get_altitude_str(str, value, 2, 'm'); break;
        case TELEM_FRSKY_DISCHARGE: _get_value_str(str, value, 0, 'D'); break;
#endif
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
        case TELEM_FRSKY_VOLT2: sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_FRSKY_VOLT1 + 1); break;
        case TELEM_FRSKY_RSSI:  strcpy(str, _tr("RSSI")); break;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_FRSKY_VOLT3: sprintf(str, "%s%d", _tr("Volt"), telem - TELEM_FRSKY_VOLT1 + 1); break;
        case TELEM_FRSKY_VOLTA: sprintf(str, "%s%c", _tr("Volt"), (_tr("Amps"))[0]); break;
        case TELEM_FRSKY_MIN_CELL: strcpy(str, _tr("MinCell")); break;
        case TELEM_FRSKY_ALL_CELL: strcpy(str, _tr("AllCell")); break;
        case TELEM_FRSKY_CELL1:
        case TELEM_FRSKY_CELL2:
        case TELEM_FRSKY_CELL3:
        case TELEM_FRSKY_CELL4:
        case TELEM_FRSKY_CELL5:
        case TELEM_FRSKY_CELL6: sprintf(str, "%s%d", _tr("Cell"), telem - TELEM_FRSKY_CELL1 + 1); break;
        case TELEM_FRSKY_RPM:   strcpy(str, _tr("RPM")); break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2: sprintf(str, "%s%d", _tr("Temp"), telem - TELEM_FRSKY_TEMP1 + 1); break;
        case TELEM_FRSKY_FUEL: strcpy(str, _tr("Fuel")); break;
        case TELEM_FRSKY_CURRENT: strcpy(str, _tr("Amps")); break;
        case TELEM_FRSKY_ALTITUDE: strcpy(str, _tr("Alt")); break;
        case TELEM_FRSKY_VARIO: strcpy(str, _tr("VSI")); break;
        case TELEM_FRSKY_DISCHARGE: strcpy(str, _tr("DIS")); break;
#endif
        default: sprintf(str, "FrST%d", telem); break;
    }
    return str;
}

const char * _frsky_name(char *str, u8 telem)
{
    switch (telem) {
        case TELEM_FRSKY_VOLT1:
        case TELEM_FRSKY_VOLT2:
#if HAS_EXTENDED_TELEMETRY
        case TELEM_FRSKY_VOLT3:
#endif
            sprintf(str, "%s%d", _tr("TelemV"), telem - TELEM_FRSKY_VOLT1 + 1);
            break;
        case TELEM_FRSKY_RSSI:
            strcpy(str, _tr("TelemRSSI"));
            break;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_FRSKY_VOLTA:
            sprintf(str, "%s%s", _tr("TelemV"), _tr("Amp"));
            break;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:
            sprintf(str, "%s%d", _tr("TelemT"), telem - TELEM_FRSKY_TEMP1 + 1);
            break;
        case TELEM_FRSKY_RPM:
            strcpy(str, _tr("TelemRPM"));
            break;
#endif
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
        case TELEM_FRSKY_RSSI:      return 60000;
#if HAS_EXTENDED_TELEMETRY
        case TELEM_FRSKY_RPM:       return 60000;
        case TELEM_FRSKY_VOLT3:     return 500 * 6;
        case TELEM_FRSKY_MIN_CELL:  return 819 ;
        case TELEM_FRSKY_ALL_CELL:  return 500 * 6;   // in 100ths of volts
        case TELEM_FRSKY_VOLTA:     return 4800;
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:     return 250;
        case TELEM_FRSKY_FUEL:      return 100;
        case TELEM_FRSKY_CURRENT:   return 1000;
        case TELEM_FRSKY_ALTITUDE:  return 900000; // cm
        case TELEM_FRSKY_VARIO:     return 500000; // cm
        case TELEM_FRSKY_DISCHARGE: return 500000;
#endif
        default:
            return 0;
    }
}

s32 _frsky_get_min_value(u8 telem)
{
    switch(telem) {
#if HAS_EXTENDED_TELEMETRY
        case TELEM_FRSKY_TEMP1:
        case TELEM_FRSKY_TEMP2:     return -30;
        case TELEM_FRSKY_RPM:       return 60;
        case TELEM_FRSKY_ALTITUDE:  return -50000; // cm
        case TELEM_FRSKY_VARIO:     return -500000; // cm
#endif
        default:
            return 0;
    }
}
