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
#include <time.h>

long delta;
// initialize RTC
void RTC_Init()
{
   delta = 0;
}

// set date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
void RTC_SetValue(u32 value)
{
    delta = time(NULL) - value;
    //_RTC_SetDayStart(value);
}

// get date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
u32 RTC_GetValue()
{
    return time(NULL) - delta;
}

