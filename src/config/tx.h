#ifndef _TX_H_
#define _TX_H_

#include "mixer.h"
#include "autodimmer.h"
#include "telemetry.h"

#define DEFAULT_BATTERY_WARNING_INTERVAL 30
#define MIN_BATTERY_WARNING_INTERVAL 0
#define MAX_BATTERY_WARNING_INTERVAL 60
#define DEFAULT_SPLASH_DELAY 35 //3.5sec

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

struct mcu_pin {
    u32 port;
    u16 pin;
};

// bitmap for rtcflags:
#define CLOCK12HR 0x01  //0b00000001
#define TIMEFMT   0x0F  //0b00001111
#define DATEFMT   0xF0  //0b11110000
struct Transmitter {
    u8 current_model;
    u8 language;
    u8 brightness;
    u8 contrast;
    u8 telem;
    u8 music_shutdown;
    enum Mode mode;
    u16 batt_alarm;
    u8 power_alarm;
    u16 batt_critical;
    u16 batt_warning_interval;
    u8 splash_delay;
    u8 volume;
    u8 vibration_state; // for future vibration on/off support
#if HAS_RTC
    u8 rtcflags;    // bit0: clock12hr, bit1-3: time format, bit4-7 date format (see pages/320x240x16/rtc_config.c)
#endif
    struct mcu_pin module_enable[TX_MODULE_LAST];
    u8 module_poweramp;
    struct StickCalibration calibration[INP_HAS_CALIBRATION];
    struct TouchCalibration touch;
    struct AutoDimmer auto_dimmer;
    struct CountDownTimerSettings countdown_timer_settings;
};

extern struct Transmitter Transmitter;
void CONFIG_LoadTx();

#endif
