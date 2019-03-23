#ifndef _TX_H_
#define _TX_H_

#include "mixer.h"
#include "autodimmer.h"
#include "telemetry.h"
#include "music.h"

#define DEFAULT_BATTERY_WARNING_INTERVAL 30
#define MIN_BATTERY_WARNING_INTERVAL 0
#define MAX_BATTERY_WARNING_INTERVAL 60
#define DEFAULT_TELEM_INTERVAL 15
#define DEFAULT_SPLASH_DELAY 35 //3.5sec

enum {
    CYRF6936_DEVO       = 0x00,
    CYRF6936_AWA24S     = 0x01,
    CYRF6936_BUYCHINA   = 0x02,
    CC2500_REVERSE_GD02 = 0x01,
};

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

#if 0 // Defined in ports.h
struct mcu_pin {
    u32 port;
    u32 pin; //This only needs to be u16, but we need the struct to be word-aligned
};
#endif

enum ExtraHardware {
    VIBRATING_MOTOR = 0x01,
    FORCE_MODULES   = 0x02,
};

// bitmap for rtcflags:
#define CLOCK12HR 0x01  //0b00000001

#ifdef HAS_MORE_THAN_32_INPUTS
typedef u64 srcsize_t;
#else
typedef u32 srcsize_t;
#endif

struct Transmitter {
    u8 current_model;
    u8 language;
    u8 backlight;
    u8 contrast;
    u8 telem;
    u8 music_shutdown;
    u8 extra_hardware;
#if HAS_EXTENDED_AUDIO
    enum AudioPlayers audio_player;
    u8 audio_vol;
#endif
#if HAS_AUDIO_UART
    u8 audio_uart;
#endif
    enum Mode mode;
    u16 batt_alarm;
    u16 batt_critical;
    u16 batt_warning_interval;
    u16 telem_alert_interval;
    u8 power_alarm;
    u8 splash_delay;
    u8 volume;
    u8 module_poweramp;
    u8 vibration_state; // for future vibration on/off support
#if HAS_RTC
    u8 rtc_timeformat;    // bit0: clock12hr, bit1-3: time format
    u8 rtc_dateformat;    // bit0-3 date format (see pages/320x240x16/rtc_config.c)
#else
    u8 padding_1[1];
#endif

    srcsize_t ignore_src;
    u32 ignore_buttons;
    struct mcu_pin module_enable[TX_MODULE_LAST];
    u8 module_config[TX_MODULE_LAST];
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

#define TX_HAS_SRC(x) ((~Transmitter.ignore_src & (((srcsize_t)1) << x)) == (((srcsize_t)1) << x))

#endif
