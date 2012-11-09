#ifndef _AUTODIMMER_H_
#define _AUTODIMMER_H_
#include "timer.h"

struct AutoDimmer {
    u16 timer;
    u8 backlight_dim_value;
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
