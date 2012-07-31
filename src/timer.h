#ifndef _TIMER_H_
#define _TIMER_H_

#define TIMER_MAX_VAL 600
#define NUM_TIMERS 2
enum TimerType {
    TIMER_STOPWATCH,
    TIMER_COUNTDOWN,
};
struct Timer {
    u8 src;
    u16 timer;
    enum TimerType type;
};

void TIMER_SetString(char *str, s16 time);
void TIMER_StartStop(u8 timer);
void TIMER_Reset(u8 timer);
s16 TIMER_GetValue(u8 timer);
void TIMER_Update();
void TIMER_Init();
#endif
