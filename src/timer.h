#ifndef _TIMER_H_
#define _TIMER_H_

#define NUM_TIMERS 4

#define TIMER_MAX_VAL 5940

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
    u8 padding_1[3];  //Needed to ensure structure is word aligned
};

struct CountDownTimerSettings {
    u32 prealert_time;
    u16 prealert_interval;
    u16 timeup_interval;
};

void TIMER_SetString(char *str, s32 time);
const char *TIMER_Name(char *str, unsigned timer);
void TIMER_StartStop(unsigned timer);
void TIMER_Reset(unsigned timer);
s32 TIMER_GetValue(unsigned timer);
void TIMER_SetValue(unsigned timer, s32 value);
void TIMER_Update();
void TIMER_Init();
#endif
