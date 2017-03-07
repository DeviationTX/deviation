#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <telemetry.h>

enum Music {
    MUSIC_STARTUP = 0,
    MUSIC_SHUTDOWN,
    MUSIC_VOLUME,
    MUSIC_ALARM1,
    MUSIC_ALARM2,
    MUSIC_ALARM3,
    MUSIC_ALARM4,
    MUSIC_BATT_ALARM,
    MUSIC_DONE_BINDING,
    MUSIC_TIMER_WARNING,
    MUSIC_KEY_PRESSING,
    MUSIC_SAVING,
    MUSIC_MAXLEN,
    MUSIC_TELEMALARM1,
    MUSIC_TELEMALARM2,
    MUSIC_TELEMALARM3,
    MUSIC_TELEMALARM4,
    MUSIC_TELEMALARM5,
    MUSIC_TELEMALARM6,
    MUSIC_INACTIVITY_ALARM,
    MUSIC_TOTAL,	// Indicate total number of system music entries
};

#if HAS_EXTENDED_AUDIO
#define MAX_MUSICMAP_ENTRIES 240 // arbitraty chosen
#ifdef _DEVO12_TARGET_H_ // Check for Devo12 screen
#define MAX_MUSIC_LABEL 35 // limit label length due to limited screen width
#else
#define MAX_MUSIC_LABEL 26
#endif
#define MUSIC_UNIT_OFFSET 130
#define CUSTOM_ALARM_ID 151 // start of custom MP3 IDs
#define MUSIC_DEC_SEP 110  // MP3 ID of DECSEP = 110 + MUSIC_TOTAL
#define NUM_STICKS	4
#define NUM_AUX_KNOBS	(INP_HAS_CALIBRATION - NUM_STICKS)	// Exclude sticks
#define NUM_SWITCHES (NUM_INPUTS - INP_HAS_CALIBRATION)
#define MODEL_CUSTOM_ALARMS (NUM_SWITCHES + NUM_AUX_KNOBS * 2 + TELEM_NUM_ALARMS)

enum AudioPlayers {
  AUDIO_NONE = 0,	// Just use Tx beeps.
  AUDIO_AUDIOFX,	// Adafruit AUDIOFX board
  AUDIO_DF_PLAYER,	// DF Player Mini
  AUDIO_LAST
};

enum AudioDevices {
    AUDDEV_UNDEF = 0,	// Undefined (Use EXTAUDIO if extended-audio is enabled, otherwise use BUZZER)
    AUDDEV_ALL,
    AUDDEV_BUZZER,
    AUDDEV_EXTAUDIO,
    AUDDEV_LAST
};

enum {
    MUSIC_UNIT_NONE = 0,
    MUSIC_UNIT_MINUS,
    MUSIC_UNIT_TEMP,
    MUSIC_UNIT_VOLT,
    MUSIC_UNIT_RPM,
    MUSIC_UNIT_AMPS,
    MUSIC_UNIT_ALTITUDE,
    MUSIC_UNIT_GFORCE,
    MUSIC_UNIT_KMH,
    MUSIC_UNIT_MPS,
    MUSIC_UNIT_MPH,
    MUSIC_UNIT_KNOTS,
    MUSIC_UNIT_METERS,
    MUSIC_UNIT_CELSIUS,
    MUSIC_UNIT_FAHRENHEIT,
    MUSIC_UNIT_PERCENT,
    MUSIC_UNIT_WATT,
    MUSIC_UNIT_DB,
    MUSIC_UNIT_SECONDS,
    MUSIC_UNIT_MINUTES,
    MUSIC_UNIT_HOURS,
    MUSIC_UNIT_TIME, // used for auto-splitting to hours/minutes/seconds
    MUSIC_UNIT_TOTAL
};

struct ButtonMusic {
    u16 on;             // Music to be played when button is On
    u16 off;
    u8 music;            // Music to be played when button is Off
};

struct CustomMusic {
    u8 music;
};

struct  Music_Nr {
    struct CustomMusic switches[NUM_SWITCHES];	//Switch array to point to music file number, no pots
    struct CustomMusic telemetry[TELEM_NUM_ALARMS]; //Telemetry Alarm array to point to music file number
    struct ButtonMusic buttons[NUM_TX_BUTTONS];	//Button array to point to music file number
#if NUM_AUX_KNOBS
    struct CustomMusic aux[NUM_AUX_KNOBS * 2]; //two per knob for up and down
#endif
};

u8 music_map_entries;

struct MusicMap {
    u16 musicid;
    u16 duration;
#if HAS_MUSIC_CONFIG
    char label[MAX_MUSIC_LABEL];
#endif
};

struct MusicMap music_map[MAX_MUSICMAP_ENTRIES];

u16 MUSIC_GetTelemetryAlarm(enum Music music);
void MUSIC_PlayValue(u16 music, s32 value, u8 unit, u8 prec);

#endif //HAS_EXTENDED_AUDIO

void MUSIC_Beep(char* note, u16 duration, u16 interval, u8 count);

void MUSIC_Play(u16 music);

#endif
