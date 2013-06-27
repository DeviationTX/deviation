#ifndef _RTC_H_
#define _RTC_H_

#ifndef HAS_RTC
    #define HAS_RTC 0
    #define NUM_RTC 0
#else
    #define NUM_RTC 2
#endif

#define RTC_STARTYEAR 2012

// initialize RTC clock
void RTC_Init();

// set date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
void RTC_SetValue(u32 value);

// get date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
u32 RTC_GetValue();

// get serial time (seconds since 01.01.2012, 0:00:00 - "deviation epoch")
u32 RTC_GetSerial(int day, int month, int year, int hour, int minute, int second);

// format time string
void RTC_GetTimeString(char *str, u32 value);
void RTC_GetTimeStringShort(char *str, u32 value);

// format date string (return year without century)
void RTC_GetDateString(char *str, u32 date);

// format date string (return year with century)
void RTC_GetDateStringLong(char *str, u32 date);

// return only time-value
u32 RTC_GetTimeValue(u32 time);

// return only date-value
u32 RTC_GetDateValue(u32 time);

//return RTC name
const char *RTC_Name(char *str, int i);
#endif
