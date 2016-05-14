#ifndef _AUTODIMMER_H_
#define _AUTODIMMER_H_
#include "timer.h"

// AutoDimmer is part of struct Transmitter and must beword aligned
struct AutoDimmer {
    u32 timer;  //bug fix: overflow when timer > 1minutes
    u8 backlight_dim_value;
    u8 padding_1[3];
};

typedef enum {
    AUTODIMMER_STOP = 0,
    AUTODIMMER_START,
} AutoDimmerState;

#define DEFAULT_BACKLIGHT_DIMTIME 20000 // Make backlight dimmer in 20s
#define DEFAULT_BACKLIGHT_DIMVALUE 0 //
#define MIN_BACKLIGHT_DIMTIME 0 // shorter than 10seconds is too frequent
#define MAX_BACKLIGHT_DIMTIME 120

void AUTODIMMER_Update();
void AUTODIMMER_Check();
void AUTODIMMER_Init();
#endif
