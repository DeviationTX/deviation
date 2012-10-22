#ifndef _TIMER_H_
#define _TIMER_H_

#define TIMER_MAX_VAL 5940
#define NUM_TIMERS 2
enum TimerType {
    TIMER_STOPWATCH,
    TIMER_COUNTDOWN,
    TIMER_LAST,
};
struct Timer {
    u8 src;
    u16 timer;
    enum TimerType type;
};

void TIMER_SetString(char *str, s32 time);
const char *TIMER_Name(char *str, u8 timer);
void TIMER_StartStop(u8 timer);
void TIMER_Reset(u8 timer);
s32 TIMER_GetValue(u8 timer);
void TIMER_Update();
void TIMER_Init();
#endif
