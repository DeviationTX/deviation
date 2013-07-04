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
#include "target_defs.h"
#include "config/tx.h"
#include "rtc.h"
#if HAS_RTC
#include <stdlib.h>

/* define deviation epoch as the number of seconds since 1.1.2012, 00:00:00
   2012: year of birth of deviation, also the first leap year for deviation lifetime
         and linux epoch will overflow in 2038 while we will work until 2080 */

// maybe useful if we get the real time via telemetry / gps
// #define BEGINNING2012 1325376000    // for recalculation with linux epoch, seconds since 1970 in UTC

void _RTC_SetDayStart(u32 value);
int _RTC_GetSecond(u32 value);
int _RTC_GetMinute(u32 value);
int _RTC_GetHour(u32 value);
int _RTC_GetDay(u32 value);
int _RTC_GetMonth(u32 value);
int _RTC_GetYear(u32 value);
u32 _RTC_GetSerialTime(int hour, int minute, int second);
u32 _RTC_GetSerialDate(int day, int month, int year);
void _RTC_GetDateStringHelper(char *str, u32 date, u8 year4);


static struct {
    u32 DayStart;
    int Day;
    int Month;
    int Year;
} today; 
static const u16 daysInYear[2][13] = { { 0,31,59,90,120,151,181,212,243,273,304,334,365},
                                        { 0,31,60,91,121,152,182,213,244,274,305,335,366} };
#define DAYSEC (60*60*24)
    
// store actual serial number for dayStart for less often calculating the date values
void _RTC_SetDayStart(u32 value)
{
    u32 days = value / DAYSEC;
    if (today.DayStart != days) {
        today.DayStart = days;
        today.Year  = (4*days) / 1461; // = days/365.25
        u8 leap = today.Year % 4 == 0;
        days -= (u32)(today.Year * 365 + today.Year / 4);
        days -= ((today.Year != 0 && days > daysInYear[leap][2]) ? 1 : 0);    //leap year correction for RTC_STARTYEAR
        today.Month = 0;
        for (today.Month=0; today.Month<12; today.Month++) {
            if (days < daysInYear[leap][today.Month + 1]) break;
        }
        today.Day = days - daysInYear[leap][today.Month];
    }
}

int _RTC_GetSecond(u32 value)
{
    return (int)(value % 60);
}

int _RTC_GetMinute(u32 value)
{
    return (int)(value / 60) % 60;
}

int _RTC_GetHour(u32 value)
{
    value /= 3600;
    if (Transmitter.clock12hr) {
       value %= 12;
       if (value == 0)
           value = 12;
    } else {
        value %= 24;
    }
    return value;
}

int _RTC_GetDay(u32 value)
{
    _RTC_SetDayStart(value);
    return today.Day+1;
}

int _RTC_GetMonth(u32 value)
{
    _RTC_SetDayStart(value);
    return today.Month+1;
}

int _RTC_GetYear(u32 value)
{
    _RTC_SetDayStart(value);
    return today.Year + RTC_STARTYEAR;
}

u32 _RTC_GetSerialTime(int hour, int minute, int second)
{
    return (u32)(hour * 3600 + minute * 60 + second);
}

u32 _RTC_GetSerialDate(int day, int month, int year)
{
    if (year>RTC_STARTYEAR) year -= RTC_STARTYEAR;
    if (year>67) year = 67;
    if (year<0) year = 0;
    return (u32)(day-1 + daysInYear[year%4 == 0 ? 1 : 0][month-1] + year*365 + year/4 + ((year != 0 && month > 2) ? 1 : 0)) * DAYSEC;
}

// get serial time (seconds since 01.01.RTC_STARTYEAR (now 2012), 0:00:00 - "deviation epoch")    //year = RTC_STARTYEAR-based
u32 RTC_GetSerial(int day, int month, int year, int hour, int minute, int second)
{
    return _RTC_GetSerialTime(hour, minute, second) + _RTC_GetSerialDate(day, month, year);
}

// format time string
void RTC_GetTimeString(char *str, u32 value)
{
    sprintf(str, "%2d:%02d:%02d", _RTC_GetHour(value), _RTC_GetMinute(value), _RTC_GetSecond(value));
}

// format time string
void RTC_GetTimeStringShort(char *str, u32 value)
{
    sprintf(str, "%2d:%02d", _RTC_GetHour(value), _RTC_GetMinute(value));
}

// format date string
void _RTC_GetDateStringHelper(char *str, u32 date, u8 year4)
{
    _RTC_SetDayStart(date);
    if (year4) sprintf(str, "%2d.%02d.%04d", today.Day+1, today.Month+1, today.Year+RTC_STARTYEAR);
    else       sprintf(str, "%2d.%02d.%02d", today.Day+1, today.Month+1, (today.Year+RTC_STARTYEAR)%100);
}

// format date string (return year without century)
void RTC_GetDateString(char *str, u32 date)
{
    _RTC_GetDateStringHelper(str, date, 0);
}

// format date string (return year with century)
void RTC_GetDateStringLong(char *str, u32 date)
{
    _RTC_GetDateStringHelper(str, date, 1);
}

// return only time-value
u32 RTC_GetTimeValue(u32 time)
{
    return time % DAYSEC;
}

// return only date-value
u32 RTC_GetDateValue(u32 time)
{
    return time / DAYSEC;
}

const char *RTC_Name(char *str, int i)
{
    sprintf(str, "%s", i == 0 ? _tr("Clock") : _tr("Date"));
    return str;
}
#endif //HAS_RTC
