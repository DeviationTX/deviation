#ifndef _RTC_H_
#define _RTC_H_

#if HAS_RTC
    #define NUM_RTC 2
#else
    #define NUM_RTC 0
#endif

#define RTC_STARTYEAR 2012

struct gtm
{
  int8_t tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
  int8_t tm_min;                   /* Minutes.     [0-59] */
  int8_t tm_hour;                  /* Hours.       [0-23] */
  int8_t tm_mday;                  /* Day.         [1-31] */
  int8_t tm_mon;                   /* Month.       [0-11] */
  int8_t tm_year;                  /* Year - 1900. Limited to the year 2115. Oh no! :P */
  // int8_t tm_wday;                  /* Day of week. [0-6] */
  // int16_t tm_yday;                 /* Day of year. [0-365] Needed internally for calculations */
};

enum { DAY=0, MONTH, YEAR, SECOND, MINUTE, HOUR };

// initialize RTC clock
void RTC_Init();

// set date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
void RTC_SetValue(u32 value);

// set time/date (don't change date/time)
void RTC_SetTime(u16 hour, u16 minute, u16 second);
void RTC_SetDate(u16 year, u16 month, u16 day);

// get date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
u32 RTC_GetValue();
void RTC_GetTimeGTM(struct gtm *t);

// get serial time (seconds since 01.01.2012, 0:00:00 - "deviation epoch")
u32 RTC_GetSerial(int year, int month, int day, int hour, int minute, int second);

// format time string
void RTC_GetTimeString(char *str, u32 value);
void RTC_GetTimeStringShort(char *str, u32 value);  // no seconds
void RTC_GetDateString(char *str, u32 date);        // without century
void RTC_GetDateStringLong(char *str, u32 date);    // with century

// return only time/date-value
u32 RTC_GetTimeValue(u32 time);
u32 RTC_GetDateValue(u32 time);

// to avoid sizeof() in rtc_config
int RTC_GetNumberTimeFormats();
int RTC_GetNumberDateFormats();

// return formatted time/date string as stated in tx config
void RTC_GetTimeFormatted(char *str, u32 time);
void RTC_GetDateFormatted(char *str, u32 date);
void RTC_GetMonthFormatted(char *str, unsigned month); // for written month names
void RTC_GetTimeFormattedBigbox(char *str, u32 time); // only this fits in big box
void RTC_GetDateFormattedBigbox(char *str, u32 date); // only this fits in big box
void RTC_GetDateFormattedOrder(unsigned index, u8 *left, u8 *middle, u8 *right); // for ordering the input fields

//return RTC name
const char *RTC_Name(int i);
#endif
