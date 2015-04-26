#ifndef _DATALOG_H_
#define _DATALOG_H_

enum {
    DLOG_TIMERS    = 0,
    DLOG_TELEMETRY = NUM_TIMERS,
    DLOG_INPUTS    = NUM_TIMERS + NUM_TELEM,
    DLOG_CHANNELS  = NUM_TIMERS + NUM_TELEM + NUM_INPUTS,
    DLOG_PPM       = NUM_TIMERS + NUM_TELEM + NUM_INPUTS + NUM_CHANNELS,
    DLOG_GPSLOC    = NUM_TIMERS + NUM_TELEM + NUM_SOURCES,
    DLOG_GPSALT,
    DLOG_GPSSPEED,
    DLOG_GPSTIME,
#if HAS_RTC
    DLOG_TIME,
#endif
    DLOG_LAST,
};

enum {
    DLOG_RATE_1SEC,
    DLOG_RATE_5SEC,
    DLOG_RATE_10SEC,
    DLOG_RATE_30SEC,
    DLOG_RATE_1MIN,
    DLOG_RATE_LAST,
};

#define GPSLOC_SIZE 8
#define GPSTIME_SIZE 4
#define CLOCK_SIZE   4
#define TIMER_SIZE   2

#define NUM_DATALOG DLOG_LAST
#define DATALOG_BYTE(x) ((x) / 8)
#define DATALOG_POS(x) ((x) % 8)

struct datalog {
    u8 enable;
    u8 rate;
    u8 source[(7 + NUM_DATALOG) / 8];
};

extern void DATALOG_Init();
extern void DATALOG_Update();
extern const char *DATALOG_Source(char *str, int idx);
extern int DATALOG_Remaining();
extern void DATALOG_Reset();
extern void DATALOG_UpdateState();
extern int DATALOG_IsEnabled();
extern const char *DATALOG_RateString(int idx);
extern void DATALOG_ApplyMask(int idx, int set);
#endif
