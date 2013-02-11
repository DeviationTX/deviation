#ifndef _TX_H_
#define _TX_H_

#include "mixer.h"
#include "autodimmer.h"
#include "telemetry.h"

#define DEFAULT_BATTERY_WARNING_INTERVAL 30
#define MIN_BATTERY_WARNING_INTERVAL 0
#define MAX_BATTERY_WARNING_INTERVAL 60

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
    u8 brightness;
    u8 contrast;
    u8 telem;
    u8 music_shutdown;
    enum Mode mode;
    u16 batt_alarm;
    u16 batt_critical;
    u16 batt_warning_interval;
    u8 volume;
    u8 vibration_state; // for future vibration on/off support
    struct StickCalibration calibration[INP_HAS_CALIBRATION];
    struct TouchCalibration touch;
    struct AutoDimmer auto_dimmer;
    struct CountDownTimerSettings countdown_timer_settings;
};

extern struct Transmitter Transmitter;
void CONFIG_LoadTx();

#endif
