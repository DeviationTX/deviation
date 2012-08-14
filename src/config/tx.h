#ifndef _TX_H_
#define _TX_H

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
    enum Mode mode;
    struct StickCalibration calibration[NUM_INPUTS];
    struct TouchCalibration touch;
};

extern struct Transmitter Transmitter;
void CONFIG_LoadTx();
void CONFIG_SaveTxIfNeeded();

#endif
