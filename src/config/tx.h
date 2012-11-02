#ifndef _TX_H_
#define _TX_H_

#include "mixer.h"

struct StickCalibration {
    u16 max;
    u16 min;
    u16 zero;
};

struct TouchCalibration {
    s32 xscale;
    s32 yscale;
    s32 xoffset;
    s32 yoffset;
};

struct Transmitter {
    u8 current_model;
    u8 language;
    enum Mode mode;
    u8 brightness;
    u8 contrast;
    u16 batt_alarm;
    u16 batt_critical;
    struct StickCalibration calibration[INP_HAS_CALIBRATION];
    struct TouchCalibration touch;
};

extern struct Transmitter Transmitter;
void CONFIG_LoadTx();

#endif
