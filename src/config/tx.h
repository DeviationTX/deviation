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
    u32 pin; //This only needs to be u16, but we need the struct to be word-aligned
};

enum ExtraHardware {
    VIBRATING_MOTOR = 0x01,
    FORCE_MODULES   = 0x02,
};

// bitmap for rtcflags:
#define CLOCK12HR 0x01  //0b00000001
#define TIMEFMT   0x0F  //0b00001111
#define DATEFMT   0xF0  //0b11110000

struct Transmitter {
    u8 current_model;
    u8 language;
    u8 backlight;
    u8 contrast;
    u8 telem;
    u8 music_shutdown;
    u8 extra_hardware;
    enum Mode mode;
    u16 batt_alarm;
    u16 batt_critical;
    u16 batt_warning_interval;
    u8 power_alarm;
    u8 splash_delay;
    u8 volume;
    u8 module_poweramp;
    u8 vibration_state; // for future vibration on/off support
    u8 padding_1[1];
#if HAS_RTC
    u8 rtcflags;    // bit0: clock12hr, bit1-3: time format, bit4-7 date format (see pages/320x240x16/rtc_config.c)
#endif
    
    #ifdef HAS_MORE_THAN_32_INPUTS
        u64 ignore_src;
    #else
        u32 ignore_src;
    #endif
    struct mcu_pin module_enable[TX_MODULE_LAST];
    u32 txid;
    struct StickCalibration calibration[INP_HAS_CALIBRATION];
    struct TouchCalibration touch;
    struct AutoDimmer auto_dimmer;
    struct CountDownTimerSettings countdown_timer_settings;
};

extern struct Transmitter Transmitter;
#define MODULE_ENABLE Transmitter.module_enable

void CONFIG_LoadTx();
void CONFIG_LoadHardware();

#endif
