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

void TELEMETRY_SetString(char *str, s32 value)
{
    sprintf(str, "%d.%dV", (int)value /10, (int)value % 10);
}
