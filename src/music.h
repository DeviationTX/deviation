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

#if HAS_EXTENDED_AUDIO
#define MAX_MUSICMAP_ENTRIES 80 // arbitraty chosen
#if HAS_TOUCH
#define MAX_MUSIC_LABEL 28 // limit label length due to limited screen width
#else
#define MAX_MUSIC_LABEL 26
#endif
#define CUSTOM_ALARM_ID 2000
#define MODEL_CUSTOM_ALARMS (NUM_INPUTS - INP_HAS_CALIBRATION + TELEM_NUM_ALARMS)
#define NUM_STICKS	4
#define NUM_AUX_KNOBS	(INP_HAS_CALIBRATION - NUM_STICKS)	// Exclude sticks

enum {
    TELEM_CUSTOM_NONE = 99,
    TELEM_ALARM_CUSTOM1,
    TELEM_ALARM_CUSTOM2,
    TELEM_ALARM_CUSTOM3,
    TELEM_ALARM_CUSTOM4,
    TELEM_ALARM_CUSTOM5,
    TELEM_ALARM_CUSTOM6,
};

enum {
    TELEM_UNIT_NONE = 0,
    TELEM_UNIT_TEMP,
    TELEM_UNIT_VOLT,
    TELEM_UNIT_RPM,
    TELEM_UNIT_AMPS,
    TELEM_UNIT_ALTITUDE,
    TELEM_UNIT_GFORCE,
    TELEM_UNIT_SECONDS,
};

struct AuxMusic {
    u16 up;              // Music to be played when Aux turns up
    u16 down;            // Music to be played when Aux turns down
};

struct ButtonMusic {
    u16 on;             // Music to be played when button is On
    u16 off;            // Music to be played when button is Off
};

struct CustomMusic {
  u16 music;
};

struct TelemetryMusic {
  u16 music;
};

struct  Music_Nr {
  struct CustomMusic switches[NUM_INPUTS - INP_HAS_CALIBRATION];	//Switch array to point to music file number, no pots
  struct CustomMusic telemetry[TELEM_NUM_ALARMS]; //Telemetry Alarm array to point to music file number
  struct ButtonMusic buttons[NUM_TX_BUTTONS];	//Button array to point to music file number
#if NUM_AUX_KNOBS
  struct AuxMusic aux[NUM_AUX_KNOBS];
#endif
};

u8 music_map_entries;
u8 music_map_custom_entries;

struct {u16 music; u16 duration; char label[MAX_MUSIC_LABEL];} music_map[MAX_MUSICMAP_ENTRIES];

u16 MUSIC_GetDuration(u16 music);
const char * MUSIC_GetLabel(u16 music);
u16 MUSIC_GetTelemetryAlarm(enum Music music);

void MUSIC_PlayValue(u16 music, u32 value, u16 unit);

#endif //HAS_EXTENDED_AUDIO

void MUSIC_Beep(char* note, u16 duration, u16 interval, u8 count);

void MUSIC_Play(u16 music);

#endif
