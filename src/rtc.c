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
u32 _RTC_GetSerialDate(int year, int month, int day);
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

const char *usmonth[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
const char *germanmonth[] = { "JAN", "FEB", "MRZ", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEZ" };

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
    if (Transmitter.rtcflags & CLOCK12HR) {
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

u32 _RTC_GetSerialDate(int year, int month, int day)
{
    if (year>=RTC_STARTYEAR) year -= RTC_STARTYEAR;
    if (year>67) year = 67;
    if (year<0) year = 0;
    return (u32)(day-1 + daysInYear[year%4 == 0 ? 1 : 0][month-1] + year*365 + year/4 + ((year != 0 && month > 2) ? 1 : 0)) * DAYSEC;
}

// get serial time (seconds since 01.01.RTC_STARTYEAR (now 2012), 0:00:00 - "deviation epoch")    //year = RTC_STARTYEAR-based
u32 RTC_GetSerial(int year, int month, int day, int hour, int minute, int second)
{
    return _RTC_GetSerialTime(hour, minute, second) + _RTC_GetSerialDate(year, month, day);
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

// return formatted time string as stated in tx config
const char *timeformats[] = { /*"default",*/ "hh:mm:ss", "hh:mm:ss am/pm" };
void RTC_GetTimeFormatted(char *str, u32 time)
{
    // which format to use?
    u8 format = Transmitter.rtcflags & TIMEFMT;
    // make sure that the number is correct; use default otherwise
    if (format >= sizeof(timeformats) / sizeof(timeformats[0])) format = 0;
    switch (format) {
        case 0: // "default" = ISO8601 = hh:mm:ss
            sprintf(str, "%2d:%02d:%02d", _RTC_GetHour(time), _RTC_GetMinute(time), _RTC_GetSecond(time));
            break;
        case 1: // a.m. / p.m. = hh:mm:ss am/pm
        {
            u8 am = ((time % DAYSEC) / 3600) < 12;
            sprintf(str, "%2d:%02d:%02d %s", _RTC_GetHour(time), _RTC_GetMinute(time), _RTC_GetSecond(time), (am) ? "AM" : "PM");
            break;
        }
    }
}

// return formatted date string as stated in tx config
const char *dateformats[] = { "YYYY-MM-DD", "MM/DD/YYYY", "DD.MM.YYYY", "Mon DD YYYY", "DD. Mon YYYY", "DD.MM.YY" };
void RTC_GetDateFormatted(char *str, u32 date)
{
    // which format to use?
    u8 format = (Transmitter.rtcflags & DATEFMT) >> 4;
    // make sure that the number is correct; use default otherwise
    if (format >= sizeof(dateformats) / sizeof(dateformats[0])) format = 0;
    _RTC_SetDayStart(date);
    switch (format) {
        case 0: // "default" = ISO8601 = YYYY-MM-DD
            sprintf(str, "%4d-%2d-%2d", today.Year + RTC_STARTYEAR, today.Month + 1, today.Day + 1);
            break;
        case 1: // US = MM/DD/YYYY
            sprintf(str, "%2d/%2d/%4d", today.Month + 1, today.Day + 1, today.Year + RTC_STARTYEAR);
            break;
        case 2: // German = DD.MM.JJJJ
            sprintf(str, "%2d.%2d.%4d", today.Day + 1, today.Month + 1, today.Year + RTC_STARTYEAR);
            break;
        case 3: // long US = Mon DD YYYY
        {
            sprintf(str, "%3s %2d %4d", usmonth[today.Month], today.Day + 1, today.Year + RTC_STARTYEAR);
            break;
        }
        case 4: // long German = DD. Mon YYYY
        {
            sprintf(str, "%2d. %3s %4d", today.Day + 1, germanmonth[today.Month], today.Year + RTC_STARTYEAR);
            break;
        }
        case 5: // German short = DD.MM.JJ
            sprintf(str, "%2d.%2d.%2d", today.Day + 1, today.Month + 1, (today.Year + RTC_STARTYEAR) % 100);
            break;
    }
}

void RTC_GetMonthFormatted(char *str, u8 month)
{
    // which format to use?
    u8 format = (Transmitter.rtcflags & DATEFMT) >> 4;
    // make sure that the number is correct; use default otherwise
    if (format >= sizeof(dateformats) / sizeof(dateformats[0])) format = 0;
    switch (format) {
        case 0: // "default" = ISO8601 = YYYY-MM-DD
        case 1: // US = MM/DD/YYYY
        case 2: // German = DD.MM.JJJJ
        case 5: // German short = DD.MM.JJ
            sprintf(str, "%2d", month);
            break;
        case 3: // long US = Mon DD YYYY
        {
            sprintf(str, "%3s", usmonth[month - 1]);
            break;
        }
        case 4: // long German = DD. Mon YYYY
        {
            sprintf(str, "%3s", germanmonth[month - 1]);
            break;
        }
    }
}

void RTC_GetDateFormattedOrder(u8 index, u8 *left, u8 *middle, u8 *right)
{
    *left = (index == 0) ? YEAR : ((index == 1 || index == 3) ? MONTH : DAY);
    *middle = (index == 1 || index == 3) ? DAY : MONTH;
    *right = index ? YEAR : DAY;
}

// for big boxes only these will fit
void RTC_GetTimeFormattedBigbox(char *str, u32 time)
{
    u8 am = ((time % DAYSEC) / 3600) < 12;
    u8 hour = _RTC_GetHour(time);
    if (hour == 12) hour = 0;
    sprintf(str, "%2d:%02d:%02d", hour + (am ? 0 : 12), _RTC_GetMinute(time), _RTC_GetSecond(time));
}
void RTC_GetDateFormattedBigbox(char *str, u32 date)
{
    _RTC_SetDayStart(date);
    sprintf(str, "%2d.%2d.%2d", today.Day + 1, today.Month + 1, (today.Year + RTC_STARTYEAR) % 100);
}

// to avoid sizeof() in rtc_config
int RTC_GetNumberTimeFormats()
{
    return sizeof(timeformats) / sizeof(timeformats[0]);
}

// to avoid sizeof() in rtc_config
int RTC_GetNumberDateFormats()
{
    return sizeof(dateformats) / sizeof(dateformats[0]);
}

// set time (don't change date)
void RTC_SetTime(u16 hour, u16 minute, u16 second)
{
    RTC_SetValue(RTC_GetDateValue(RTC_GetValue()) * DAYSEC + _RTC_GetSerialTime((int)hour, (int)minute, (int)second));
}

// set date (don't change time)
void RTC_SetDate(u16 year, u16 month, u16 day)
{
    RTC_SetValue(RTC_GetTimeValue(RTC_GetValue()) + _RTC_GetSerialDate((int)year, (int)month, (int)day));
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
