#ifndef _TIMER_H_
#define _TIMER_H_

#ifndef HAS_RTC
    #define HAS_RTC 0
    #define NUM_TIMERS 5
#else
    #define NUM_TIMERS 4
#endif

#define TIMER_MAX_VAL 5940
#define IS_RTC(x) ((x) == NUM_TIMERS - 1)

#define DEFAULT_TIMEUP_INTERVAL 10000
#define DEFAULT_PERALERT_TIME 30000
#define DEFAULT_PREALERT_INTERVAL 10000 // 10seconds
#define MIN_PERALERT_TIME 0
#define MAX_PERALERT_TIME 60

enum TimerType {
    TIMER_STOPWATCH,
    TIMER_STOPWATCH_PROP,
    TIMER_COUNTDOWN,
    TIMER_COUNTDOWN_PROP,
    TIMER_PERMANENT,
    TIMER_LAST,
};
struct Timer {
    u8 src;
    u8 resetsrc;
    u16 timer;
    u32 val;
    enum TimerType type;
};

struct CountDownTimerSettings {
    u32 prealert_time;
    u16 prealert_interval;
    u16 timeup_interval;
};

void TIMER_SetString(char *str, s32 time);
const char *TIMER_Name(char *str, u8 timer);
void TIMER_StartStop(u8 timer);
void TIMER_Reset(u8 timer);
s32 TIMER_GetValue(u8 timer);
void TIMER_Update();
void TIMER_Init();
#endif
