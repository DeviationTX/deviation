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
#endif
